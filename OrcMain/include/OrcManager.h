#pragma once

#include "OrcGraphicsDevice.h"

#include <memory>
#include <vector>

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

        std::vector<std::shared_ptr<GraphicsCommandList>> mLists;
        std::shared_ptr<GraphicsDevice> mRefDevice;
    };
}