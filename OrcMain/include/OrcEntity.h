#pragma once

#include "OrcDefines.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    class Entity
    {
    public:

        ORC_DISABLE_COPY_AND_MOVE(Entity)
    protected:
        Entity(const String entName);
        ~Entity() {}

        void setVerticesData(std::shared_ptr<void> verticesData)
        {
            mGpuVerticesData = verticesData;
        }

        void setIndicesData(std::shared_ptr<void> indicesData)
        {
            mGpuIndicesData = indicesData;
        }

        String mName;

        std::shared_ptr<void> mGpuVerticesData;
        std::shared_ptr<void> mGpuIndicesData;
    };
}