#include "OrcGraphicsFactory.h"
#include "OrcRoot.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL_events.h>

namespace Orc
{
    Root::Root(void* handle, uint32 w, uint32 h, GraphicsDevice::GraphicsDeviceType type) :
        mWindowHandle(handle), mWidthForSwapChain(w), mHeightForSwapChain(h), mGraphicsDeviceType(type)
    {
        mDevice = createGraphicsDeviceByType(mWindowHandle, mWidthForSwapChain, mHeightForSwapChain, mGraphicsDeviceType);
    }

    GraphicsDevice* Root::getGraphicsDevice()
    {
        return mDevice.get();
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
            // todo
            //device->clearSwapChainColor(0.0f, 1.0f, 0.0f, 1.0f);

            mDevice->endDraw();
        }
    }
}