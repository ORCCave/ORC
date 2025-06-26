#include "OrcPrerequisites.h"

#include "OrcApplicationContext.h"
#include "OrcDetail.h"

#include <memory>

namespace Orc
{
    ApplicationContext::ApplicationContext(const std::wstring& windowTitle, uint32 width, uint32 height) : mWindowTitle(windowTitle),
        mWidth(width), mHeight(height)
    {
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = detail::wndProc;
        wcex.hInstance = (HINSTANCE)GetModuleHandleW(nullptr);
        wcex.hIcon = LoadIconW(wcex.hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"Orc";
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        RegisterClassExW(&wcex);

        auto stype = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX;
        RECT rc = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
        AdjustWindowRect(&rc, stype, FALSE);
        auto hwnd = CreateWindowExW(0, L"Orc", mWindowTitle.c_str(), stype, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wcex.hInstance, nullptr);
        ShowWindow(hwnd, SW_SHOWDEFAULT);

        mRoot = std::make_shared<detail::Root>(&hwnd, mWidth, mHeight);
    }

    Root* ApplicationContext::getRoot() const
    {
        return  static_cast<Root*>(mRoot.get());
    }
}