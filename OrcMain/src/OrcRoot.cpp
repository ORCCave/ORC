#include "OrcException.h"
#include "OrcGraphicsFactory.h"
#include "OrcRoot.h"

#include <SDL3/SDL_events.h>

namespace Orc
{
    GraphicsDevice* Root::createGraphicsDevice(GraphicsDevice::GraphicsDeviceTypes type)
    {
        GraphicsDevice* device = Orc::createGraphicsDevice(mWindowHandle, mWidthForSwapChain, mHeightForSwapChain, type);
        if (device == nullptr)
            throw OrcException("Invalid GraphicsDeviceType");
        return device;
    }

    void Root::destroyGraphicsDevice(GraphicsDevice* device)
    {
        delete device;
    }

    void Root::startRendering(GraphicsDevice* device)
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

            device->acquireNextImage();
            // todo

            device->present();
        }
    }
}