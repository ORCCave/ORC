#pragma once

#include "OrcPrerequisites.h"
#include "OrcRoot.h"

namespace Orc
{
	namespace detail
	{
        LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
        {
            LRESULT result;
            switch (message)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                result = 0;
                break;
            default:
                result = DefWindowProcW(hwnd, message, wparam, lparam);
                break;
            }
            return result;
        }

        struct Root : public Orc::Root
        {
            Root(void* handle, uint32 width, uint32 height) : Orc::Root(handle, width, height) {}
        };
	}
} 