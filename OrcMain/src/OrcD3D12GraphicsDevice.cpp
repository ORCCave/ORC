#ifdef ORC_PLATFORM_WIN32
#include "OrcD3D12Prerequisites.h"

#include <SDL3/SDL.h>

namespace Orc
{
    class D3D12GraphicsDevice : public GraphicsDevice
    {
    public:
        D3D12GraphicsDevice(HWND hwnd, uint32 width, uint32 height)
        {
            if(mHD3D12 == NULL)
                mHD3D12 = LoadLibraryW(L"d3d12.dll");
            if (mHDXGI == NULL)
                mHDXGI = LoadLibraryW(L"dxgi.dll");
            if (!mHD3D12 || !mHDXGI) { throw OrcException("Can't find D3D dll library"); }
#ifdef _DEBUG
            if (mHD3D12Debug == NULL)
                mHD3D12Debug = LoadLibraryW(L"D3D12SDKLayers.dll");
            if (mHDXGIDebug == NULL)
                mHDXGIDebug = LoadLibraryW(L"dxgidebug.dll");
#endif
            _createDevice();
            _createQueue();
            _createSwapChain(hwnd, width, height);
            mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
            CHECK_DX_RESULT(mDevice->CreateFence(mFenceValue[mFrameIndex]++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
            mEvent.Attach(CreateEventW(nullptr, FALSE, FALSE, nullptr));
            CHECK_DX_RESULT(mDevice->CreateFence(mCopyFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCopyFence)));
            mCopyEvent.Attach(CreateEventW(nullptr, FALSE, FALSE, nullptr));
            CHECK_DX_RESULT(mDevice->CreateFence(mComputeFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence)));
            mComputeEvent.Attach(CreateEventW(nullptr, FALSE, FALSE, nullptr));

            _createRTV();

            for (uint32 i = 0;i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                mGraphicsList[i] = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS);
            }
            mComputeList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE);
            mCopyList = createCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_COPY);

            D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
            mEnhancedBarriersSupported = false;
            if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12))))
            {
                mEnhancedBarriersSupported = options12.EnhancedBarriersSupported;
            }

        }

        ~D3D12GraphicsDevice()
        {
            _wait(GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS);
            _wait(GraphicsCommandList::GraphicsCommandListType::GCLT_COPY);
            _wait(GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE);
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
                CHECK_DX_RESULT(pfnD3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController)));
                mDebugController->EnableDebugLayer();
            }

            if (mHDXGIDebug)
            {
                factoryFlag = DXGI_CREATE_FACTORY_DEBUG;
            }
