#pragma once

#include "OrcPrerequisites.h"

namespace Orc
{
    class GpuResource
    {
    public:
        GpuResource(Microsoft::WRL::ComPtr<ID3D12Resource> res) : mResource(res) {}

        ID3D12Resource* getRawGpuResource() const { return mResource.Get(); }
    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
    };
}