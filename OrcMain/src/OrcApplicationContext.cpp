#include "OrcApplicationContext.h"

#include "OrcException.h"
#include "OrcGraphicsDevice.h"
#include "OrcRoot.h"
#include "OrcTypes.h"

#include <SDL3/SDL.h>

#include <memory>
#include <string>

namespace Orc
{
    // For use std::unique_ptr to manage the lifetime of Root
    struct RootProxy : public Root
    {
        RootProxy(void* handle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type) : Root(handle, width, height, type) {}
    };

    std::shared_ptr<void> createWindow(const char* title, int w, int h, SDL_WindowFlags flags)
    {
        return std::shared_ptr<void>(SDL_CreateWindow(title, w, h, flags), [](void* ptr) {SDL_DestroyWindow(static_cast<SDL_Window*>(ptr));});
    }

    class ApplicationContext::impl
    {
    public:
        impl(void* handle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type) : mRoot(std::make_unique<RootProxy>(handle, width, height, type)) {}
        std::unique_ptr<RootProxy> mRoot;
    };

    ApplicationContext::ApplicationContext(const std::string& windowTitle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type)
        : mWindowTitle(windowTitle), mWidth(width), mHeight(height)
    {
        SDL_WindowFlags windowFlags = 0;
        if (type == GraphicsDevice::GraphicsDeviceType::GDT_VULKAN)
            windowFlags = SDL_WINDOW_VULKAN;
        if (!SDL_Init(SDL_INIT_VIDEO)) throw OrcException(SDL_GetError());
        mWindowHandle = createWindow(mWindowTitle.c_str(), mWidth, mHeight, windowFlags);
        if (!mWindowHandle)
        {
            SDL_Quit();
            throw OrcException(SDL_GetError());
        }
        mpimpl = std::make_unique<impl>(mWindowHandle.get(), mWidth, mHeight, type);
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