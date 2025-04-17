#include "OrcGraphicsFactory.h"
#include "OrcRoot.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL_events.h>

namespace Orc
{
    GraphicsDevice* Root::getGraphicsDevice(GraphicsDevice::GraphicsDeviceType type)
    {
        auto it = mDeviceCache.find(type);
        if (it != mDeviceCache.end())
        {
            return it->second.get();
        }

        auto device = createGraphicsDeviceByType(mWindowHandle, mWidthForSwapChain, mHeightForSwapChain, type);
        mDeviceCache[type] = device;
        return device.get();
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
            //device->clearSwapChainColor(0.0f, 1.0f, 0.0f, 1.0f);

            device->endDraw();
        }
    }
}