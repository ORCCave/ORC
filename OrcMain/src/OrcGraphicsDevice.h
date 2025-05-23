#pragma once

#include "OrcPrerequisites.h"

#include "OrcCommandList.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    class GraphicsDevice
    {
    public:
        ID3D12Device4* getRawGraphicsDevice() const { return mDevice.Get(); }

        void beginDraw();
        void endDraw();
        GraphicsDevice(HWND hwnd, uint32 width, uint32 height);
        ~GraphicsDevice();

        uint32 getCurrentFrameIndex() const { return mFrameIndex; }

        std::shared_ptr<CommandListContext> createCommandListContext(CommandListType type);
        void executeCommandList(CommandListContext* context);
    private:
        void _createSwapChain(HWND hwnd, uint32 width, uint32 height);
        void _createRTV();
        void _moveToNextFrame();

        void _wait(CommandListType type);
        void _clearSwapChainColor(float r, float g, float b, float a);
        D3D12_CPU_DESCRIPTOR_HANDLE _getCurrentRenderTargetView() const;

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

        uint64 mGraphicsFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mGraphicsFence;
        Microsoft::WRL::Wrappers::Event mGraphicsEvent;
        uint64 mCopyFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mCopyFence;
        Microsoft::WRL::Wrappers::Event mCopyEvent;
        uint64 mComputeFenceValue = 0;
        Microsoft::WRL::ComPtr<ID3D12Fence1> mComputeFence;
        Microsoft::WRL::Wrappers::Event mComputeEvent;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

        uint32 mRtvDescriptorSize;

        ID3D12Resource* mSwapChainRes[ORC_SWAPCHAIN_COUNT]{};

        std::shared_ptr<CommandListContext> mGraphicsCommandList;
        std::shared_ptr<CommandListContext> mCopyCommandList;
        std::shared_ptr<CommandListContext> mComputeCommandList;

        inline static HMODULE mHD3D12Debug = NULL;
        inline static HMODULE mHDXGIDebug = NULL;
    };
}