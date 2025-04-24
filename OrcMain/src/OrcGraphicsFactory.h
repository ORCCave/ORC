#pragma once

#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    std::shared_ptr<GraphicsDevice> createGraphicsDeviceByType(void* windowHandle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type);
}