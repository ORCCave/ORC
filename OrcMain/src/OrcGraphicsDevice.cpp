#include "OrcCommandList.h"
#include "OrcException.h"
#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    GraphicsDevice::GraphicsDevice(HWND hwnd, uint32 width, uint32 height) :
        mEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr)),
        mGraphicsEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr)),
        mCopyEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr)),
        mComputeEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr))
    {
        uint32 factoryFlag = 0;
#ifdef _DEBUG
        if (mHD3D12Debug == NULL)
            mHD3D12Debug = LoadLibraryW(L"D3D12SDKLayers.dll");
        if (mHDXGIDebug == NULL)
            mHDXGIDebug = LoadLibraryW(L"dxgidebug.dll");
        if (mHD3D12Debug)
        {
            D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController));
            mDebugController->EnableDebugLayer();
        }
        if (mHDXGIDebug)
            factoryFlag = DXGI_CREATE_FACTORY_DEBUG;
#endif
        CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&mFactory));
        mFactory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&mAdapter));
        D3D12CreateDevice(mAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice));

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mGraphicsQueue));
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyQueue));
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeQueue));

        _createSwapChain(hwnd, width, height);

        mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
        _createRTV();

        mDevice->CreateFence(mFenceValue[mFrameIndex]++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
        mDevice->CreateFence(mGraphicsFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mGraphicsFence));
        mDevice->CreateFence(mCopyFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCopyFence));
        mDevice->CreateFence(mComputeFenceValue++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence));

        mGraphicsCommandList = createCommandListContext(CommandListType::CLT_GRAPHICS);
        mCopyCommandList = createCommandListContext(CommandListType::CLT_COPY);
        mComputeCommandList = createCommandListContext(CommandListType::CLT_COMPUTE);
    }

    GraphicsDevice::~GraphicsDevice()
    {
        _wait(CommandListType::CLT_GRAPHICS);
        _wait(CommandListType::CLT_COPY);
        _wait(CommandListType::CLT_COMPUTE);
    }

    void GraphicsDevice::_wait(CommandListType type)
    {
        switch (type)
        {
        case CommandListType::CLT_GRAPHICS:
            mGraphicsQueue->Signal(mGraphicsFence.Get(), mGraphicsFenceValue);
            mGraphicsFence->SetEventOnCompletion(mGraphicsFenceValue++, mGraphicsEvent.Get());
            if (WaitForSingleObjectEx(mGraphicsEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
            break;
        case CommandListType::CLT_COPY:
            mCopyQueue->Signal(mCopyFence.Get(), mCopyFenceValue);
            mCopyFence->SetEventOnCompletion(mCopyFenceValue++, mCopyEvent.Get());
            if (WaitForSingleObjectEx(mCopyEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
            break;
        case CommandListType::CLT_COMPUTE:
            mComputeQueue->Signal(mComputeFence.Get(), mComputeFenceValue);
            mComputeFence->SetEventOnCompletion(mComputeFenceValue++, mComputeEvent.Get());
            if (WaitForSingleObjectEx(mComputeEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
            break;
        }
    }

    void GraphicsDevice::_createSwapChain(HWND hwnd, uint32 width, uint32 height)
    {
        DXGI_SWAP_CHAIN_DESC1 scDesc{};
        scDesc.BufferCount = ORC_SWAPCHAIN_COUNT;
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
        mFactory->CreateSwapChainForHwnd(mGraphicsQueue.Get(), hwnd, &scDesc, &fsSwapChainDesc, nullptr, &swapChain);
        mFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
        swapChain.As(&mSwapChain);
    }

    void GraphicsDevice::_createRTV()
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = ORC_SWAPCHAIN_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap));
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
        mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (uint32 i = 0; i < ORC_SWAPCHAIN_COUNT; ++i)
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> renderTarget;
            mSwapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget));
            mDevice->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle);
            rtvHandle.ptr += mRtvDescriptorSize;
            mSwapChainRes[i] = renderTarget.Get();
        }
    }

    std::shared_ptr<CommandListContext> GraphicsDevice::createCommandListContext(CommandListType type)
    {
        return std::make_shared<CommandListContext>(this, type);
    }

    void GraphicsDevice::executeCommandListContext(CommandListContext* context)
    {
        auto tempList = static_cast<ID3D12CommandList*>(context->getRawCommandList());
        auto type = context->getCommandListType();
        ID3D12CommandList* tempLists[1] = { tempList };
        switch (type)
        {
        case CommandListType::CLT_GRAPHICS:
            mGraphicsQueue->ExecuteCommandLists(1, tempLists);
            break;
        case CommandListType::CLT_COPY:
            mCopyQueue->ExecuteCommandLists(1, tempLists);
            break;
        case CommandListType::CLT_COMPUTE:
            mComputeQueue->ExecuteCommandLists(1, tempLists);
            break;
        }
    }

    void GraphicsDevice::_moveToNextFrame()
    {
        const uint64 currentFenceValue = mFenceValue[mFrameIndex];
        mGraphicsQueue->Signal(mFence.Get(), currentFenceValue);
        mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
        if (mFence->GetCompletedValue() < mFenceValue[mFrameIndex])
        {
            mFence->SetEventOnCompletion(mFenceValue[mFrameIndex], mEvent.Get());
            if (WaitForSingleObjectEx(mEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
        }
        mFenceValue[mFrameIndex] = currentFenceValue + 1;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::_getCurrentRenderTargetView() const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += mFrameIndex * mRtvDescriptorSize;
        return handle;
    }

    void GraphicsDevice::_clearSwapChainColor(float r, float g, float b, float a)
    {
        auto rtvHandle = _getCurrentRenderTargetView();
        float colorRGBA[4] = { r, g, b, a };
        mGraphicsCommandList->getRawCommandList()->ClearRenderTargetView(rtvHandle, colorRGBA, 0, nullptr);
    }

    void GraphicsDevice::beginDraw()
    {
        mGraphicsCommandList->begin();
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = mSwapChainRes[mFrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        mGraphicsCommandList->getRawCommandList()->ResourceBarrier(1, &barrier);
        _clearSwapChainColor(0, 0, 0, 1);
    }

    void GraphicsDevice::endDraw()
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = mSwapChainRes[mFrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        mGraphicsCommandList->getRawCommandList()->ResourceBarrier(1, &barrier);

        mGraphicsCommandList->end();
        executeCommandListContext(mGraphicsCommandList.get());

        mGraphicsQueue->Signal(mGraphicsFence.Get(), mGraphicsFenceValue++);
        mCopyQueue->Signal(mCopyFence.Get(), mCopyFenceValue++);
        mComputeQueue->Signal(mComputeFence.Get(), mComputeFenceValue++);

        mSwapChain->Present(1, 0);
        _moveToNextFrame();

        if (mGraphicsFence->GetCompletedValue() < mGraphicsFenceValue - (ORC_SWAPCHAIN_COUNT - 1))
        {
            mGraphicsFence->SetEventOnCompletion(mGraphicsFenceValue - (ORC_SWAPCHAIN_COUNT - 1), mGraphicsEvent.Get());
            if (WaitForSingleObjectEx(mGraphicsEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
        }
        if (mCopyFence->GetCompletedValue() < mCopyFenceValue - (ORC_SWAPCHAIN_COUNT - 1))
        {
            mCopyFence->SetEventOnCompletion(mCopyFenceValue - (ORC_SWAPCHAIN_COUNT - 1), mCopyEvent.Get());
            if (WaitForSingleObjectEx(mCopyEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
        }
        if (mComputeFence->GetCompletedValue() < mComputeFenceValue - (ORC_SWAPCHAIN_COUNT - 1))
        {
            mComputeFence->SetEventOnCompletion(mComputeFenceValue - (ORC_SWAPCHAIN_COUNT - 1), mComputeEvent.Get());
            if (WaitForSingleObjectEx(mComputeEvent.Get(), INFINITE, FALSE) == WAIT_FAILED)
                throw OrcException("Fail to call WaitForSingleObjectEx");
        }
    }
}