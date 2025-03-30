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
            CHECK_VK_RESULT(vkAllocateCommandBuffers(mDevice, &allocInfo, &mCommandBuffer));
        }

        ~VulkanCommandList()
        {
            vkFreeCommandBuffers(mDevice, mCmdPool, 1, &mCommandBuffer);
        }

        void begin()
        {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            CHECK_VK_RESULT(vkBeginCommandBuffer(mCommandBuffer, &beginInfo));
        }

        void end()
        {
            CHECK_VK_RESULT(vkEndCommandBuffer(mCommandBuffer));
        }

        void* getRawGraphicsCommandList() const
        {
            return mCommandBuffer;
        }

        VkDevice mDevice;
        VkCommandPool mCmdPool;
        VkCommandBuffer mCommandBuffer;
    };

    CommandList* createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type)
    {
        return new VulkanCommandList(device, cmdPool, type);
    }
}
#endif
