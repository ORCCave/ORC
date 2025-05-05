#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsDevice.h"

#include "OrcException.h"
#include "OrcGraphicsCommandList.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Orc
{
    bool checkValidation()
    {
        std::vector<vk::LayerProperties> properties = vk::enumerateInstanceLayerProperties();
        for (auto const& property : properties)
            if (std::string(property.layerName.data()) == "VK_LAYER_KHRONOS_validation")
                return true;
        return false;
    }

    class VulkanGraphicsDevice : public GraphicsDevice
    {
        // For raii
        struct VulkanSurfaceWrapper
        {
            VulkanSurfaceWrapper(vk::Instance instance, SDL_Window* window) : mTempInst(instance)
            {
                if (!SDL_Vulkan_CreateSurface(window, instance, &mSurface)) 
                    throw OrcException(SDL_GetError());
            }

            ~VulkanSurfaceWrapper()
            {
                vkDestroySurfaceKHR(mTempInst, mSurface, nullptr);
            }

            vk::Instance mTempInst;
            VkSurfaceKHR mSurface;
        };

        void checkAndAddExtension(std::vector<const char*>& extensions, const char* name)
        {
            if (std::find_if(extensions.begin(), extensions.end(), [name](const char* ext) { return std::strcmp(ext, name) == 0; }) == extensions.end())
                extensions.push_back(name);
        }

    public:
        VulkanGraphicsDevice(SDL_Window* window, uint32 width, uint32 height) : GraphicsDevice(GraphicsDeviceType::GDT_VULKAN)
        {
            mWidth = width;
            mHeight = height;
            _createInstance(window);
            mSurfaceWrapper = std::make_unique<VulkanSurfaceWrapper>(mInstance.get(), window);
            _createPhysicalDevice();
            _createDevice();
            _createSwapChain(width, height);
            _createQueue();
            _createCommandPool();
            _createCommandBuffer();
            _createSemaphore();
            _createFence();
            _transitionSwapchainForDrawing();
            _createView();
        }

        ~VulkanGraphicsDevice()
        {
            mDevice->waitIdle();
            mSwapChain.reset();
            mSurfaceWrapper.reset();
            mListsCache.clear();
        }

        void _createInstance(SDL_Window* window)
        {
            uint32 count_instance_extensions;
            if(!SDL_Vulkan_GetInstanceExtensions(window, &count_instance_extensions, nullptr))
                throw OrcException(SDL_GetError());
            std::vector<const char*> extensions(count_instance_extensions);
            if (!SDL_Vulkan_GetInstanceExtensions(window, &count_instance_extensions, extensions.data()))
                throw OrcException(SDL_GetError());
            std::vector<const char*> layers;
#ifndef NDEBUG
            if (checkValidation())
            {
                layers.emplace_back("VK_LAYER_KHRONOS_validation");
                checkAndAddExtension(extensions, "VK_EXT_debug_utils");
            }
#endif
            checkAndAddExtension(extensions, "VK_EXT_swapchain_colorspace");

            vk::ApplicationInfo appInfo("ORC", 1, "ORC", 1, VK_API_VERSION_1_3);
            vk::InstanceCreateInfo createInfo(
                {},
                &appInfo,
                static_cast<uint32>(layers.size()), layers.size() ? layers.data() : nullptr,
                static_cast<uint32>(extensions.size()), extensions.data()
            );
            mInstance = vk::createInstanceUnique(createInfo);
        }

        void _createPhysicalDevice()
        {
            auto physicalDevices = mInstance->enumeratePhysicalDevices();
            if (physicalDevices.empty())
                throw OrcException("Physical device not found");

            mPhysicalDevice = physicalDevices[0];
            for (const auto& device : physicalDevices)
            {
                if (auto properties = device.getProperties(); properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                {
                    mPhysicalDevice = device;
                    break;
                }
            }
        }

        void _createDevice()
        {
            auto queueFamilies = mPhysicalDevice.getQueueFamilyProperties();
            for (uint32 i = 0; i < queueFamilies.size(); ++i) {
                const auto& props = queueFamilies[i];

                if ((props.queueFlags & vk::QueueFlagBits::eGraphics) && !mGraphicsFamily.has_value())
                {
                    mGraphicsFamily = i;
                    continue;
                }
                if ((props.queueFlags & vk::QueueFlagBits::eCompute) && !mComputeFamily.has_value())
                {
                    mComputeFamily = i;
                    continue;
                }
                if ((props.queueFlags & vk::QueueFlagBits::eTransfer) && !mTransferFamily.has_value())
                {
                    mTransferFamily = i;
                    continue;
                }
            }

            if(!mGraphicsFamily.has_value())
                throw OrcException("Graphics queue family not found");
            if(!mComputeFamily.has_value())
                mComputeFamily = mGraphicsFamily;

            if (!mTransferFamily.has_value())
            {
                if (queueFamilies[mGraphicsFamily.value()].queueFlags & vk::QueueFlagBits::eTransfer)
                    mTransferFamily = mGraphicsFamily;
                else if (queueFamilies[mComputeFamily.value()].queueFlags & vk::QueueFlagBits::eTransfer)
                    mTransferFamily = mComputeFamily;
                else
                    throw OrcException("Transfer queue family not found");
            }

            float graphicspriority = 1.0f;
            float computepriority = 0.8f;
            float copypriority = 0.5f;

            _getGraphicsIndex(graphicspriority);
            _getComputeIndex(computepriority);
            _getCopyIndex(copypriority);

            std::vector<const char*> extensions = { "VK_KHR_swapchain" };
            // Open dynamic rendering
            vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures(true);
            vk::PhysicalDeviceSynchronization2Features sync2Features(true);
            dynamicRenderingFeatures.pNext = &sync2Features;
            vk::DeviceCreateInfo createInfo(
                {},
                static_cast<uint32>(mQueueCreateInfos.size()),
                mQueueCreateInfos.data(),
                0,
                nullptr,
                static_cast<uint32>(extensions.size()),
                extensions.data(),
                nullptr,
                &dynamicRenderingFeatures
            );
            mDevice = mPhysicalDevice.createDeviceUnique(createInfo);
        }

        void _getGraphicsIndex(float& priority)
        {
            vk::DeviceQueueCreateInfo qci{};
            qci.queueFamilyIndex = mGraphicsFamily.value();
            qci.queueCount = 1;
            qci.pQueuePriorities = &priority;
            mQueueCreateInfos.push_back(qci);
            mGraphicsQueueIndex = 0;
        }

        void _getComputeIndex(float& priority)
        {
            if (mComputeFamily == mGraphicsFamily)
            {
                auto queueFamilies = mPhysicalDevice.getQueueFamilyProperties();
                mQueueCreateInfos[0].queueCount = queueFamilies[mComputeFamily.value()].queueCount == 1 ? 1 : 2;
                mComputeQueueIndex = mQueueCreateInfos[0].queueCount - 1;
            }
            else
            {
                vk::DeviceQueueCreateInfo qci{};
                qci.queueFamilyIndex = mComputeFamily.value();
                qci.queueCount = 1;
                qci.pQueuePriorities = &priority;
                mQueueCreateInfos.push_back(qci);
                mComputeQueueIndex = 0;
            }
        }

        void _getCopyIndex(float& priority)
        {
            auto queueFamilies = mPhysicalDevice.getQueueFamilyProperties();
            if (mTransferFamily == mGraphicsFamily && mTransferFamily == mComputeFamily)
            {
                mQueueCreateInfos[0].queueCount = queueFamilies[mTransferFamily.value()].queueCount >= 3 ? 3 : queueFamilies[mTransferFamily.value()].queueCount;
                mTransferQueueIndex = mQueueCreateInfos[0].queueCount - 1;
            }
            else if (mTransferFamily == mGraphicsFamily)
            {
                mQueueCreateInfos[0].queueCount = queueFamilies[mTransferFamily.value()].queueCount == 1 ? 1 : 2;
                mTransferQueueIndex = mQueueCreateInfos[0].queueCount - 1;
            }
            else if (mTransferFamily == mComputeFamily)
            {
                mQueueCreateInfos[1].queueCount = queueFamilies[mTransferFamily.value()].queueCount == 1 ? 1 : 2;
                mTransferQueueIndex = mQueueCreateInfos[1].queueCount - 1;
            }
            else
            {
                vk::DeviceQueueCreateInfo qci{};
                qci.queueFamilyIndex = mTransferFamily.value();
                qci.queueCount = 1;
                qci.pQueuePriorities = &priority;
                mQueueCreateInfos.push_back(qci);
                mTransferQueueIndex = 0;
            }
        }

        void _createSwapChain(uint32 w, uint32 h)
        {
            auto surfaceFormat = _getSurfaceFormat();
            vk::SwapchainCreateInfoKHR createInfo(
                {},
                mSurfaceWrapper->mSurface,
                ORC_SWAPCHAIN_COUNT,
                surfaceFormat.format,
                surfaceFormat.colorSpace,
                vk::Extent2D(w, h),
                1,
                vk::ImageUsageFlagBits::eColorAttachment,
                vk::SharingMode::eExclusive,
                {},
                vk::SurfaceTransformFlagBitsKHR::eIdentity,
                vk::CompositeAlphaFlagBitsKHR::eOpaque,
                vk::PresentModeKHR::eFifo,
                vk::True,
                {}
            );
            mSwapChain = mDevice->createSwapchainKHRUnique(createInfo);
            mSwapchainImages = mDevice->getSwapchainImagesKHR(mSwapChain.get());
        }

        void _createQueue()
        {
            mGraphicsQueue = mDevice->getQueue(mGraphicsFamily.value(), mGraphicsQueueIndex);
            mComputeQueue = mDevice->getQueue(mComputeFamily.value(), mComputeQueueIndex);
            mTransferQueue = mDevice->getQueue(mTransferFamily.value(), mTransferQueueIndex);
        }

        void _createCommandPool()
        {
            auto this_id = std::this_thread::get_id();
            mGraphicsCommandPool[this_id] = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsFamily.value()));
            mComputeCommandPool[this_id] = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mComputeFamily.value()));
            mTransferCommandPool[this_id] = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mTransferFamily.value()));
        }

        void _createCommandBuffer()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                mGraphicsList[i] = _createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS, true);
                mComputeList[i] = _createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE, true);
                mCopyList[i] = _createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COPY, true);
            }
        }

        void _createFence()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                mGMainFence[i] = mDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
                mTransferMainFence[i] = mDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
                mComputeFence[i] = mDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
            }
        }

        void _transitionSwapchainForDrawing()
        {
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList[0]->getRawCommandList()));
            mGraphicsList[0]->begin();
            for (uint32 i = 0; i < 3; ++i)
            {
                vk::ImageMemoryBarrier2 imageMemoryBarrier{};
                imageMemoryBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
                imageMemoryBarrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
                imageMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
                imageMemoryBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
                imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.image = mSwapchainImages[i];
                imageMemoryBarrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
                commandBuffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, {}, {}, 1, &imageMemoryBarrier));
            }
            mGraphicsList[0]->end();
            mGraphicsQueue.submit(vk::SubmitInfo({}, {}, {}, 1, &commandBuffer));
            mGraphicsQueue.waitIdle();
        }

        void _createSemaphore()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                mImageAvailableSemaphore[i] = mDevice->createSemaphoreUnique({});
                mRenderFinishedSemaphore[i] = mDevice->createSemaphoreUnique({});
            }
        }

        void _createView()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                vk::ImageViewCreateInfo createInfo{};
                createInfo.image = mSwapchainImages[i];
                createInfo.viewType = vk::ImageViewType::e2D;
                createInfo.format = mSwapChainFormat;
                createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;
                mSwapChainViews[i] = mDevice->createImageViewUnique(createInfo);
            }
        }

        void beginDraw()
        {
            vk::Fence fences[3] = { mGMainFence[mCurrentIndex].get(), mTransferMainFence[mCurrentIndex].get(), mComputeFence[mCurrentIndex].get() };

            if(mDevice->waitForFences(3, fences, vk::True, std::numeric_limits<uint64>::max()) != vk::Result::eSuccess)
                throw OrcException("Failed to wait for fences");

            if (mDevice->resetFences(3, fences) != vk::Result::eSuccess)
                throw OrcException("Failed to reset fences");

            auto frameIndex = mDevice->acquireNextImageKHR(mSwapChain.get(), std::numeric_limits<uint64>::max(), mImageAvailableSemaphore[mCurrentIndex].get());
            if (frameIndex.result != vk::Result::eSuccess)
                throw OrcException("Failed to acquire next image");

            mFrameIndex = frameIndex.value;
            mGraphicsList[mCurrentIndex]->begin();
            mCopyList[mCurrentIndex]->begin();
            mComputeList[mCurrentIndex]->begin();

            vk::CommandBuffer gCommandBuffer(static_cast<VkCommandBuffer>(mGraphicsList[mCurrentIndex]->getRawCommandList()));
            {
                vk::ImageMemoryBarrier2 preBarrier2(
                    vk::PipelineStageFlagBits2::eTopOfPipe,
                    vk::AccessFlagBits2::eNone,
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::AccessFlagBits2::eColorAttachmentWrite,
                    vk::ImageLayout::ePresentSrcKHR,
                    vk::ImageLayout::eAttachmentOptimal,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    mSwapchainImages[mFrameIndex],
                    { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
                );

                vk::DependencyInfo depBegin{};
                depBegin.imageMemoryBarrierCount = 1;
                depBegin.pImageMemoryBarriers = &preBarrier2;
                gCommandBuffer.pipelineBarrier2(depBegin);
            }

            vk::RenderingAttachmentInfo colorAttachment(mSwapChainViews[mFrameIndex].get(), vk::ImageLayout::eAttachmentOptimal);
            colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttachment.clearValue = vk::ClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
            vk::RenderingInfo renderingInfo{};
            renderingInfo.renderArea = vk::Rect2D({ 0, 0 }, { mWidth, mHeight });
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            gCommandBuffer.beginRendering(renderingInfo);
        }

        void endDraw()
        {
            vk::CommandBuffer gCommandBuffer(static_cast<VkCommandBuffer>(mGraphicsList[mCurrentIndex]->getRawCommandList()));
            vk::CommandBuffer copyCommandBuffer(static_cast<VkCommandBuffer>(mCopyList[mCurrentIndex]->getRawCommandList()));
            vk::CommandBuffer computeCommandBuffer(static_cast<VkCommandBuffer>(mComputeList[mCurrentIndex]->getRawCommandList()));

            gCommandBuffer.endRendering();
            if (!mGCmdBuffers.empty())
            {
                gCommandBuffer.executeCommands(mGCmdBuffers);
            }
            if (!mCopyCmdBuffers.empty())
            {
                copyCommandBuffer.executeCommands(mCopyCmdBuffers);
            }
            if (!mComputeCmdBuffers.empty())
            {
                computeCommandBuffer.executeCommands(mComputeCmdBuffers);
            }

            {
                vk::ImageMemoryBarrier2 postRenderBarrier2(
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::AccessFlagBits2::eColorAttachmentWrite,
                    vk::PipelineStageFlagBits2::eBottomOfPipe,
                    vk::AccessFlagBits2::eNone,
                    vk::ImageLayout::eAttachmentOptimal,
                    vk::ImageLayout::ePresentSrcKHR,
                    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                    mSwapchainImages[mFrameIndex],
                    { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
                );

                vk::DependencyInfo depEnd{};
                depEnd.imageMemoryBarrierCount = 1;
                depEnd.pImageMemoryBarriers = &postRenderBarrier2;
                gCommandBuffer.pipelineBarrier2(depEnd);
            }

            mGraphicsList[mCurrentIndex]->end();
            mCopyList[mCurrentIndex]->end();
            mComputeList[mCurrentIndex]->end();

            vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            mGraphicsQueue.submit(vk::SubmitInfo(1, &mImageAvailableSemaphore[mCurrentIndex].get(), &waitStageMask, 1, &gCommandBuffer, 1, &mRenderFinishedSemaphore[mCurrentIndex].get()), mGMainFence[mCurrentIndex].get());
            mTransferQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &copyCommandBuffer, 0, nullptr), mTransferMainFence[mCurrentIndex].get());
            mComputeQueue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &computeCommandBuffer, 0, nullptr), mComputeFence[mCurrentIndex].get());

            vk::PresentInfoKHR presentInfo{};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &mSwapChain.get();
            presentInfo.pImageIndices = &mFrameIndex;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &mRenderFinishedSemaphore[mCurrentIndex].get();

            if (mGraphicsQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
                throw OrcException("Failed to present image");

            mCurrentIndex = (mCurrentIndex + 1) % ORC_SWAPCHAIN_COUNT;
        }

        void* getRawGraphicsDevice() const
        {
            return static_cast<VkDevice>(mDevice.get());
        }

        void executeCommandList(GraphicsCommandList* list)
        {
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(list->getRawCommandList()));
            auto type = list->getCommandListType();
            std::lock_guard<std::mutex> lock(mExecuteMutex);
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                mGCmdBuffers.push_back(commandBuffer);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                mCopyCmdBuffers.push_back(commandBuffer);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                mComputeCmdBuffers.push_back(commandBuffer);
                break;
            }
        }

        std::shared_ptr<VulkanCommandList> _createCommandList(GraphicsCommandList::GraphicsCommandListType type, bool isPromary)
        {
            std::shared_ptr<VulkanCommandList> list;
            auto this_id = std::this_thread::get_id();
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                if (!mGraphicsCommandPool.contains(this_id))
                {
                    mGraphicsCommandPool[this_id] = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsFamily.value()));
                }
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mGraphicsCommandPool[this_id].get()), type, isPromary);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                if (!mTransferCommandPool.contains(this_id))
                {
                    mTransferCommandPool[this_id] = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mTransferFamily.value()));
                }
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mTransferCommandPool[this_id].get()), type, isPromary);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                if (!mComputeCommandPool.contains(this_id))
                {
                    mComputeCommandPool[this_id] = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mComputeFamily.value()));
                }
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mComputeCommandPool[this_id].get()), type, isPromary);
                break;
            }
            return list;
        }

        vk::SurfaceFormatKHR _getSurfaceFormat()
        {
            auto formats = mPhysicalDevice.getSurfaceFormatsKHR(mSurfaceWrapper->mSurface);
            if (formats.empty())
                throw OrcException("Surface format not found");

            const std::vector<vk::Format> preferredFormats =
            {
                vk::Format::eR8G8B8A8Unorm,
                vk::Format::eB8G8R8A8Unorm,
                vk::Format::eR8G8B8A8Srgb,
                vk::Format::eB8G8R8A8Srgb,
            };

            for (const auto& pref : preferredFormats)
            {
                if (auto it = std::find_if(formats.begin(), formats.end(), [pref](const vk::SurfaceFormatKHR& sf) {return (sf.format == pref && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);});it != formats.end())
                {
                    mSwapChainFormat = it->format;
                    return *it;
                }
            }

            for (const auto& pref : preferredFormats)
            {
                if (auto it = std::find_if(formats.begin(), formats.end(), [pref](const vk::SurfaceFormatKHR& sf) {return (sf.format == pref);});it != formats.end())
                {
                    mSwapChainFormat = it->format;
                    return *it;
                }
            }

            mSwapChainFormat = formats[0].format;
            return formats[0];
        }

        void runGarbageCollection()
        {
            mGCmdBuffers.clear();
            mCopyCmdBuffers.clear();
            mComputeCmdBuffers.clear();
            if (listCount >= 300)
            {
                // Remove all
                for (auto it = mListsCache.begin(); it != mListsCache.end(); ++it)
                {
                    auto& lists = it->second;
                    for (auto it2 = lists.begin(); it2 != lists.end();)
                    {
                        if (auto result = mDevice->getFenceStatus(it2->get()->mFence); result == vk::Result::eSuccess)
                        {
                            it2 = lists.erase(it2);
                            --listCount;
                        }
                        else
                        {
                            ++it2;
                        }
                    }
                }
            }
            else if (listCount >= 100)
            {
                // Remove one
                for (auto it = mListsCache.begin(); it != mListsCache.end(); ++it)
                {
                    auto& lists = it->second;
                    for (auto it2 = lists.begin(); it2 != lists.end(); ++it2)
                    {
                        if (auto result = mDevice->getFenceStatus(it2->get()->mFence); result == vk::Result::eSuccess)
                        {
                            lists.erase(it2);
                            --listCount;
                            return;
                        }
                    }
                }
            }
        }

        bool _checkCommandList(const std::vector<std::shared_ptr<VulkanCommandList>>& lists, GraphicsCommandList::GraphicsCommandListType type, size_t& index)
        {
            for (size_t i = 0; i < lists.size(); ++i)
            {
                if (lists[i]->getCommandListType() == type)
                {
                    if (auto result = mDevice->getFenceStatus(lists[i]->mFence); result == vk::Result::eSuccess)
                    {
                        index = i;
                        return true;
                    }
                }
            }
            return false;
        }

        GraphicsCommandList* _getCmdList(GraphicsCommandList::GraphicsCommandListType type)
        {
            vk::UniqueFence* realFence = nullptr;
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                realFence = &mGMainFence[mCurrentIndex];
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                realFence = &mTransferMainFence[mCurrentIndex];
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                realFence = &mComputeFence[mCurrentIndex];
                break;
            };

            auto this_id = std::this_thread::get_id();
            std::lock_guard<std::mutex> lock(mListMutex);
            if (mListsCache.contains(this_id))
            {
                auto& lists = mListsCache[this_id];
                size_t index = 0;
                if (_checkCommandList(lists, type, index))
                {
                    lists[index]->mFence = realFence->get();
                    return lists[index].get();
                }
                else
                {
                    ++listCount;
                    auto list = _createCommandList(type, false);
                    list->mFence = realFence->get();
                    lists.push_back(list);
                    return lists.back().get();
                }
            }

            ++listCount;
            std::vector<std::shared_ptr<VulkanCommandList>> lists;
            auto list = _createCommandList(type, false);
            list->mFence = realFence->get();
            lists.push_back(list);
            mListsCache[this_id] = lists;
            return lists.back().get();
        }

        vk::UniqueInstance mInstance;
        std::optional<uint32> mGraphicsFamily;
        std::optional<uint32> mComputeFamily;
        std::optional<uint32> mTransferFamily;
        uint32 mGraphicsQueueIndex = 0;
        uint32 mComputeQueueIndex = 0;
        uint32 mTransferQueueIndex = 0;

        uint32 mFrameIndex = 0;
        uint32 mCurrentIndex = 0;

        std::unique_ptr<VulkanSurfaceWrapper> mSurfaceWrapper;

        vk::PhysicalDevice mPhysicalDevice;
        vk::UniqueDevice mDevice;
        vk::UniqueSwapchainKHR mSwapChain;
        std::unordered_map<std::thread::id, vk::UniqueCommandPool> mGraphicsCommandPool;
        std::unordered_map<std::thread::id, vk::UniqueCommandPool> mComputeCommandPool;
        std::unordered_map<std::thread::id, vk::UniqueCommandPool> mTransferCommandPool;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mTransferQueue;
        vk::UniqueSemaphore mImageAvailableSemaphore[ORC_SWAPCHAIN_COUNT];
        vk::UniqueSemaphore mRenderFinishedSemaphore[ORC_SWAPCHAIN_COUNT];
        vk::UniqueFence mGMainFence[ORC_SWAPCHAIN_COUNT];
        vk::UniqueFence mTransferMainFence[ORC_SWAPCHAIN_COUNT];
        vk::UniqueFence mComputeFence[ORC_SWAPCHAIN_COUNT];

        std::vector<vk::Image> mSwapchainImages;
        std::vector<vk::ImageView> mSwapChainImageViews;

        std::shared_ptr<GraphicsCommandList> mGraphicsList[ORC_SWAPCHAIN_COUNT];
        std::shared_ptr<GraphicsCommandList> mComputeList[ORC_SWAPCHAIN_COUNT];
        std::shared_ptr<GraphicsCommandList> mCopyList[ORC_SWAPCHAIN_COUNT];

        vk::Format mSwapChainFormat = vk::Format::eUndefined;
        vk::UniqueImageView mSwapChainViews[ORC_SWAPCHAIN_COUNT];

        uint32 mWidth;
        uint32 mHeight;

        std::vector<vk::DeviceQueueCreateInfo> mQueueCreateInfos;
        
        std::mutex mListMutex;
        std::mutex mExecuteMutex;
        std::unordered_map<std::thread::id, std::vector<std::shared_ptr<VulkanCommandList>>> mListsCache;

        std::vector<vk::CommandBuffer> mGCmdBuffers;
        std::vector<vk::CommandBuffer> mCopyCmdBuffers;
        std::vector<vk::CommandBuffer> mComputeCmdBuffers;

        size_t listCount = 0;
    };

    std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        return std::make_shared<VulkanGraphicsDevice>(static_cast<SDL_Window*>(windowHandle), width, height);
    }
}
#endif