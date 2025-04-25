#include "OrcManager.h"

#include "OrcGraphicsDevice.h"

namespace Orc
{
    GraphicsCommandList* GraphicsCommandListManager::getUsableCommandList(GraphicsCommandList::GraphicsCommandListType type)
    {
        return static_cast<GraphicsDevice*>(mDevice)->_getCmdList(type);
    }
}