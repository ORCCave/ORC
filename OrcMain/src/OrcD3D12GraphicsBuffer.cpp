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
            
        }

        void* getRawGraphicsResource() const { return mResource.Get(); }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
    };

    class D3D12GraphicsTexture : public GraphicsTexture
    {
    public:
        D3D12GraphicsTexture()
        {
            
        }

        void* getRawGraphicsResource() const { return mResource.Get(); }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
    };
}
#endif