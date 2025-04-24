#pragma once

#include "OrcGraphicsDevice.h"

#include <memory>
#include <mutex>
#include <vector>

namespace Orc
{
    class Root;
    class GraphicsCommandListManager
    {
    public:
        GraphicsCommandList* getUsableCommandList(GraphicsCommandList::GraphicsCommandListType type);

        ORC_DISABLE_COPY_AND_MOVE(GraphicsCommandListManager)
    protected:
        GraphicsCommandListManager(std::shared_ptr<GraphicsDevice> device) : mRefDevice(device) {}
        ~GraphicsCommandListManager() = default;

        void releaseCommandList();

        std::vector<std::shared_ptr<GraphicsCommandList>> mLists;
        std::shared_ptr<GraphicsDevice> mRefDevice;

        std::mutex mLock;

        friend Root;
    };
}