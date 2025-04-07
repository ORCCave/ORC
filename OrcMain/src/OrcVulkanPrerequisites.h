#pragma once

#include "OrcGraphicsDevice.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

#ifdef ORC_USE_VULKAN
    #if defined(ORC_PLATFORM_WIN32)
        #define VK_USE_PLATFORM_WIN32_KHR
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
    #elif defined(ORC_PLATFORM_LINUX)
        #include <build_config/SDL_build_config.h>
        #if defined(SDL_VIDEO_DRIVER_WAYLAND)
            #define ORC_USE_WAYLAND
        #elif defined(SDL_VIDEO_DRIVER_X11)
            #define ORC_USE_X11
        #endif
        #if !defined(ORC_USE_WAYLAND) && !defined(ORC_USE_X11)
            #error "SDL3 must support at least one of Wayland or X11 on Linux."
        #endif
    #endif

    #include <vk_mem_alloc.h>
    #include <vulkan/vulkan.hpp>

    namespace Orc
    {
        std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height);
        std::shared_ptr<GraphicsCommandList> createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListType type);
    }
#else
    namespace Orc { inline std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height) { return std::shared_ptr<GraphicsDevice>(); } }
#endif
