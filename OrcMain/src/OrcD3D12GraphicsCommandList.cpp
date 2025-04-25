#ifdef ORC_PLATFORM_WIN32
#include "OrcD3D12Prerequisites.h"

#include "OrcGraphicsCommandList.h"
#include "OrcGraphicsDevice.h"

#include <memory>

namespace Orc
{
    class D3D12GraphicsCommandList : public D3D12CommandList
    {
    public:
        D3D12GraphicsCommandList(GraphicsDevice* device, GraphicsCommandListType type) : D3D12CommandList(type)
        {
            auto d3d12Device = static_cast<ID3D12Device4*>(device->getRawGraphicsDevice());
            D3D12_COMMAND_LIST_TYPE d3d12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            switch (type)
            {
            case GraphicsCommandListType::GCLT_GRAPHICS:
                d3d12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
            case GraphicsCommandListType::GCLT_COPY:
                d3d12Type = D3D12_COMMAND_LIST_TYPE_COPY;
                break;
            case GraphicsCommandListType::GCLT_COMPUTE:
                d3d12Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;
            }
            CHECK_DX_RESULT(d3d12Device->CreateCommandAllocator(d3d12Type, IID_PPV_ARGS(&mCommandAllocator)));
            CHECK_DX_RESULT(d3d12Device->CreateCommandList1(0, d3d12Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCommandList)));
        }

        void begin()
        {
            CHECK_DX_RESULT(mCommandAllocator->Reset());
            CHECK_DX_RESULT(mCommandList->Reset(mCommandAllocator.Get(), nullptr));
        }

        void end()
        {
            CHECK_DX_RESULT(mCommandList->Close());
        }

        void* getRawCommandList() const
        {
            return mCommandList.Get();
        }
    private:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    };

    std::shared_ptr<GraphicsCommandList> createD3D12CommandList(GraphicsDevice* device, GraphicsCommandList::GraphicsCommandListType type)
    {
        return std::make_shared<D3D12GraphicsCommandList>(device, type);
    }
}
#endif