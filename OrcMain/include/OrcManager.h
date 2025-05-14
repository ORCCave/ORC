#pragma once

#include "OrcDefines.h"
#include "OrcEntity.h"
#include "OrcTypes.h"

#include <memory>
#include <vector>

namespace Orc
{
    class SceneManager
    {
    public:
        Entity* createEntity(const String& entityName, const String& filePath);
        void destroyEntity(Entity* ent);
        ORC_DISABLE_COPY_AND_MOVE(SceneManager)
    protected:
        SceneManager(const String& sceneManagerName) : mName(sceneManagerName) {}
        ~SceneManager() {}

        String mName;
        std::vector<std::shared_ptr<Entity>> mEntities;
    };
}