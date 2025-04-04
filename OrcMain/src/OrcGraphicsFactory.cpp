#include "OrcD3D12Prerequisites.h"
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsFactory.h"

namespace Orc
{
    std::shared_ptr<GraphicsDevice> createGraphicsDeviceByType(void* windowHandle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type)
    {
        std::shared_ptr<GraphicsDevice> device;
        switch (type)
        {
        case GraphicsDevice::GraphicsDeviceType::GDT_D3D12:
            device = createD3D12GraphicsDevice(windowHandle, width, height);
            break;
        case GraphicsDevice::GraphicsDeviceType::GDT_VULKAN:
            device = createVulkanGraphicsDevice(windowHandle, width, height);
            break;
        }
        return device;
    }
}