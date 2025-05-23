#include "OrcCommandList.h"
#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

namespace Orc
{
    CommandListContext::CommandListContext(void* device, CommandListType type) : mType(type)
    {
        auto d3d12Device = static_cast<GraphicsDevice*>(device)->getRawGraphicsDevice();
        mDevice = device;
        D3D12_COMMAND_LIST_TYPE d3d12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        switch (type)
        {
        case CommandListType::CLT_GRAPHICS:
            d3d12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;
        case CommandListType::CLT_COPY:
            d3d12Type = D3D12_COMMAND_LIST_TYPE_COPY;
            break;
        case CommandListType::CLT_COMPUTE:
            d3d12Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;
        }

        for (uint32 i = 0;i < ORC_SWAPCHAIN_COUNT; ++i)
        {
            d3d12Device->CreateCommandAllocator(d3d12Type, IID_PPV_ARGS(&mCommandAllocator[i]));
            d3d12Device->CreateCommandList1(1, d3d12Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCommandList[i]));
        }
    }

    void CommandListContext::begin()
    {
        auto currentIndex = static_cast<GraphicsDevice*>(mDevice)->getCurrentFrameIndex();
        mCommandAllocator[currentIndex]->Reset();
        mCommandList[currentIndex]->Reset(mCommandAllocator[currentIndex].Get(), nullptr);
    }

    void CommandListContext::end()
    {
        auto currentIndex = static_cast<GraphicsDevice*>(mDevice)->getCurrentFrameIndex();
        mCommandList[currentIndex]->Close();
    }

    ID3D12GraphicsCommandList* CommandListContext::getRawCommandList() const
    {
        auto currentIndex = static_cast<GraphicsDevice*>(mDevice)->getCurrentFrameIndex();
        return mCommandList[currentIndex].Get();
    }
}