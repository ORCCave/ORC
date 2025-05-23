#include "OrcEntity.h"
#include "OrcGpuResource.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    Entity::Entity(const String entName) : mName(entName)
    {
        //mGpuVerticesData = std::make_shared<GpuResource>(nullptr);
    }
}