#endif
            auto pfnCreateDXGIFactory2 = reinterpret_cast<decltype(&CreateDXGIFactory2)>(GetProcAddress(mHDXGI, "CreateDXGIFactory2"));
            if (!pfnCreateDXGIFactory2) { throw OrcException("Can't find CreateDXGIFactory2"); }
            CHECK_DX_RESULT(pfnCreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&mFactory)));
            CHECK_DX_RESULT(mFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter)));

            auto pfnD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(mHD3D12, "D3D12CreateDevice"));
            if (!pfnD3D12CreateDevice) { throw OrcException("Can't find D3D12CreateDevice"); }
            CHECK_DX_RESULT(pfnD3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));
        }

        void _createQueue()
        {
            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            CHECK_DX_RESULT(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mGraphicsQueue)));
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            CHECK_DX_RESULT(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyQueue)));
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            CHECK_DX_RESULT(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeQueue)));
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
            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
            CHECK_DX_RESULT(mFactory->CreateSwapChainForHwnd(mGraphicsQueue.Get(), hwnd, &scDesc, &fsSwapChainDesc, nullptr, &swapChain));
            CHECK_DX_RESULT(mFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));
            CHECK_DX_RESULT(swapChain.As(&mSwapChain));
        }

        void beginDraw()
        {
            mGraphicsList[mFrameIndex]->begin();
        }

        void endDraw()
        {
            mGraphicsList[mFrameIndex]->end();
            auto tempPtr = mGraphicsList[mFrameIndex].get();
            executeCommandList(GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS, 1, &tempPtr);
            CHECK_DX_RESULT(mSwapChain->Present(1, 0));
            moveToNextFrame();
        }

        void moveToNextFrame()
        {
            const uint64 currentFenceValue = mFenceValue[mFrameIndex];
            mGraphicsQueue->Signal(mFence.Get(), currentFenceValue);
            mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
            if (mFence->GetCompletedValue() < mFenceValue[mFrameIndex])
            {
                CHECK_DX_RESULT(mFence->SetEventOnCompletion(mFenceValue[mFrameIndex], mEvent.Get()));
                if (WaitForSingleObjectEx(mEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                {
                    throw OrcException("WaitForSingleObjectEx failed");
                }
            }
            mFenceValue[mFrameIndex] = currentFenceValue + 1;
        }

        std::shared_ptr<GraphicsCommandList> createCommandList(GraphicsCommandList::GraphicsCommandListType type)
        {
            return createD3D12CommandList(this, type);
        }

        void _wait(GraphicsCommandList::GraphicsCommandListType type)
        {
            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                CHECK_DX_RESULT(mGraphicsQueue->Signal(mFence.Get(), mFenceValue[mFrameIndex]));
                CHECK_DX_RESULT(mFence->SetEventOnCompletion(mFenceValue[mFrameIndex]++, mEvent.Get()));
                if (WaitForSingleObjectEx(mEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                {
                    throw OrcException("WaitForSingleObjectEx failed");
                }
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                mCopyQueue->Signal(mCopyFence.Get(), mCopyFenceValue);
                mCopyFence->SetEventOnCompletion(mCopyFenceValue++, mCopyEvent.Get());
                if (WaitForSingleObjectEx(mCopyEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                {
                    throw OrcException("WaitForSingleObjectEx failed");
                }
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                mComputeQueue->Signal(mComputeFence.Get(), mComputeFenceValue);
                mComputeFence->SetEventOnCompletion(mComputeFenceValue++, mComputeEvent.Get());
                if (WaitForSingleObjectEx(mComputeEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                {
                    throw OrcException("WaitForSingleObjectEx failed");
                }
                break;
            }
        }

        void executeCommandList(GraphicsCommandList::GraphicsCommandListType type, uint32 numLists, GraphicsCommandList* const* lists)
        {
            std::vector<ID3D12CommandList*> tempLists;
            for (uint32 i = 0; i < numLists; ++i)
            {
                auto rawList = lists[i]->getRawCommandList();
                tempLists.push_back(reinterpret_cast<ID3D12CommandList*>(rawList));
            }

            switch (type)
            {
            case GraphicsCommandList::GraphicsCommandListType::GCLT_GRAPHICS:
                mGraphicsQueue->ExecuteCommandLists(numLists, tempLists.data());
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COPY:
                mCopyQueue->ExecuteCommandLists(numLists, tempLists.data());
                break;
            case GraphicsCommandList::GraphicsCommandListType::GCLT_COMPUTE:
                mComputeQueue->ExecuteCommandLists(numLists, tempLists.data());
                break;
            }
        }

        void _createRTV()
        {
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = ORC_SWAPCHAIN_COUNT;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            CHECK_DX_RESULT(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
            mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
            {
                Microsoft::WRL::ComPtr<ID3D12Resource> renderTarget;
                CHECK_DX_RESULT(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget)));
                mDevice->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle);
                rtvHandle.ptr += mRtvDescriptorSize;
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE getCurrentRenderTargetView() const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += mFrameIndex * mRtvDescriptorSize;
            return handle;
        }

        void clearSwapChainColor(float r, float g, float b, float a)
        {
            auto rtvHandle = getCurrentRenderTargetView();
            float colorRGBA[4] = { r, g, b, a };
            static_cast<ID3D12GraphicsCommandList*>(mGraphicsList[mFrameIndex]->getRawCommandList())->ClearRenderTargetView(rtvHandle, colorRGBA, 0, nullptr);
        }

    private:
        Microsoft::WRL::ComPtr<IDXGIAdapter4> mAdapter;
        Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;
        Microsoft::WRL::ComPtr<IDXGIFactory7> mFactory;
        Microsoft::WRL::ComPtr<ID3D12Device4> mDevice;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mGraphicsQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCopyQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeQueue;

        Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;

        uint32 mFrameIndex = 0;

        uint64 mFenceValue[ORC_SWAPCHAIN_COUNT]{};
        Microsoft::WRL::ComPtr<ID3D12Fence1> mFence;
        Microsoft::WRL::Wrappers::Event mEvent;

        uint64 mCopyFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mCopyFence;
        Microsoft::WRL::Wrappers::Event mCopyEvent;
        uint64 mComputeFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mComputeFence;
        Microsoft::WRL::Wrappers::Event mComputeEvent;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

        std::shared_ptr<GraphicsCommandList> mGraphicsList[ORC_SWAPCHAIN_COUNT];
        std::shared_ptr<GraphicsCommandList> mComputeList;
        std::shared_ptr<GraphicsCommandList> mCopyList;

        inline static HMODULE mHD3D12 = NULL;
        inline static HMODULE mHDXGI = NULL;
        inline static HMODULE mHD3D12Debug = NULL;
        inline static HMODULE mHDXGIDebug = NULL;

        UINT mRtvDescriptorSize;
        bool mEnhancedBarriersSupported;
    };

    std::shared_ptr<GraphicsDevice> createD3D12GraphicsDevice(void* windowHandle, uint32 width, uint32 height)
    {
        auto props = SDL_GetWindowProperties(static_cast<SDL_Window*>(windowHandle));
        if (!props) { throw OrcException(SDL_GetError()); }
        auto hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (!hwnd) { throw OrcException(SDL_GetError()); }
        return std::make_shared<D3D12GraphicsDevice>((HWND)hwnd, width, height);
    }
}
#endif