#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsCommandList.h"

#include "OrcGraphicsDevice.h"

#include <memory>
#include <vector>

namespace Orc
{
    class VulkanGraphicsCommandList : public VulkanCommandList
    {
    public:
        VulkanGraphicsCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandListType type)
            : mDevice(static_cast<VkDevice>(device->getRawGraphicsDevice())), mCmdPool(cmdPool), VulkanCommandList(device, type)
        {
            vk::CommandBufferAllocateInfo allocInfo
            {
                mCmdPool,
                vk::CommandBufferLevel::ePrimary,
                1
            };
            mCommandBuffer = mDevice.allocateCommandBuffersUnique(allocInfo);
        }

        void begin()
        {
            mCommandBuffer[0]->reset();
            mCommandBuffer[0]->begin({});
        }

        void end()
        {
            mCommandBuffer[0]->end();
        }

        void* getRawCommandList() const
        {
            return static_cast<VkCommandBuffer>(mCommandBuffer[0].get());
        }

        vk::Device mDevice;
        vk::CommandPool mCmdPool;
        std::vector<vk::UniqueCommandBuffer> mCommandBuffer;
    };

    std::shared_ptr<GraphicsCommandList> createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListType type)
    {
        return std::make_shared<VulkanGraphicsCommandList>(device, cmdPool, type);
    }
}
#endif
