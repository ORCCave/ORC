#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcException.h"
#include "OrcSingleton.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace Orc
{
    bool checkLayers(std::vector<char const*> const& layers, std::vector<vk::LayerProperties> const& properties)
    {
        // return true if all layers are listed in the properties
        return std::all_of(layers.begin(),
            layers.end(),
            [&properties](char const* name)
            {
                return std::any_of(properties.begin(),
                    properties.end(),
                    [&name](vk::LayerProperties const& property) { return strcmp(property.layerName, name) == 0; });
            });
    }

    class VulkanGraphicsDevice : public GraphicsDevice, public Singleton<VulkanGraphicsDevice>
    {
        struct VulkanSurfaceAndInstanceWrapper
        {
            VulkanSurfaceAndInstanceWrapper(SDL_Window* window)
            {
                uint32 count_instance_extensions;
                const char* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);
                if (instance_extensions == nullptr) { throw OrcException(SDL_GetError()); }
                std::vector<const char*> extensions(instance_extensions, instance_extensions + count_instance_extensions);
                vk::ApplicationInfo appInfo("ORC", 1, "ORC", 1, VK_API_VERSION_1_3);
                std::vector<const char*> layers;
#ifndef NDEBUG
                auto instanceLayerProperties = vk::enumerateInstanceLayerProperties();
                layers.emplace_back("VK_LAYER_KHRONOS_validation");
                if (checkLayers(layers, instanceLayerProperties))
                {
                    extensions.emplace_back("VK_EXT_debug_utils");
                }
                else
                {
                    layers.clear();
                }
#endif
                extensions.emplace_back("VK_EXT_swapchain_colorspace");
                vk::InstanceCreateInfo createInfo(
                    {},
                    &appInfo,
                    static_cast<uint32>(layers.size()), layers.size() ? layers.data() : nullptr,
                    static_cast<uint32>(extensions.size()), extensions.data()
                );
                mInstance = vk::createInstanceUnique(createInfo);
#ifdef ORC_PLATFORM_WIN32
                auto props = SDL_GetWindowProperties(static_cast<SDL_Window*>(window));
                if (!props) { throw OrcException(SDL_GetError()); }
                auto instance = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
                if (!instance) { throw OrcException(SDL_GetError()); }
                auto hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                if (!hwnd) { throw OrcException(SDL_GetError()); }
                vk::Win32SurfaceCreateInfoKHR surfaceInfo({}, reinterpret_cast<HINSTANCE>(instance), reinterpret_cast<HWND>(hwnd));
                mSurface = static_cast<VkSurfaceKHR>(mInstance->createWin32SurfaceKHR(surfaceInfo));
#else
                if (!SDL_Vulkan_CreateSurface(window, mInstance.get(), nullptr, &mSurface)) { throw OrcException(SDL_GetError()); }
#endif
            }

            ~VulkanSurfaceAndInstanceWrapper()
            {
#ifdef ORC_PLATFORM_WIN32
                mInstance->destroySurfaceKHR(mSurface);
#else
                SDL_Vulkan_DestroySurface(mInstance.get(), mSurface, nullptr);
#endif
            }

            VkSurfaceKHR mSurface;
            vk::UniqueInstance mInstance;
        };

    public:
        VulkanGraphicsDevice(SDL_Window* window, uint32 width, uint32 height) : mSurfaceAndInstance(window)
        {
            _createPhysicalDevice();
            _createDevice();
            _createSwapChain(width, height);
            _createQueue();
            _createCommandPool();
            _createCommandBuffer();
            _createSemaphore();
            _createFence();
            _transitionSwapchainForDrawing();
        }

        ~VulkanGraphicsDevice()
        {
            mDevice->waitIdle();
        }

        void _createPhysicalDevice()
        {
            auto physicalDevices = mSurfaceAndInstance.mInstance->enumeratePhysicalDevices();
            if (physicalDevices.empty())
            {
                throw OrcException("Physical device not found");
            }

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
            auto findQueueFamilyEx = [&queueFamilies](vk::QueueFlagBits flag, const std::vector<int32>& excludes = {}) -> int32
            {
                for (int32 i = 0; i < static_cast<int32>(queueFamilies.size()); ++i)
                {
                    if (std::find(excludes.begin(), excludes.end(), i) != excludes.end())
                        continue;
                    if (queueFamilies[i].queueFlags & flag)
                        return i;
                }
                return -1;
            };

            mGraphicsFamily = findQueueFamilyEx(vk::QueueFlagBits::eGraphics);
            if (mGraphicsFamily == -1)
                throw OrcException("Graphics queue family not found");

            mComputeFamily = findQueueFamilyEx(vk::QueueFlagBits::eCompute, { mGraphicsFamily });
            if (mComputeFamily == -1)
                mComputeFamily = mGraphicsFamily;

            mTransferFamily = findQueueFamilyEx(vk::QueueFlagBits::eTransfer, { mGraphicsFamily,mComputeFamily });
            if (mTransferFamily == -1)
            {
                if (queueFamilies[mGraphicsFamily].queueFlags & vk::QueueFlagBits::eTransfer)
                    mTransferFamily = mGraphicsFamily;
                else if (queueFamilies[mComputeFamily].queueFlags & vk::QueueFlagBits::eTransfer)
                    mTransferFamily = mComputeFamily;
                else
                    throw OrcException("Transfer queue family not found");
            }

            float queuePriority = 1.0f;
            std::map<uint32, uint32> familyQueueCount;
            familyQueueCount[mGraphicsFamily] += 1;
            familyQueueCount[mComputeFamily]  += 1;
            familyQueueCount[mTransferFamily] += 1;
            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
            for (auto& pair : familyQueueCount)
            {
                vk::DeviceQueueCreateInfo queueCreateInfo;
                queueCreateInfo.queueFamilyIndex = pair.first;
                auto availableQueues = queueFamilies[pair.first].queueCount;
                auto requestedQueues = pair.second;
                if (requestedQueues > availableQueues)
                {
                    requestedQueues = availableQueues;
                }
                queueCreateInfo.queueCount = requestedQueues;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);

                if (pair.first == mTransferFamily)
                {
                    mTransferQueueCount = requestedQueues;
                }
            }

            std::vector<const char*> extensions
            {
                "VK_KHR_swapchain",
            };
            // Open dynamic rendering
            vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures(true);
            vk::PhysicalDeviceSynchronization2Features sync2Features(true);
            dynamicRenderingFeatures.pNext = &sync2Features;
            vk::DeviceCreateInfo createInfo(
                {},
                static_cast<uint32>(queueCreateInfos.size()),
                queueCreateInfos.data(),
                0,
                nullptr,
                static_cast<uint32>(extensions.size()),
                extensions.data(),
                nullptr,
                &dynamicRenderingFeatures
            );
            mDevice = mPhysicalDevice.createDeviceUnique(createInfo);
        }

        void _createSwapChain(uint32 w, uint32 h)
        {
            auto surfaceFormat = _getSurfaceFormat();
            vk::SwapchainCreateInfoKHR createInfo(
                {},
                mSurfaceAndInstance.mSurface,
                3,
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
                VK_TRUE,
                {}
            );
            mSwapChain = mDevice->createSwapchainKHRUnique(createInfo);
            mSwapchainImages = mDevice->getSwapchainImagesKHR(mSwapChain.get());
        }

        void _createQueue()
        {
            mGraphicsQueue = mDevice->getQueue(mGraphicsFamily, mGraphicsQueueIndex);

            auto queueFamilies = mPhysicalDevice.getQueueFamilyProperties();

            if (mComputeFamily == mGraphicsFamily && queueFamilies[mComputeFamily].queueCount > 1) { mComputeQueueIndex = 1; }
            mComputeQueue = mDevice->getQueue(mComputeFamily, mComputeQueueIndex);

            // Avoid repeat
            mTransferQueueIndex = mTransferQueueCount - 1;
            mTransferQueue = mDevice->getQueue(mTransferFamily, mTransferQueueIndex);
        }

        void _createCommandPool()
        {
            mGraphicsCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsFamily));
            mComputeCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mComputeFamily));
            mTransferCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mTransferFamily));
        }

        void _createCommandBuffer()
        {
            mGraphicsList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS);
            mComputeList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE);
            mCopyList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COPY);
        }

        void _createFence()
        {
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                mMainFence[i] = mDevice->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
                // Fix first call bug.
                if (i != ORC_SWAPCHAIN_COUNT - 1)
                {
                    mDevice->resetFences(mMainFence[i].get());
                }
            }
        }

        void _transitionSwapchainForDrawing()
        {
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList->getRawCommandList()));
            vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            mGraphicsList->begin();
            for (uint32 i = 0;i < 3; ++i)
            {
                vk::ImageMemoryBarrier2 imageMemoryBarrier{};
                imageMemoryBarrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
                imageMemoryBarrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
                imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
                imageMemoryBarrier.dstAccessMask = {};
                imageMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
                imageMemoryBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
                imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageMemoryBarrier.image = mSwapchainImages[i];
                imageMemoryBarrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
                commandBuffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, {}, {}, 1, &imageMemoryBarrier));
            }
            mGraphicsList->end();
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

        void beginDraw()
        {
            if (auto result = mDevice->getFenceStatus(mMainFence[mLastCurrentIndex].get()); result == vk::Result::eNotReady)
            {
                mDevice->waitForFences(mMainFence[mLastCurrentIndex].get(), VK_TRUE, std::numeric_limits<uint64>::max());
            }
            else if (result != vk::Result::eSuccess) { throw OrcException("Failed to get fence status"); }

            mDevice->resetFences(mMainFence[mLastCurrentIndex].get());

            auto frameIndex = mDevice->acquireNextImageKHR(mSwapChain.get(), std::numeric_limits<uint64>::max(), mImageAvailableSemaphore[mCurrentIndex].get());
            mFrameIndex = frameIndex.value;

            vk::ImageMemoryBarrier2 imageMemoryBarrier{};
            imageMemoryBarrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            imageMemoryBarrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            imageMemoryBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
            imageMemoryBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = mSwapchainImages[mFrameIndex];
            imageMemoryBarrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            mGraphicsList->begin();
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList->getRawCommandList()));
            commandBuffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, {}, {}, 1, &imageMemoryBarrier));
        }

        void endDraw()
        {
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList->getRawCommandList()));
            vk::ImageMemoryBarrier2 imageMemoryBarrier{};
            imageMemoryBarrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            imageMemoryBarrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
            imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            imageMemoryBarrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
            imageMemoryBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            imageMemoryBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            imageMemoryBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            imageMemoryBarrier.image = mSwapchainImages[mFrameIndex];
            imageMemoryBarrier.subresourceRange = vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 );
            commandBuffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, {}, {}, 1, &imageMemoryBarrier));
            mGraphicsList->end();

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
            {
                throw OrcException("Failed to present image");
            }

            mLastCurrentIndex = mCurrentIndex;
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

        void executeCommandList(GraphicsCommandList::GraphicsCommandListType type, uint32 numLists, GraphicsCommandList* const* lists)
        {
            std::vector<vk::CommandBuffer> commandBuffers;
            for (uint32 i = 0; i < numLists; ++i)
            {
                auto rawList = lists[i]->getRawCommandList();
                commandBuffers.emplace_back(static_cast<VkCommandBuffer>(rawList));
            }
            vk::SubmitInfo submitInfo;
            submitInfo.commandBufferCount = numLists;
            submitInfo.pCommandBuffers = commandBuffers.data();
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

        void clearSwapChainColor(float r, float g, float b, float a)
        {
            vk::ClearColorValue clearColor(r, g, b, a);
            vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(mGraphicsList->getRawCommandList()));
            commandBuffer.clearColorImage(mSwapchainImages[mFrameIndex], vk::ImageLayout::eColorAttachmentOptimal, &clearColor, 1, &subresourceRange);
        }

        GraphicsCommandList* getInternalCommandList(GraphicsCommandList::GraphicsCommandListType type) const
        {
            GraphicsCommandList* list = nullptr;
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                list = mGraphicsList.get();
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                list = mCopyList.get();
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                list = mComputeList.get();
                break;
            }
            return list;
        }

        vk::SurfaceFormatKHR _getSurfaceFormat()
        {
            auto formats = mPhysicalDevice.getSurfaceFormatsKHR(mSurfaceAndInstance.mSurface);
            if (formats.empty())
            {
                throw OrcException("Surface format not found");
            }
            const std::vector<vk::Format> preferredFormats =
            {
                vk::Format::eR8G8B8A8Unorm,
                vk::Format::eB8G8R8A8Unorm,
                vk::Format::eR8G8B8A8Srgb,
                vk::Format::eB8G8R8A8Srgb,
            };

            for (const auto& pref : preferredFormats)
            {
                auto it = std::find_if(formats.begin(), formats.end(),
                    [pref](const vk::SurfaceFormatKHR& sf)
                    {
                        return (sf.format == pref && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
                    }
                );
                if (it != formats.end()) {
                    return *it;
                }
            }

            for (const auto& pref : preferredFormats)
            {
                auto it = std::find_if(formats.begin(), formats.end(),
                    [pref](const vk::SurfaceFormatKHR& sf)
                    {
                        return (sf.format == pref);
                    }
                );
                if (it != formats.end())
                {
                    return *it;
                }
            }

            return formats[0];
        }

        int32 mGraphicsFamily = -1;
        int32 mComputeFamily = -1;
        int32 mTransferFamily = -1;
        uint32 mGraphicsQueueIndex = 0;
        uint32 mComputeQueueIndex = 0;
        uint32 mTransferQueueIndex = 0;

        uint32 mTransferQueueCount = 0;

        uint32 mFrameIndex = 0;
        uint32 mCurrentIndex = 0;
        uint32 mLastCurrentIndex = ORC_SWAPCHAIN_COUNT - 1;

        VulkanSurfaceAndInstanceWrapper mSurfaceAndInstance;

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

        std::shared_ptr<GraphicsCommandList> mGraphicsList;
        std::shared_ptr<GraphicsCommandList> mComputeList;
        std::shared_ptr<GraphicsCommandList> mCopyList;
    };

    std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        return std::make_shared<VulkanGraphicsDevice>(static_cast<SDL_Window*>(windowHandle), width, height);
    }
}
#endif