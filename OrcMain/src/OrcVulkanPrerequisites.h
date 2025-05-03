#pragma once

#ifdef ORC_USE_VULKAN

#include "OrcGraphicsPrerequisites.h"

#include "OrcGraphicsCommandList.h"
#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

#ifdef ORC_PLATFORM_WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <thread>

namespace Orc
{
    class VulkanCommandList : public GraphicsCommandList
    {
    public:
        VulkanCommandList(GraphicsDevice* device,GraphicsCommandListType type) : GraphicsCommandList(type)
        {
            vk::Device realdevice = static_cast<VkDevice>(device->getRawGraphicsDevice());
            mFence = realdevice.createFenceUnique(vk::FenceCreateInfo());
            mThreadId = std::this_thread::get_id();
        }
        vk::UniqueFence mFence;
        std::thread::id mThreadId;
    };

    std::shared_ptr<GraphicsDevice> createVulkanGraphicsDevice(void* windowHandle, uint32 width, uint32 height);
    std::shared_ptr<GraphicsCommandList> createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListType type, bool isPrimary = true);
}
#endif
