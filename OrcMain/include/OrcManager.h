#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsCommandList.h"

namespace Orc
{
    class GraphicsCommandListManager
    {
    public:
        virtual GraphicsCommandList& getCommandList(GraphicsCommandList::GraphicsCommandListType type) = 0;

        ORC_DISABLE_COPY_AND_MOVE(GraphicsCommandListManager)
    protected:
        GraphicsCommandListManager() = default;
        ~GraphicsCommandListManager() = default;
    };
}