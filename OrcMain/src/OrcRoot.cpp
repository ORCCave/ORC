#include "OrcGraphicsFactory.h"
#include "OrcRoot.h"

#include <SDL3/SDL_events.h>

#include <memory>

namespace Orc
{
    struct GraphicsCommandListManagerProxy : public GraphicsCommandListManager
    {
        GraphicsCommandListManagerProxy(std::shared_ptr<GraphicsDevice> device) : GraphicsCommandListManager(device) {}
    };

    Root::Root(void* handle, uint32 w, uint32 h, GraphicsDevice::GraphicsDeviceType type) :
        mWindowHandle(handle), mWidthForSwapChain(w), mHeightForSwapChain(h), mGraphicsDeviceType(type)
    {
        mDevice = createGraphicsDeviceByType(mWindowHandle, mWidthForSwapChain, mHeightForSwapChain, mGraphicsDeviceType);
    }

    GraphicsDevice* Root::getGraphicsDevice()
    {
        return mDevice.get();
    }

    GraphicsCommandListManager* Root::getGraphicsCommandListManager()
    {
        if (!mGCLManager)
        {
            mGCLManager = std::make_shared<GraphicsCommandListManagerProxy>(mDevice);
        }
        return mGCLManager.get();
    }

    void Root::startRendering()
    {
        SDL_Event event;
        bool running = true;

        while (running)
        {
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                    break;
                }
            }

            if (!running)
                break;
            mDevice->beginDraw();
            mDevice->endDraw();

            if (mGCLManager)
            {
                mGCLManager->releaseCommandList();
            }

        }
    }
}