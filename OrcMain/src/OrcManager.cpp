#include "OrcManager.h"

namespace Orc
{
    Entity* SceneManager::createEntity(const String& entityName, const String& filePath)
    {
        return nullptr;
    }

    void SceneManager::destroyEntity(Entity* ent)
    {
        for (auto it = mEntities.begin(); it != mEntities.end(); ++it)
        {
            if (ent == it->get())
            {
                mEntities.erase(it);
                break;
            }
        }
    }
}