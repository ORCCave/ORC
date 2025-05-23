#pragma once

#include "OrcPrerequisites.h"

namespace Orc
{
    enum class CommandListType
    {
        CLT_GRAPHICS,
        CLT_COPY,
        CLT_COMPUTE,
    };

    class CommandListContext
    {
    public:
        void begin();
        void end();
        ID3D12GraphicsCommandList* getRawCommandList() const;

        CommandListContext(void* device, CommandListType type);
        ~CommandListContext() = default;

        CommandListType getCommandListType() const { return mType; }
    private:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList[ORC_SWAPCHAIN_COUNT];
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator[ORC_SWAPCHAIN_COUNT];
        void* mDevice;
        CommandListType mType;
    };
}