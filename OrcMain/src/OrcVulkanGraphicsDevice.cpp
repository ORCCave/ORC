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
            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "ORC";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "ORC";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_3;

            std::vector<const char*> extensions =
            {
                "VK_KHR_surface",
#ifdef ORC_PLATFORM_WIN32
                "VK_KHR_win32_surface"
#endif
            };

            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &mInstance));
        }

		void _createPhysicalDevice()
		{
            uint32 deviceCount = 0;
            CHECK_VK_RESULT(vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr));
            if (deviceCount == 0)
            {
                throw OrcException("No physical device found");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            CHECK_VK_RESULT(vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data()));

            mPhysicalDevice = VK_NULL_HANDLE;
            for (auto&& device : devices)
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(device, &properties);
                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    mPhysicalDevice = device;
                    break;
                }
            }
            if (mPhysicalDevice == VK_NULL_HANDLE)
            {
                mPhysicalDevice = devices[0];
            }
		}

        void _createDevice()
        {
            uint32 queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilies.data());

            for (uint32 i = 0; i < queueFamilyCount; i++)
            {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    mGraphicsFamily = i;
                    break;
                }
            }

            if (mGraphicsFamily == -1)
            {
                throw OrcException("No graphics queue family found");
            }

            for (uint32 i = 0; i < queueFamilyCount; i++)
            {
                if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && i != mGraphicsFamily)
                {
                    mComputeFamily = i;
                    break;
                }
            }
            if (mComputeFamily == -1)
            {
                mComputeFamily = mGraphicsFamily;
            }

            for (uint32 i = 0; i < queueFamilyCount; i++)
            {
                if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && i != mGraphicsFamily && i != mComputeFamily)
                {
                    mTransferFamily = i;
                    break;
                }
            }

            if (mTransferFamily == -1)
            {
                if (queueFamilies[mGraphicsFamily].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    mTransferFamily = mGraphicsFamily;
                }
                else if (queueFamilies[mComputeFamily].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    mTransferFamily = mComputeFamily;
                }
                else
                {
                    throw OrcException("No transfer queue family found");
                }
            }

            float queuePriority = 1.0f;
            VkDeviceQueueCreateInfo queueCreateInfos[3];
            queueCreateInfos[0] = {};
            queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[0].queueFamilyIndex = mGraphicsFamily;
            queueCreateInfos[0].queueCount = 1;
            queueCreateInfos[0].pQueuePriorities = &queuePriority;
            queueCreateInfos[1] = {};
            queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[1].queueFamilyIndex = mComputeFamily;
            queueCreateInfos[1].queueCount = 1;
            queueCreateInfos[1].pQueuePriorities = &queuePriority;
            queueCreateInfos[2] = {};
            queueCreateInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[2].queueFamilyIndex = mTransferFamily;
            queueCreateInfos[2].queueCount = 1;
            queueCreateInfos[2].pQueuePriorities = &queuePriority;

            VkPhysicalDeviceFeatures deviceFeatures = {};
            VkDeviceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pQueueCreateInfos = queueCreateInfos;
            createInfo.queueCreateInfoCount = 3;
            createInfo.pEnabledFeatures = &deviceFeatures;
            std::vector<const char*> extensions = 
            {
                "VK_KHR_swapchain",
                "VK_KHR_dynamic_rendering"
            };
            createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            CHECK_VK_RESULT(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice));
        }

        void _createSurface(void* hinstance, void* hwnd)
        {
#ifdef ORC_PLATFORM_WIN32
            VkWin32SurfaceCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hinstance = (HINSTANCE)hinstance;
            createInfo.hwnd = (HWND)hwnd;
            CHECK_VK_RESULT(vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface));
