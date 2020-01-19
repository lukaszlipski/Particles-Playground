#include "System/window.h"
#include "System/graphic.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    
    Window::Get().Startup();
    Graphic::Get().Startup();

    Window::Get().Show();
    while (Window::Get().IsRunning())
    {
        Window::Get().Update();


    }

    Graphic::Get().Shutdown();
    Window::Get().Shutdown();

}
