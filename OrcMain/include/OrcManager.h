#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsCommandList.h"

namespace Orc
{
    class GraphicsCommandListManager
    {
    public:
        GraphicsCommandList* getUsableCommandList(GraphicsCommandList::GraphicsCommandListType type);

        ORC_DISABLE_COPY_AND_MOVE(GraphicsCommandListManager)
    protected:
        GraphicsCommandListManager(void* device) : mDevice(device) {}
        ~GraphicsCommandListManager() = default;

        void* mDevice;
    };
}