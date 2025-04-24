#include "OrcManager.h"

#include <algorithm>
#include <memory>
#include <mutex>

namespace Orc
{
    GraphicsCommandList* GraphicsCommandListManager::getUsableCommandList(GraphicsCommandList::GraphicsCommandListType type)
    {
        std::lock_guard<std::mutex> lock(mLock);
        auto it = std::find_if(mLists.begin(), mLists.end(), [](const std::shared_ptr<GraphicsCommandList>& item) { return item->mIsUsable; });
        if (it == mLists.end())
        {
            auto list = mRefDevice->createCommandList(type);
            mLists.push_back(list);
            list->mIsUsable = false;
            return list.get();
        }
        it->get()->mIsUsable = false;
        return it->get();
    }

    void GraphicsCommandListManager::releaseCommandList()
    {

    }
}