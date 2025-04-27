#pragma once

#ifdef ORC_PLATFORM_WIN32

//------------------------------------------------------------------------------
// Windows Platform Definitions
//------------------------------------------------------------------------------
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <combaseapi.h>
#include <comdef.h>
#include <concurrent_queue.h>
#include <libloaderapi.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

//------------------------------------------------------------------------------
// DirectX and WRL Headers
//------------------------------------------------------------------------------
#include "directx/d3dx12.h"
#include "directx/dxgiformat.h"

#include <dxgi1_6.h>

//------------------------------------------------------------------------------
// Project Prerequisites
//------------------------------------------------------------------------------
#include "OrcGraphicsPrerequisites.h"

#include "OrcException.h"
#include "OrcGraphicsCommandList.h"
#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

#include <format>
#include <limits>
#include <memory>
#include <string>

namespace Orc
{
    inline void CHECK_DX_RESULT(HRESULT hr)
    {
        if (FAILED(hr))
        {
#ifdef _DEBUG
            std::string s = std::format("(0x{:08x}) ", static_cast<unsigned int>(hr));
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            throw OrcException(s + errMsg);
#endif
        }
    }

    class D3D12CommandList : public GraphicsCommandList
    {
    public:
        D3D12CommandList(GraphicsCommandListType type) : GraphicsCommandList(type) {}
        uint64 mValue = std::numeric_limits<uint64>::max();
    };

    std::shared_ptr<GraphicsDevice> createD3D12GraphicsDevice(void* hwnd, uint32 width, uint32 height);
    std::shared_ptr<GraphicsCommandList> createD3D12CommandList(GraphicsDevice* device, GraphicsCommandList::GraphicsCommandListType type);
}

#endif
