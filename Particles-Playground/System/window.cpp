#include "window.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool Window::Startup()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEX windowClass = {};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WindowProc;
    windowClass.hInstance = GetModuleHandle(0);
    windowClass.lpszClassName = L"ParticlesPlaygroundClassName";

    if (!RegisterClassEx(&windowClass)) { return false; }

    const DWORD style = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX;

    RECT windowsRect = {};
    windowsRect.right = mWidth;
    windowsRect.bottom = mHeight;
    AdjustWindowRect(&windowsRect, style, false);

    const int32_t windowWidth = windowsRect.right - windowsRect.left;
    const int32_t windowHeight = windowsRect.bottom - windowsRect.top;

    const std::wstring title = L"Particles Playground";
    mHandle = CreateWindowEx(0, windowClass.lpszClassName, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, 0, 0, GetModuleHandle(0), 0);

    if (!mHandle) { return false; }

    mIsRunning = true;
    return true;
}

bool Window::Shutdown()
{ 
    DestroyWindow(mHandle);

    return true;
}

void Window::Close()
{
    mIsRunning = false;
}

void Window::Show()
{
    ShowWindow(mHandle, SW_SHOW);
}

void Window::Update()
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        Window::Get().Close();
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
