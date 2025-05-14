#include "OrcGraphicsDevice.h"
#include "OrcRoot.h"
#include "OrcTypes.h"

#include <Windows.h>

#include <memory>

namespace Orc
{
    Root::Root(void* handle, uint32 w, uint32 h) : mWidthForSwapChain(w), mHeightForSwapChain(h)
    {
        HWND* hwndptr = static_cast<HWND*>(handle);
        mGraphicsDevice = std::make_shared<GraphicsDevice>(*hwndptr, mWidthForSwapChain, mHeightForSwapChain);
    }

    void Root::startRendering()
    {
        GraphicsDevice* realDevice = static_cast<GraphicsDevice*>(mGraphicsDevice.get());

        MSG msg = {};
        while (WM_QUIT != msg.message)
        {
            if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
            {
                realDevice->beginDraw();
                realDevice->endDraw();

            }
        }
    }
}