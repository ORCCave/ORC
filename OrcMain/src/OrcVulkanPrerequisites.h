#pragma once

#include "OrcGraphicsDevice.h"
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
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include <vulkan/vulkan.hpp>

#include "OrcException.h"
#ifdef NDEBUG
#define CHECK_VK_RESULT(result)
#else
#define CHECK_VK_RESULT(result)                                 \
do                                                              \
{                                                               \
    VkResult res = (result);                                    \
    if (res != VK_SUCCESS)                                      \
    {                                                           \
        throw Orc::OrcException(Orc::VKResultToString(result)); \
    }                                                           \
} while (0)
#endif

namespace Orc
{
    inline const char* VKResultToString(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS: return "VK_SUCCESS";
        default: return "Unknown VkResult";
        }
    }

    GraphicsDevice* createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height);
    CommandList* createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type);
}
#else
namespace Orc { inline GraphicsDevice* createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height) { return nullptr; } }
#endif
