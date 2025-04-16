#include "OrcException.h"
#include "OrcGraphicsFactory.h"
#include "OrcRoot.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL_events.h>

namespace Orc
{
    GraphicsDevice* Root::getGraphicsDevice(GraphicsDevice::GraphicsDeviceType type)
    {
        static std::map<GraphicsDevice::GraphicsDeviceType, std::shared_ptr<GraphicsDevice>> deviceCache;

        auto it = deviceCache.find(type);
        if (it != deviceCache.end())
        {
            return it->second.get();
        }

        auto device = createGraphicsDeviceByType(mWindowHandle, mWidthForSwapChain, mHeightForSwapChain, type);
        deviceCache[type] = device;
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