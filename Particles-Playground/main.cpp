#include "system/window.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    
    Window::Get().Startup();

    Window::Get().Show();
    while (Window::Get().IsRunning())
    {
        Window::Get().Update();


    }

    Window::Get().Shutdown();

}
