#include "OrcGraphicsDevice.h"
#include "OrcManager.h"
#include "OrcRoot.h"
#include "OrcTypes.h"

#include <Windows.h>

#include <memory>
#include <vector>

namespace Orc
{
    namespace detail
    {
        class _SceneManager : public SceneManager
        {
        public:
            _SceneManager(const String& name) : SceneManager(name) {}
        };
    }

    Root::Root(void* handle, uint32 w, uint32 h) : mWidthForSwapChain(w), mHeightForSwapChain(h)
    {
        HWND* hwndPtr = static_cast<HWND*>(handle);
        mGraphicsDevice = std::make_shared<GraphicsDevice>(*hwndPtr, mWidthForSwapChain, mHeightForSwapChain);
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

    SceneManager* Root::createSceneManager(const String& sceneManagerName)
    {
        auto& ptr = mSceneManagers.emplace_back(std::make_shared<detail::_SceneManager>(sceneManagerName));
        return ptr.get();
    }
}