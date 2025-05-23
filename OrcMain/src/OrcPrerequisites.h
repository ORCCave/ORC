#pragma once

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#include <Windows.h>

#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

#define ORC_SWAPCHAIN_COUNT 3