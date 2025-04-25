#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsDevice.h"

#include "OrcException.h"
#include "OrcGraphicsCommandList.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <vector>
#include <string>

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
                if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &mSurface)) 
                    throw OrcException(SDL_GetError());
            }

            ~VulkanSurfaceWrapper()
            {
                SDL_Vulkan_DestroySurface(mTempInst, mSurface, nullptr);
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
            _createInstance();
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
        }

        void _createInstance()
        {
            uint32 count_instance_extensions;
            const char* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);
            if (instance_extensions == nullptr) { throw OrcException(SDL_GetError()); }
            std::vector<const char*> extensions(instance_extensions, instance_extensions + count_instance_extensions);
            std::vector<const char*> layers;
#ifndef NDEBUG
            auto instanceLayerProperties = vk::enumerateInstanceLayerProperties();
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
            mGraphicsCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsFamily.value()));
            mComputeCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mComputeFamily.value()));
            mTransferCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mTransferFamily.value()));
        }

        void _createCommandBuffer()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
                mGraphicsList[i] = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS);

            mComputeList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE);
            mCopyList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COPY);
        }

        void _createFence()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
                mMainFence[i] = mDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
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
                imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
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
            if(mDevice->waitForFences(1, &mMainFence[mCurrentIndex].get(), VK_TRUE, std::numeric_limits<uint64>::max()) != vk::Result::eSuccess)
                throw OrcException("Failed to wait for fences");

            if (mDevice->resetFences(1, &mMainFence[mCurrentIndex].get()) != vk::Result::eSuccess)
                throw OrcException("Failed to reset fences");

            auto frameIndex = mDevice->acquireNextImageKHR(mSwapChain.get(), std::numeric_limits<uint64>::max(), mImageAvailableSemaphore[mCurrentIndex].get());
            if (frameIndex.result != vk::Result::eSuccess)
                throw OrcException("Failed to acquire next image");

            mFrameIndex = frameIndex.value;
            mGraphicsList[mCurrentIndex]->begin();

            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList[mCurrentIndex]->getRawCommandList()));

            vk::RenderingAttachmentInfo colorAttachment(mSwapChainViews[mFrameIndex].get(), vk::ImageLayout::eAttachmentOptimal);
            colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttachment.clearValue = vk::ClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

            vk::RenderingInfo renderingInfo{};
            renderingInfo.renderArea = vk::Rect2D({ 0, 0 }, { mWidth, mHeight });
            renderingInfo.layerCount = 1;
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;

            commandBuffer.beginRendering(renderingInfo);
        }

        void endDraw()
        {
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList[mCurrentIndex]->getRawCommandList()));

            commandBuffer.endRendering();

            mGraphicsList[mCurrentIndex]->end();

            vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            mGraphicsQueue.submit(vk::SubmitInfo(1, &mImageAvailableSemaphore[mCurrentIndex].get(), &waitStageMask, 1, &commandBuffer, 1, &mRenderFinishedSemaphore[mCurrentIndex].get()), 
                mMainFence[mCurrentIndex].get());

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

        std::shared_ptr<GraphicsCommandList> createCommandList(GraphicsCommandList::GraphicsCommandListType type)
        {
            std::shared_ptr<GraphicsCommandList> list;
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mGraphicsCommandPool.get()), type);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mTransferCommandPool.get()), type);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mComputeCommandPool.get()), type);
                break;
            }
            return list;
        }

        void executeCommandList(GraphicsCommandList* list)
        {
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(list->getRawCommandList()));
            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            auto type = list->getCommandListType();
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                mGraphicsQueue.submit(submitInfo);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                mTransferQueue.submit(submitInfo);
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                mComputeQueue.submit(submitInfo);
                break;
            }
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

        bool checkCmdListUsable(GraphicsCommandList* list)
        {
            return false;
        }

        void runGarbageCollection()
        {

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
        vk::UniqueCommandPool mGraphicsCommandPool;
        vk::UniqueCommandPool mComputeCommandPool;
        vk::UniqueCommandPool mTransferCommandPool;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mTransferQueue;
        vk::UniqueSemaphore mImageAvailableSemaphore[ORC_SWAPCHAIN_COUNT];
        vk::UniqueSemaphore mRenderFinishedSemaphore[ORC_SWAPCHAIN_COUNT];
        vk::UniqueFence mMainFence[ORC_SWAPCHAIN_COUNT];

        std::vector<vk::Image> mSwapchainImages;
        std::vector<vk::ImageView> mSwapChainImageViews;

        std::shared_ptr<GraphicsCommandList> mGraphicsList[ORC_SWAPCHAIN_COUNT];
        std::shared_ptr<GraphicsCommandList> mComputeList;
        std::shared_ptr<GraphicsCommandList> mCopyList;

        vk::Format mSwapChainFormat = vk::Format::eUndefined;
        vk::UniqueImageView mSwapChainViews[ORC_SWAPCHAIN_COUNT];

        uint32 mWidth;
        uint32 mHeight;

        std::vector<vk::DeviceQueueCreateInfo> mQueueCreateInfos;
    };

    std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        return std::make_shared<VulkanGraphicsDevice>(static_cast<SDL_Window*>(windowHandle), width, height);
    }
}
#endif