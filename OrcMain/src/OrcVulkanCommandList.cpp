#ifdef ORC_USE_VULKAN
#include "OrcVulkanPrerequisites.h"

#include "OrcCommandList.h"

namespace Orc
{
    class VulkanCommandList : public CommandList
    {
    public:
        VulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type)
        {
            auto vulkanDevice = static_cast<VkDevice>(device->getRawGraphicsDevice());
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = cmdPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;
            CHECK_VK_RESULT(vkAllocateCommandBuffers(vulkanDevice, &allocInfo, &mCommandBuffer));
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

        VkCommandBuffer mCommandBuffer;
    };

    CommandList* createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type)
    {
        return new VulkanCommandList(device, cmdPool, type);
    }
}
#endif
