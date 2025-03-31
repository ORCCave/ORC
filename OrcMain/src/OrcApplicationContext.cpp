#include "OrcApplicationContext.h"
#include "OrcException.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL.h>

namespace Orc
{
    class ApplicationContext::impl
    {
    public:
        impl(void* handle, uint32 width, uint32 height) : mRoot(new Root(handle, width, height)) {}
        ~impl() { delete mRoot; }
        Root* mRoot;
    };

    ApplicationContext::ApplicationContext(const std::string& windowTitle, uint32 width, uint32 height)
        : mWindowTitle(windowTitle), mWidth(width), mHeight(height)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            throw OrcException("SDL_Init failed");
        }
        mWindowHandle = SDL_CreateWindow(mWindowTitle.c_str(), mWidth, mHeight, 0);
        if (!mWindowHandle)
        {
            SDL_Quit();
            throw OrcException("SDL_CreateWindow failed");
        }
        mpimpl = std::make_unique<impl>(mWindowHandle, mWidth, mHeight);
    }

    ApplicationContext::~ApplicationContext()
    {
        mpimpl.reset();
        SDL_DestroyWindow(static_cast<SDL_Window*>(mWindowHandle));
        SDL_Quit();
    }

    Root* ApplicationContext::getRoot() const
    {
        return mpimpl ? mpimpl->mRoot : nullptr;
    }
}