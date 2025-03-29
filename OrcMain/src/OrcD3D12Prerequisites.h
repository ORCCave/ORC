#pragma once

#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

#ifdef ORC_PLATFORM_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

#include <combaseapi.h>
#include <libloaderapi.h>

namespace Orc
{
    GraphicsDevice* createD3D12GraphicsDevice(void* hwnd, uint32 width, uint32 height);
    CommandList* createD3D12CommandList(GraphicsDevice* device, CommandList::CommandListTypes type);
}

#else
namespace Orc { inline GraphicsDevice* createD3D12GraphicsDevice(void* hwnd, uint32 width, uint32 height) { return nullptr; } }
#endif
