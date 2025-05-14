#pragma once

#include "OrcDefines.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    class Root
    {
    public:
        void startRendering();

        ORC_DISABLE_COPY_AND_MOVE(Root)
    protected:
        Root(void* handle, uint32 w, uint32 h);

        ~Root() = default;

        uint32 mWidthForSwapChain;
        uint32 mHeightForSwapChain;

        std::shared_ptr<void> mGraphicsDevice;
    };
}