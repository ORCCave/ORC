#pragma once

#include "OrcDefines.h"
#include "OrcRoot.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

namespace Orc
{
    class ApplicationContext
    {
    public:
        ApplicationContext(const std::string& windowTitle, uint32 width, uint32 height);
        ~ApplicationContext() = default;
        void initApp();
        void closeApp();

        Root* getRoot() const;

        ORC_DISABLE_COPY_AND_MOVE(ApplicationContext)
    private:
        class impl;
        impl* pimpl;

        void* mWindowHandle;

		std::string mWindowTitle;
		uint32 mWidth;
		uint32 mHeight;
    };
}