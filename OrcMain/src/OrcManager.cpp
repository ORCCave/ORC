#include "OrcEntity.h"
#include "OrcManager.h"
#include "OrcTypes.h"

#include <memory>
#include <vector>

namespace Orc
{
    namespace detail
    {
        class _Entity : public Entity
        {
        public:
            _Entity(const String& name) : Entity(name) {}
        };
    }

    Entity* SceneManager::createEntity(const String& entityName, const String& filePath)
    {
        auto& ptr = mEntities.emplace_back(std::make_shared<detail::_Entity>(entityName));
        return ptr.get();
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