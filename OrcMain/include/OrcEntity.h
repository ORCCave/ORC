#pragma once

#include "OrcDefines.h"
#include "OrcTypes.h"

namespace Orc
{
    class Entity
    {
    public:

        ORC_DISABLE_COPY_AND_MOVE(Entity)
    protected:
        Entity(const String entName) : mName(entName) {}
        ~Entity() {}

        String mName;
        friend class SceneManager;
    };
}