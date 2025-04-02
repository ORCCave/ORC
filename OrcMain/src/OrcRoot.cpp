#include "OrcException.h"
#include "OrcGraphicsFactory.h"
#include "OrcRoot.h"

#include <SDL3/SDL_events.h>

namespace Orc
{
    std::shared_ptr<GraphicsDevice> Root::createGraphicsDevice(GraphicsDevice::GraphicsDeviceTypes type)
    {
        if (auto device = createGraphicsDeviceByType(mWindowHandle, mWidthForSwapChain, mHeightForSwapChain, type))
            return device;
        throw OrcException("Invalid GraphicsDeviceType");
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

            device->beginDraw();
            // todo

            device->endDraw();
        }
    }
}