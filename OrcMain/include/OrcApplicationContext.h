#pragma once

#include "OrcRoot.h"

#include <memory>
#include <string>

namespace Orc
{
    class ApplicationContext
    {
    public:
        ApplicationContext(const std::string& windowTitle, uint32 width, uint32 height, GraphicsDevice::GraphicsDeviceType type = GraphicsDevice::GraphicsDeviceType::GDT_VULKAN);
        ~ApplicationContext();

        Root* getRoot() const;

        ORC_DISABLE_COPY_AND_MOVE(ApplicationContext)
    private:
        class impl;
        std::unique_ptr<impl> mpimpl;

        std::shared_ptr<void> mWindowHandle;

        std::string mWindowTitle;
        uint32 mWidth;
        uint32 mHeight;
    };
}