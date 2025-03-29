#ifdef ORC_PLATFORM_WIN32
#include "OrcD3D12Prerequisites.h"

#include "OrcCommandList.h"

namespace Orc
{
	class D3D12CommandList : public CommandList
	{
	public:

        D3D12CommandList(GraphicsDevice* device, CommandList::CommandListTypes type) : CommandList()
        {
            auto d3d12Device = static_cast<ID3D12Device4*>(device->getRawGraphicsDevice());
            D3D12_COMMAND_LIST_TYPE d3d12Type = D3D12_COMMAND_LIST_TYPE_NONE;
			switch (type)
			{
			case CommandListTypes::CLT_GRAPHICS:
				d3d12Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				break;
			case CommandListTypes::CLT_COPY:
                d3d12Type = D3D12_COMMAND_LIST_TYPE_COPY;
				break;
			case CommandListTypes::CLT_COMPUTE:
                d3d12Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
				break;
			}
            d3d12Device->CreateCommandAllocator(d3d12Type, IID_PPV_ARGS(&mCommandAllocator));
			d3d12Device->CreateCommandList1(0, d3d12Type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&mCommandList));
        }

		void begin()
		{
            mCommandAllocator->Reset();
            mCommandList->Reset(mCommandAllocator.Get(), nullptr);
		}
		void end()
		{
            mCommandList->Close();
		}

		void* getRawGraphicsCommandList() const
		{
			return mCommandList.Get();
		}

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	};

	CommandList* createD3D12CommandList(GraphicsDevice* device, CommandList::CommandListTypes type)
	{
		return new D3D12CommandList(device, type);
	}
}
#endif