#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcException.h"
#include "OrcSingleton.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL.h>

namespace Orc
{
    class VulkanGraphicsDevice : public GraphicsDevice, public Singleton<VulkanGraphicsDevice>
    {
    public:
        VulkanGraphicsDevice(void* instance, void* windowHandle, uint32 width, uint32 height) : GraphicsDevice(GraphicsDeviceTypes::GDT_VULKAN)
        {
            _createInstance();
			_createPhysicalDevice();
            _createSurface(instance, windowHandle);
			_createDevice();
            _createSwapChain(width, height);
            _createQueue();
			_createCommandPool();
			_createCommandBuffer();
			_createSemaphore();
        }

        void _createInstance()
        {
            std::vector<const char*> extensions =
            {
                "VK_KHR_surface",
#ifdef ORC_PLATFORM_WIN32
                "VK_KHR_win32_surface"
#endif
            };

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
                throw OrcException("No physical device found");
            }

            mPhysicalDevice = physicalDevices[0];
            for (const auto& device : physicalDevices)
            {
                vk::PhysicalDeviceProperties properties = device.getProperties();
                if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                {
                    mPhysicalDevice = device;
                    break;
                }
            }
		}

        void _createDevice()
        {
            auto queueFamilies = mPhysicalDevice.getQueueFamilyProperties();
            auto findQueueFamilyEx = [&queueFamilies](vk::QueueFlagBits flag, const std::vector<int32>& excludes = {}) -> int32_t
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
                throw OrcException("No graphics queue family found");

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
                    throw OrcException("No transfer queue family found");
            }

            float queuePriority = 1.0f;

            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos =
            {
                vk::DeviceQueueCreateInfo({}, mGraphicsFamily, 1, &queuePriority),
                vk::DeviceQueueCreateInfo({}, mComputeFamily, 1, &queuePriority),
                vk::DeviceQueueCreateInfo({}, mTransferFamily, 1, &queuePriority)
            };
            vk::PhysicalDeviceFeatures deviceFeatures{};
            std::vector<const char*> extensions =
            {
                "VK_KHR_swapchain",
                "VK_KHR_dynamic_rendering"
            };
            vk::DeviceCreateInfo createInfo(
                {},
                static_cast<uint32>(queueCreateInfos.size()),
                queueCreateInfos.data(),
                0,
                nullptr,
                static_cast<uint32>(extensions.size()),
                extensions.data(),
                &deviceFeatures
            );
            mDevice = mPhysicalDevice.createDeviceUnique(createInfo);
        }

        void _createSurface(void* hinstance, void* hwnd)
        {
#ifdef ORC_PLATFORM_WIN32
            vk::Win32SurfaceCreateInfoKHR createInfo({}, reinterpret_cast<HINSTANCE>(hinstance), reinterpret_cast<HWND>(hwnd));
            mSurface = mInstance->createWin32SurfaceKHRUnique(createInfo);
#endif
        }

        void _createSwapChain(uint32 w, uint32 h)
        {
            vk::SwapchainCreateInfoKHR createInfo(
                {},
                mSurface.get(),
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

        ~VulkanGraphicsDevice() { mDevice->waitIdle(); }

        void acquireNextImage()
        {
            auto frameIndex = mDevice->acquireNextImageKHR(mSwapChain.get(), std::numeric_limits<uint64>::max(), mImageAvailableSemaphore.get());
            mFrameIndex = frameIndex.value;
        }

        void endDraw()
        {
        }

        void* getRawGraphicsDevice() const
        {
            return static_cast<VkDevice>(mDevice.get());
        }

        CommandList* createCommandList(CommandList::CommandListTypes type)
        {
            CommandList* list = nullptr;
            switch (type)
            {
            case CommandList::CommandListTypes::CLT_GRAPHICS:
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mGraphicsCommandPool.get()), type);
                break;
            case CommandList::CommandListTypes::CLT_COPY:
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mTransferCommandPool.get()), type);
                break;
            case CommandList::CommandListTypes::CLT_COMPUTE:
                list = createVulkanCommandList(this, static_cast<VkCommandPool>(mComputeCommandPool.get()), type);
                break;
            }
            return list;
        }

        void executeCommandList(CommandList::CommandListTypes type, uint32 numLists, CommandList* const* lists)
        {

        }

        int32 mGraphicsFamily = -1;
        int32 mComputeFamily = -1;
        int32 mTransferFamily = -1;

        uint32 mFrameIndex = 0;

        vk::UniqueInstance mInstance;
        vk::PhysicalDevice mPhysicalDevice;
        vk::UniqueDevice mDevice;
        vk::UniqueSurfaceKHR mSurface;
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

    GraphicsDevice* createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        auto props = SDL_GetWindowProperties((SDL_Window*)windowHandle);
        auto instance = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
        auto hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        return new VulkanGraphicsDevice(instance, hwnd, width, height);
    }
}
#endif