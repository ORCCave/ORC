#include "OrcRoot.h"

#include "OrcGraphicsDevice.h"
#include "OrcGraphicsFactory.h"
#include "OrcManager.h"
#include "OrcTypes.h"

#include <SDL3/SDL_events.h>

#include <memory>

namespace Orc
{
    struct GraphicsCommandListManagerProxy : public GraphicsCommandListManager
    {
        GraphicsCommandListManagerProxy(GraphicsDevice* device) : GraphicsCommandListManager(device) {}
        ~GraphicsCommandListManagerProxy() {}
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
        if (mGCLManager == nullptr)
            mGCLManager = std::make_shared<GraphicsCommandListManagerProxy>(mDevice.get());
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

            mDevice->runGarbageCollection();
        }
    }
}