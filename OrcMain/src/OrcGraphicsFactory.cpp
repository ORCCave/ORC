#include "OrcD3D12Prerequisites.h"
#include "OrcVulkanPrerequisites.h"

#include "OrcGraphicsFactory.h"

#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    std::shared_ptr<GraphicsDevice> createGraphicsDeviceByType(void* windowHandle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type)
    {
        std::shared_ptr<GraphicsDevice> device;
        switch (type)
        {
#ifdef ORC_PLATFORM_WIN32
        case GraphicsDevice::GraphicsDeviceType::GDT_D3D12:
            device = Orc::createD3D12GraphicsDevice(windowHandle, width, height);
            break;
#endif
#ifdef ORC_USE_VULKAN
        case GraphicsDevice::GraphicsDeviceType::GDT_VULKAN:
            device = createVulkanGraphicsDevice(windowHandle, width, height);
            break;
#endif
        default:
            break;
        }
        return device;
    }
}