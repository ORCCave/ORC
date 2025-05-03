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
        VulkanGraphicsCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandListType type, bool isPrimary)
            : mDevice(static_cast<VkDevice>(device->getRawGraphicsDevice())), mCmdPool(cmdPool), VulkanCommandList(device, type)
        {
            mIsPrimary = isPrimary;
            vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary;
            if (!isPrimary)
            {
                level = vk::CommandBufferLevel::eSecondary;
            }
            vk::CommandBufferAllocateInfo allocInfo
            {
                mCmdPool,
                level,
                1
            };
            mCommandBuffer = mDevice.allocateCommandBuffersUnique(allocInfo);
        }

        void begin()
        {
            mCommandBuffer[0]->reset();
            vk::CommandBufferBeginInfo beginInfo{};
            vk::CommandBufferInheritanceInfo inheritanceInfo{};
            if (!mIsPrimary)
            {
                beginInfo.pInheritanceInfo = &inheritanceInfo;
            }
            mCommandBuffer[0]->begin(&beginInfo);
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
        bool mIsPrimary;
    };

    std::shared_ptr<VulkanCommandList> createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, GraphicsCommandList::GraphicsCommandListType type, bool isPrimary)
    {
        return std::make_shared<VulkanGraphicsCommandList>(device, cmdPool, type, isPrimary);
    }
}
#endif
