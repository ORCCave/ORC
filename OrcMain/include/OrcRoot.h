#pragma once

#include "OrcManager.h"
#include "OrcTypes.h"

#include <memory>

namespace Orc
{
    class Root
    {
    public:
        GraphicsDevice* getGraphicsDevice();
        GraphicsCommandListManager* getGraphicsCommandListManager();

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
        std::shared_ptr<GraphicsCommandListManager> mGCLManager;
    };
}