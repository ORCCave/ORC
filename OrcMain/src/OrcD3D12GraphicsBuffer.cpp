#ifdef ORC_PLATFORM_WIN32
#include "OrcD3D12Prerequisites.h"
#include "OrcGraphicsResource.h"

namespace Orc
{
    class D3D12GraphicsBuffer : public GraphicsBuffer
    {
    public:
        D3D12GraphicsBuffer()
        {
            //D3D12_HEAP_TYPE_UPLOAD
        }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
    };
}
#endif