#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsCommandList.h"

namespace Orc
{
    class VulkanCommandList : public GraphicsCommandList
    {
    public:
        VulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListTypes type)
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

    std::shared_ptr<GraphicsCommandList> createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListTypes type)
    {
        return std::make_shared<VulkanCommandList>(device, cmdPool, type);
    }
}
#endif