#endif
        }


        void _createSwapChain(uint32 w, uint32 h)
        {
            VkSwapchainCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = mSurface;
            createInfo.minImageCount = 3;
            createInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
            // no gamma correction
            createInfo.imageColorSpace = VK_COLOR_SPACE_PASS_THROUGH_EXT;
            createInfo.imageExtent.width = w;
            createInfo.imageExtent.height = h;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
            createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;
            CHECK_VK_RESULT(vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain));
        }

        void _createQueue()
        {
            vkGetDeviceQueue(mDevice, mGraphicsFamily, 0, &mGraphicsQueue);
            vkGetDeviceQueue(mDevice, mComputeFamily, 0, &mComputeQueue);
            vkGetDeviceQueue(mDevice, mTransferFamily, 0, &mTransferQueue);
        }

		void _createCommandPool()
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = mGraphicsFamily;
			CHECK_VK_RESULT(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mGraphicsCommandPool));
			poolInfo.queueFamilyIndex = mComputeFamily;
			CHECK_VK_RESULT(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mComputeCommandPool));
			poolInfo.queueFamilyIndex = mTransferFamily;
			CHECK_VK_RESULT(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mTransferCommandPool));
		}

		void _createCommandBuffer()
		{
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
            allocInfo.commandPool = mGraphicsCommandPool;
			CHECK_VK_RESULT(vkAllocateCommandBuffers(mDevice, &allocInfo, &mGraphicsCommandBuffer));
			allocInfo.commandPool = mComputeCommandPool;
			CHECK_VK_RESULT(vkAllocateCommandBuffers(mDevice, &allocInfo, &mComputeCommandBuffer));
			allocInfo.commandPool = mTransferCommandPool;
			CHECK_VK_RESULT(vkAllocateCommandBuffers(mDevice, &allocInfo, &mTransferCommandBuffer));
		}

        void _createSemaphore()
        {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = nullptr;
            semaphoreInfo.flags = 0;
            CHECK_VK_RESULT(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore));
            CHECK_VK_RESULT(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphore));
        }

        ~VulkanGraphicsDevice()
        {
            vkDeviceWaitIdle(mDevice);
			vkDestroySemaphore(mDevice, mImageAvailableSemaphore, nullptr);
			vkDestroySemaphore(mDevice, mRenderFinishedSemaphore, nullptr);
			vkDestroyCommandPool(mDevice, mGraphicsCommandPool, nullptr);
			vkDestroyCommandPool(mDevice, mComputeCommandPool, nullptr);
			vkDestroyCommandPool(mDevice, mTransferCommandPool, nullptr);
            vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
            vkDestroyDevice(mDevice, nullptr);
            vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
            vkDestroyInstance(mInstance, nullptr);
        }

        void present()
        {
            uint32 imageIndex;
            CHECK_VK_RESULT(vkAcquireNextImageKHR(mDevice, mSwapChain, std::numeric_limits<uint32>::max(), mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex));
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &mImageAvailableSemaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &mSwapChain;
            presentInfo.pImageIndices = &imageIndex;
			CHECK_VK_RESULT(vkQueuePresentKHR(mGraphicsQueue, &presentInfo));
        }

        void* getRawGraphicsDevice() const
        {
            return mDevice;
        }

        CommandList* createCommandList(CommandList::CommandListTypes type)
        {
            CommandList* list = nullptr;
            switch (type)
            {
            case CommandList::CommandListTypes::CLT_GRAPHICS:
                list = createVulkanCommandList(this, mGraphicsCommandPool, type);
                break;
            case CommandList::CommandListTypes::CLT_COPY:
                list = createVulkanCommandList(this, mTransferCommandPool, type);
                break;
            case CommandList::CommandListTypes::CLT_COMPUTE:
                list = createVulkanCommandList(this, mComputeCommandPool, type);
                break;
            }
            return list;
        }

        VkInstance mInstance;
        VkPhysicalDevice mPhysicalDevice;
        VkSurfaceKHR mSurface;
        VkSwapchainKHR mSwapChain;
        VkDevice mDevice;

        VkSemaphore mImageAvailableSemaphore;
        VkSemaphore mRenderFinishedSemaphore;

        VkQueue mGraphicsQueue;
        VkQueue mComputeQueue;
        VkQueue mTransferQueue;
        VkCommandPool mGraphicsCommandPool;
		VkCommandPool mComputeCommandPool;
		VkCommandPool mTransferCommandPool;
		VkCommandBuffer mGraphicsCommandBuffer;
		VkCommandBuffer mComputeCommandBuffer;
		VkCommandBuffer mTransferCommandBuffer;

        int32 mGraphicsFamily = -1;
        int32 mComputeFamily = -1;
        int32 mTransferFamily = -1;
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