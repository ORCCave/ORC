#pragma once

#include "OrcGraphicsDevice.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

#ifdef ORC_USE_VULKAN

#ifdef ORC_PLATFORM_WIN32

#define VK_USE_PLATFORM_WIN32_KHR

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
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
