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
                auto requestedQueues = pair.second;
                vk::DeviceQueueCreateInfo queueCreateInfo;
                queueCreateInfo.queueFamilyIndex = pair.first;
                auto availableQueues = queueFamilies[pair.first].queueCount;
                if (requestedQueues > availableQueues)
                {
                    requestedQueues = availableQueues;
                }
                queueCreateInfo.queueCount = requestedQueues;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
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
            mGraphicsQueue = mDevice->getQueue(mGraphicsFamily, 0);
            mComputeQueue = mDevice->getQueue(mComputeFamily, 0);
            mTransferQueue = mDevice->getQueue(mTransferFamily, 0);
        }

        void _createCommandPool()
        {
            vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsFamily);
            mGraphicsCommandPool = mDevice->createCommandPoolUnique(createInfo);
            createInfo.queueFamilyIndex = mComputeFamily;
            mComputeCommandPool = mDevice->createCommandPoolUnique(createInfo);
            createInfo.queueFamilyIndex = mTransferFamily;
            mTransferCommandPool = mDevice->createCommandPoolUnique(createInfo);
        }

        void _createCommandBuffer()
        {
            vk::CommandBufferAllocateInfo allocateInfo(mGraphicsCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1);
            auto graphicsCommandBuffers = mDevice->allocateCommandBuffersUnique(allocateInfo);
            mGraphicsCommandBuffer = std::move(graphicsCommandBuffers[0]);
            allocateInfo.commandPool = mComputeCommandPool.get();
            auto computeCommandBuffers = mDevice->allocateCommandBuffersUnique(allocateInfo);
            mComputeCommandBuffer = std::move(computeCommandBuffers[0]);
            allocateInfo.commandPool = mTransferCommandPool.get();
            auto transferCommandPools = mDevice->allocateCommandBuffersUnique(allocateInfo);
            mTransferCommandBuffer = std::move(transferCommandPools[0]);
        }

        void _transitionSwapchainForDrawing()
        {
            vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            mGraphicsCommandBuffer->begin(beginInfo);
            for (uint32 i = 0;i < 3; ++i)
            {
                VkImageMemoryBarrier2 imageBarrier = {};
                imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                imageBarrier.srcAccessMask = 0;
                imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.image = mSwapchainImages[i];
                imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBarrier.subresourceRange.baseMipLevel = 0;
                imageBarrier.subresourceRange.levelCount = 1;
                imageBarrier.subresourceRange.baseArrayLayer = 0;
                imageBarrier.subresourceRange.layerCount = 1;
                VkDependencyInfo dependencyInfo = {};
                dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                dependencyInfo.imageMemoryBarrierCount = 1;
                dependencyInfo.pImageMemoryBarriers = &imageBarrier;
                vkCmdPipelineBarrier2(mGraphicsCommandBuffer.get(), &dependencyInfo);
            }
            mGraphicsCommandBuffer->end();
            vk::SubmitInfo submitInfo({}, {}, {}, 1, &mGraphicsCommandBuffer.get());
            mGraphicsQueue.submit(submitInfo);
            mGraphicsQueue.waitIdle();
        }

        void _createSemaphore()
        {
            vk::SemaphoreCreateInfo createInfo;
            mImageAvailableSemaphore = mDevice->createSemaphoreUnique(createInfo);
            mRenderFinishedSemaphore = mDevice->createSemaphoreUnique(createInfo);
        }

        void beginDraw()
        {
            //auto frameIndex = mDevice->acquireNextImageKHR(mSwapChain.get(), std::numeric_limits<uint64>::max(), mImageAvailableSemaphore.get());
            //mFrameIndex = frameIndex.value;
        }

        void endDraw()
        {
            //vk::PresentInfoKHR presentInfo{};
            //presentInfo.swapchainCount = 1;
            //auto swapchainHandle = mSwapChain.get();
            //presentInfo.pSwapchains = &swapchainHandle;
            //presentInfo.pImageIndices = &mFrameIndex;
            //vk::Semaphore needSemaphres;
            //if (mHasRenderSubmission)
            //{
            //    needSemaphres = mRenderFinishedSemaphore.get();
            //}
            //else
            //{
            //    needSemaphres = mImageAvailableSemaphore.get();
            //}
            //presentInfo.waitSemaphoreCount = 1;
            //presentInfo.pWaitSemaphores = &needSemaphres;

            //if (mGraphicsQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
            //{
            //    throw OrcException("Failed to present image");
            //}
            //mHasRenderSubmission = false;
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

        void clearSwapChainColor(GraphicsCommandList* list, float r, float g, float b, float a)
        {
            //vk::CommandBuffer commandBuffer(static_cast<VkCommandBuffer>(list->getRawCommandList()));
            //vk::ClearColorValue clearColor(r, g, b, a);

            //VkPhysicalDeviceVulkan13Features
        }

        vk::SurfaceFormatKHR _getSurfaceFormat()
        {
			auto formats = mPhysicalDevice.getSurfaceFormatsKHR(mSurfaceAndInstance.mSurface);
            if (formats.empty())
            {
				throw OrcException("Surface format not found");
            }
			for (const auto& format : formats)
			{
				if (format.format == vk::Format::eR8G8B8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					return format;
				}
				else if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					return format;
				}
			}
			return formats[0];
        }

        int32 mGraphicsFamily = -1;
        int32 mComputeFamily = -1;
        int32 mTransferFamily = -1;

        uint32 mFrameIndex = 0;

        bool mHasRenderSubmission = false;

        VulkanSurfaceAndInstanceWrapper mSurfaceAndInstance;

        vk::PhysicalDevice mPhysicalDevice;
        vk::UniqueDevice mDevice;
        vk::UniqueSwapchainKHR mSwapChain;
        vk::UniqueCommandPool mGraphicsCommandPool;
        vk::UniqueCommandPool mComputeCommandPool;
        vk::UniqueCommandPool mTransferCommandPool;
        vk::UniqueCommandBuffer mGraphicsCommandBuffer;
        vk::UniqueCommandBuffer mComputeCommandBuffer;
        vk::UniqueCommandBuffer mTransferCommandBuffer;
        vk::Queue mGraphicsQueue;
        vk::Queue mComputeQueue;
        vk::Queue mTransferQueue;
        vk::UniqueSemaphore mImageAvailableSemaphore;
        vk::UniqueSemaphore mRenderFinishedSemaphore;

        std::vector<vk::Image> mSwapchainImages;
    };

    std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        return std::make_shared<VulkanGraphicsDevice>(static_cast<SDL_Window*>(windowHandle), width, height);
    }
}
#endif