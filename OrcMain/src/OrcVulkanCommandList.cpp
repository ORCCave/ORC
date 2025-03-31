#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcCommandList.h"

namespace Orc
{
    class VulkanCommandList : public CommandList
    {
    public:
        VulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type)
            : mDevice(static_cast<VkDevice>(device->getRawGraphicsDevice())), mCmdPool(cmdPool)
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = mCmdPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;
            vk::CommandBufferAllocateInfo createInfo(allocInfo);
            vk::Device wrapDevice(mDevice);
            vk::CommandPool cmdPool(mCmdPool);
            std::vector<vk::UniqueCommandBuffer> mCommandBuffer;
            mCommandBuffer = wrapDevice.allocateCommandBuffersUnique(createInfo);
        }

        void begin()
        {
            VkCommandBufferBeginInfo rawBeginInfo = {};
            rawBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            rawBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            vk::CommandBufferBeginInfo beginInfo(rawBeginInfo);
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

        VkDevice mDevice;
        VkCommandPool mCmdPool;
        std::vector<vk::UniqueCommandBuffer> mCommandBuffer;
    };

    CommandList* createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type)
    {
        return new VulkanCommandList(device, cmdPool, type);
    }
}
#endif
