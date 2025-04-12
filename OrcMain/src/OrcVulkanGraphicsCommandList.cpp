#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsCommandList.h"

namespace Orc
{
    class VulkanGraphicsCommandList : public GraphicsCommandList
    {
    public:
        VulkanGraphicsCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListType type)
            : mDevice(static_cast<VkDevice>(device->getRawGraphicsDevice())), mCmdPool(cmdPool)
        {
            vk::CommandBufferAllocateInfo allocInfo{
                mCmdPool,
                vk::CommandBufferLevel::ePrimary,
                1
            };
            mCommandBuffer = mDevice.allocateCommandBuffersUnique(allocInfo);
        }

        void begin()
        {
            mCommandBuffer[0]->reset();
            vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
            mCommandBuffer[0]->begin(beginInfo);
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
