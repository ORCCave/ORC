#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsDevice.h"
#include "OrcTypes.h"

namespace Orc
{
    class ApplicationContext;
    class Root
    {
    public:
        GraphicsDevice* getGraphicsDevice(GraphicsDevice::GraphicsDeviceType type);
        void startRendering(GraphicsDevice* device);

        ORC_DISABLE_COPY_AND_MOVE(Root)
    protected:
        Root(void* handle, uint32 w, uint32 h) : 
            mWindowHandle(handle), mWidthForSwapChain(w), mHeightForSwapChain(h) {}

        ~Root() = default;

        void* mWindowHandle;

        uint32 mWidthForSwapChain;
        uint32 mHeightForSwapChain;

        friend class ApplicationContext;
    };
}