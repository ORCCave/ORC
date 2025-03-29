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
        }

        void begin()
        {
        }
        void end()
        {
        }
        void* getRawGraphicsCommandList() const
        {
            return nullptr;
        }
    };

    CommandList* createVulkanCommandList(GraphicsDevice* device, VkCommandPool cmdPool, CommandList::CommandListTypes type)
    {
        return new VulkanCommandList(device, cmdPool, type);
    }
}
#endif
