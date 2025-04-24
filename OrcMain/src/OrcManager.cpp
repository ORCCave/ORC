#include "OrcManager.h"

namespace Orc
{
    GraphicsCommandList* GraphicsCommandListManager::getUsableCommandList(GraphicsCommandList::GraphicsCommandListType type)
    {
        auto it = std::find_if(mLists.begin(), mLists.end(), [](const std::shared_ptr<GraphicsCommandList>& item) { return item->mIsUsable; });
        if (it == mLists.end())
        {
            auto list = mRefDevice->createCommandList(type);
            mLists.push_back(list);
            return list.get();
        }
        return it->get();
    }
}