#include "OrcD3D12Prerequisites.h"
#ifdef ORC_PLATFORM_WIN32
#include "OrcSingleton.h"

#include <SDL3/SDL.h>

namespace Orc
{
    class D3D12GraphicsDevice : public GraphicsDevice, public Singleton<D3D12GraphicsDevice>
    {
    public:
        D3D12GraphicsDevice(HWND hwnd, uint32 width, uint32 height) : GraphicsDevice(GraphicsDeviceTypes::GDT_D3D12)
        {
            mHD3D12 = LoadLibraryW(L"d3d12.dll");
            mHDXGI = LoadLibraryW(L"dxgi.dll");
            mHD3D12Debug = LoadLibraryW(L"D3D12SDKLayers.dll");
            mHDXGIDebug = LoadLibraryW(L"dxgidebug.dll");
            _createDevice();
            _createQueue();
            _createSwapChain(hwnd, width, height);
            mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
            mDevice->CreateFence(mFenceValue[mFrameIndex]++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
            mEvent.Attach(CreateEventW(nullptr, FALSE, FALSE, nullptr));
            mDevice->CreateFence(mCopyFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCopyFence));
            mCopyEvent.Attach(CreateEventW(nullptr, FALSE, FALSE, nullptr));
            mDevice->CreateFence(mComputeFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence));
            mComputeEvent.Attach(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        }

        ~D3D12GraphicsDevice()
        {
            mAdapter.Reset();
            mDebugController.Reset();
            mFactory.Reset();
            mDevice.Reset();
            mGraphicsQueue.Reset();
            mCopyQueue.Reset();
            mComputeQueue.Reset();
            mSwapChain.Reset();
            mFence.Reset();
            mEvent.Close();
            mCopyFence.Reset();
            mCopyEvent.Close();
            mComputeFence.Reset();
            mComputeEvent.Close();
            FreeLibrary(mHD3D12);
            FreeLibrary(mHDXGI);
            if (mHD3D12Debug)
            {
                FreeLibrary(mHD3D12Debug);
            }
            if (mHDXGIDebug)
            {
                FreeLibrary(mHDXGIDebug);
            }
        }

        void* getRawGraphicsDevice() const
        {
            return mDevice.Get();
        }

        void _createDevice()
        {
            uint32 factoryFlag = 0;
#ifdef _DEBUG
            if (mHD3D12Debug)
            {
                auto pfnD3D12GetDebugInterface = reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(GetProcAddress(mHD3D12, "D3D12GetDebugInterface"));
                if (SUCCEEDED(pfnD3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController))))
                {
                    mDebugController->EnableDebugLayer();
                }
            }

            if (mHDXGIDebug)
            {
                factoryFlag = DXGI_CREATE_FACTORY_DEBUG;
            }
#endif
            auto pfnCreateDXGIFactory2 = reinterpret_cast<decltype(&CreateDXGIFactory2)>(GetProcAddress(mHDXGI, "CreateDXGIFactory2"));
            pfnCreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&mFactory));
            mFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter));

            auto pfnD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(mHD3D12, "D3D12CreateDevice"));
            pfnD3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));
        }

        void _createQueue()
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mGraphicsQueue));
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyQueue));
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeQueue));
        }

        void _createSwapChain(HWND hwnd, uint32 width, uint32 height)
        {
            DXGI_SWAP_CHAIN_DESC1 scDesc{};
            scDesc.BufferCount = 3;
            scDesc.Width = width;
            scDesc.Height = height;
            scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            scDesc.SampleDesc.Count = 1;
            scDesc.SampleDesc.Quality = 0;
            scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            scDesc.Scaling = DXGI_SCALING_STRETCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc{};
            fsSwapChainDesc.Windowed = TRUE;
            IDXGISwapChain1* swapChain;

            mFactory->CreateSwapChainForHwnd(mGraphicsQueue.Get(), hwnd, &scDesc, &fsSwapChainDesc, nullptr, &swapChain);
            mFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

            swapChain->QueryInterface(mSwapChain.ReleaseAndGetAddressOf());

            swapChain->Release();
        }

        void present()
        {
            mSwapChain->Present(1, 0);
            moveToNextFrame();
        }

        void moveToNextFrame()
        {
            const uint64 currentFenceValue = mFenceValue[mFrameIndex];
            mGraphicsQueue->Signal(mFence.Get(), currentFenceValue);
            mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
            if (mFence->GetCompletedValue() < mFenceValue[mFrameIndex])
            {
                mFence->SetEventOnCompletion(mFenceValue[mFrameIndex], mEvent.Get());
                WaitForSingleObjectEx(mEvent.Get(), INFINITE, FALSE);
            }
            mFenceValue[mFrameIndex] = currentFenceValue + 1;
        }

        CommandList* createCommandList(CommandList::CommandListTypes type)
        {
            return createD3D12CommandList(this, type);
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter;
        Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;
        Microsoft::WRL::ComPtr<IDXGIFactory7> mFactory;
        Microsoft::WRL::ComPtr<ID3D12Device4> mDevice;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mGraphicsQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCopyQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeQueue;

        HMODULE mHD3D12;
        HMODULE mHDXGI;

        HMODULE mHD3D12Debug;
        HMODULE mHDXGIDebug;

        Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;

        uint32 mFrameIndex = 0;

        uint64 mFenceValue[3]{};
        Microsoft::WRL::ComPtr<ID3D12Fence1> mFence;
        Microsoft::WRL::Wrappers::Event mEvent;

        uint64 mCopyFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mCopyFence;
        Microsoft::WRL::Wrappers::Event mCopyEvent;
        uint64 mComputeFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mComputeFence;
        Microsoft::WRL::Wrappers::Event mComputeEvent;
    };

    GraphicsDevice* createD3D12GraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        auto props = SDL_GetWindowProperties((SDL_Window*)windowHandle);
        auto hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        return new D3D12GraphicsDevice((HWND)hwnd, width, height);
    }
}
#endif