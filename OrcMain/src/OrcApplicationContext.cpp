#include "OrcApplicationContext.h"
#include "OrcException.h"
#include "OrcStdHeaders.h"

#include <SDL3/SDL.h>

namespace Orc
{
    // For use std::unique_ptr to manage the lifetime of Root
    class InternalRoot : public Root
    {
    public:
        InternalRoot(void* handle, uint32 width, uint32 height) : Root(handle, width, height) {}
        ~InternalRoot() = default;
    };

    std::shared_ptr<void> createWindow(const char* title, int w, int h, SDL_WindowFlags flags)
    {
        SDL_Window* window = SDL_CreateWindow(title, w, h, flags);
        return std::shared_ptr<void>(window, [](void* ptr) {
            SDL_DestroyWindow(static_cast<SDL_Window*>(ptr));
        });
    }

    class ApplicationContext::impl
    {
    public:
        impl(void* handle, uint32 width, uint32 height) : mRoot(std::make_unique<InternalRoot>(handle, width, height)) {}
        std::unique_ptr<InternalRoot> mRoot;
    };

    ApplicationContext::ApplicationContext(const std::string& windowTitle, uint32 width, uint32 height)
        : mWindowTitle(windowTitle), mWidth(width), mHeight(height)
    {
        SDL_WindowFlags windowFlags = 0;
#ifdef ORC_PLATFORM_LINUX
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");
            if (!SDL_Init(SDL_INIT_VIDEO)) { throw OrcException(SDL_GetError()); }
        }
        windowFlags = SDL_WINDOW_VULKAN;
#else
        if (!SDL_Init(SDL_INIT_VIDEO)) { throw OrcException(SDL_GetError()); }
#endif
        mWindowHandle = createWindow(mWindowTitle.c_str(), mWidth, mHeight, windowFlags);
        if (!mWindowHandle)
        {
            SDL_Quit();
            throw OrcException(SDL_GetError());
        }
        mpimpl = std::make_unique<impl>(mWindowHandle.get(), mWidth, mHeight);
    }

    ApplicationContext::~ApplicationContext()
    {
        mpimpl.reset();
        mWindowHandle.reset();
        SDL_Quit();
    }

    Root* ApplicationContext::getRoot() const
    {
        return mpimpl ? mpimpl->mRoot.get() : nullptr;
    }
}