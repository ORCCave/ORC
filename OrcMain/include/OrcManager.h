#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsCommandList.h"
#include "OrcGraphicsDevice.h"
#include "OrcStdHeaders.h"

namespace Orc
{
    class GraphicsCommandListManager
    {
    public:
        virtual GraphicsCommandList& getCommandList(GraphicsCommandList::GraphicsCommandListType type) = 0;

        ORC_DISABLE_COPY_AND_MOVE(GraphicsCommandListManager)
    protected:
        GraphicsCommandListManager(std::shared_ptr<GraphicsDevice> device) : mRefDevice(device) {}
        ~GraphicsCommandListManager() = default;

        std::shared_ptr<GraphicsDevice> mRefDevice;
    };
}