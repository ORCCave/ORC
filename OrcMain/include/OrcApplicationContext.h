#pragma once

#include "OrcDefines.h"
#include "OrcRoot.h"
#include "OrcTypes.h"

#include <memory>
#include <string>

namespace Orc
{
    class ApplicationContext
    {
    public:
        ApplicationContext(const std::wstring& windowTitle, uint32 width, uint32 height);
        ~ApplicationContext() {}

        Root* getRoot() const;

        ORC_DISABLE_COPY_AND_MOVE(ApplicationContext)
    private:
        std::wstring mWindowTitle;
        uint32 mWidth;
        uint32 mHeight;

        std::shared_ptr<void> mRoot;
    };
}