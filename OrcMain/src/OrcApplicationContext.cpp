#include "OrcApplicationContext.h"

#include "OrcException.h"
#include "OrcGraphicsDevice.h"
#include "OrcRoot.h"
#include "OrcTypes.h"

#include <SDL2/SDL.h>

#include <memory>
#include <string>

namespace Orc
{
    // For use std::unique_ptr to manage the lifetime of Root
    struct RootProxy : public Root
    {
        RootProxy(void* handle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type) : Root(handle, width, height, type) {}
    };

    std::shared_ptr<void> createWindow(const char* title, int w, int h, uint32 flags)
    {
        return std::shared_ptr<void>(SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags), [](void* ptr) {SDL_DestroyWindow(static_cast<SDL_Window*>(ptr));});
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
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) throw OrcException(SDL_GetError());
        if (type == GraphicsDevice::GraphicsDeviceType::GDT_VULKAN)
        {
            mWindowHandle = createWindow(mWindowTitle.c_str(), mWidth, mHeight, SDL_WINDOW_VULKAN);
        }
        else
        {
            mWindowHandle = createWindow(mWindowTitle.c_str(), mWidth, mHeight, 0);
        }
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