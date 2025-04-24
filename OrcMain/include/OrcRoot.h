#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsDevice.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

namespace Orc
{
    class ApplicationContext;
    class Root
    {
    public:
        GraphicsDevice* getGraphicsDevice();
        void startRendering();

        ORC_DISABLE_COPY_AND_MOVE(Root)
    protected:
        Root(void* handle, uint32 w, uint32 h, GraphicsDevice::GraphicsDeviceType type);

        ~Root() = default;

        void* mWindowHandle;

        uint32 mWidthForSwapChain;
        uint32 mHeightForSwapChain;

        GraphicsDevice::GraphicsDeviceType mGraphicsDeviceType;

        std::shared_ptr<GraphicsDevice> mDevice;

        friend class ApplicationContext;
    };
}