#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcException.h"
#include "OrcSingleton.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace Orc
{
    bool isExtensionAvailable(const std::string& extensionName)
    {
        std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
        for (const auto& extension : availableExtensions)
        {
            if (extensionName == extension.extensionName)
            {
                return true;
            }
        }
        return false;
    }

    class VulkanGraphicsDevice : public GraphicsDevice, public Singleton<VulkanGraphicsDevice>
    {
    public:
        VulkanGraphicsDevice(SDL_Window* window, uint32 width, uint32 height)
        {
            _createInstance();
            _createPhysicalDevice();
            _createSurface(window);
            _createDevice();
            _createSwapChain(width, height);
            _createQueue();
            _createCommandPool();
            _createCommandBuffer();
            _createSemaphore();
        }

        ~VulkanGraphicsDevice()
        {
            mDevice->waitIdle();
#ifdef ORC_PLATFORM_LINUX
            SDL_Vulkan_DestroySurface(mInstance.get(),mSurface,nullptr);
#else
            mInstance->destroySurfaceKHR(mSurface);
#endif
        }

        void _createInstance()
        {
            std::vector<const char*> extensions =
            {
                "VK_KHR_surface",
            };
            std::string videoDriverType = SDL_GetCurrentVideoDriver();
            if (videoDriverType == "windows")
            {
                extensions.emplace_back("VK_KHR_win32_surface");
            }
            else if (videoDriverType == "x11")
            {
                if (isExtensionAvailable("VK_KHR_xlib_surface"))
                {
                    extensions.emplace_back("VK_KHR_xlib_surface");
                }
                else
                {
                    throw OrcException("X11 extension not found");
                }
            }
            else if (videoDriverType == "wayland")
            {
                extensions.emplace_back("VK_KHR_wayland_surface");
            }

            vk::ApplicationInfo appInfo("ORC", 1, "ORC", 1, VK_API_VERSION_1_3);
            vk::InstanceCreateInfo createInfo(
                {},
                &appInfo,
                0, nullptr,
                static_cast<uint32>(extensions.size()), extensions.data()
            );
            mInstance = vk::createInstanceUnique(createInfo);
        }

        void _createPhysicalDevice()
        {
            auto physicalDevices = mInstance->enumeratePhysicalDevices();
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

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos =
            {
                vk::DeviceQueueCreateInfo({}, mGraphicsFamily, 1, &queuePriority),
                vk::DeviceQueueCreateInfo({}, mComputeFamily, 1, &queuePriority),
                vk::DeviceQueueCreateInfo({}, mTransferFamily, 1, &queuePriority)
            };

            std::vector<const char*> extensions =
            {
                "VK_KHR_swapchain",
            };
            // Open dynamic rendering
            vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures(true);
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

        void _createSurface(SDL_Window* window)
        {
#ifdef ORC_PLATFORM_WIN32
            auto props = SDL_GetWindowProperties(static_cast<SDL_Window*>(window));
            if (!props) { throw OrcException(SDL_GetError()); }
            auto instance = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
            if (!instance) { throw OrcException(SDL_GetError()); }
            auto hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            if (!hwnd) { throw OrcException(SDL_GetError()); }
            vk::Win32SurfaceCreateInfoKHR createInfo({}, reinterpret_cast<HINSTANCE>(instance), reinterpret_cast<HWND>(hwnd));
            mSurface = mInstance->createWin32SurfaceKHRcreateInfo);
#else
            VkSurfaceKHR surface;
            SDL_Vulkan_CreateSurface(window, mInstance.get(), nullptr, &surface);
            mSurface = surface;
#endif
        }

        void _createSwapChain(uint32 w, uint32 h)
        {
            vk::SwapchainCreateInfoKHR createInfo(
                {},
                mSurface,
                3,
                vk::Format::eR8G8B8A8Unorm,
                vk::ColorSpaceKHR::ePassThroughEXT,
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
        }

        void _createQueue()
        {
            mGraphicsQueue = mDevice->getQueue(mGraphicsFamily, 0);
            mComputeQueue = mDevice->getQueue(mComputeFamily, 0);
            mTransferQueue = mDevice->getQueue(mTransferFamily, 0);
        }

        void _createCommandPool()
        {
            vk::CommandPoolCreateInfo createInfo({}, mGraphicsFamily);
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

        void _createSemaphore()
        {
            vk::SemaphoreCreateInfo createInfo;
            mImageAvailableSemaphore = mDevice->createSemaphoreUnique(createInfo);
            mRenderFinishedSemaphore = mDevice->createSemaphoreUnique(createInfo);
        }

        void beginDraw()
        {
            auto frameIndex = mDevice->acquireNextImageKHR(mSwapChain.get(), std::numeric_limits<uint64>::max(), mImageAvailableSemaphore.get());
            mFrameIndex = frameIndex.value;
        }

        void endDraw()
        {
            vk::PresentInfoKHR presentInfo{};
            presentInfo.swapchainCount = 1;
            auto swapchainHandle = mSwapChain.get();
            presentInfo.pSwapchains = &swapchainHandle;
            presentInfo.pImageIndices = &mFrameIndex;
            vk::Semaphore needSemaphres;
            if (mHasRenderSubmission)
            {
                needSemaphres = mRenderFinishedSemaphore.get();
            }
            else
            {
                needSemaphres = mImageAvailableSemaphore.get();
            }
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &needSemaphres;

            if (mGraphicsQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
            {
                throw OrcException("Failed to present image");
            }
            mHasRenderSubmission = false;
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

        int32 mGraphicsFamily = -1;
        int32 mComputeFamily = -1;
        int32 mTransferFamily = -1;

        uint32 mFrameIndex = 0;

        bool mHasRenderSubmission = false;

        vk::UniqueInstance mInstance;
        vk::PhysicalDevice mPhysicalDevice;
        vk::UniqueDevice mDevice;
        vk::SurfaceKHR mSurface;
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
    };

    std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        return std::make_shared<VulkanGraphicsDevice>(static_cast<SDL_Window*>(windowHandle), width, height);
    }
}
#endif