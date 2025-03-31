#pragma once

#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

namespace Orc
{
    GraphicsDevice* createGraphicsDevice(void* windowHandle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceTypes type);
}