#pragma once

#include "OrcGraphicsDevice.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

namespace Orc
{
    std::shared_ptr<GraphicsDevice> createGraphicsDeviceByType(void* windowHandle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type);
}