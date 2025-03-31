#pragma once

#include "OrcDefines.h"
#include "OrcException.h"

namespace Orc
{
    template <typename T>
    class Singleton
    {
    public:
        static T* getSingleton()
        {
            return msSingleton;
        }

        ORC_DISABLE_COPY_AND_MOVE(Singleton)
    protected:
        Singleton()
        {
            if (++msInstanceCount > 1)
            {
                throw OrcException("There can be only one singleton");
            }

            msSingleton = static_cast<T*>(this);
        }
        ~Singleton()
        {
            --msInstanceCount;
            msSingleton = nullptr;
        }

        inline static int msInstanceCount = 0;
        inline static T* msSingleton = nullptr;
    };
}