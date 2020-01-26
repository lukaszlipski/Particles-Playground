#include "System/window.h"
#include "System/graphic.h"
#include "System/commandlist.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    
    Window::Get().Startup();
    Graphic::Get().Startup();

    Window::Get().Show();
    while (Window::Get().IsRunning())
    {
        Window::Get().Update();
        Graphic::Get().PreUpdate();

        CommandList commandList(QueueType::Direct);

        // Record commands
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        commandList->ClearRenderTargetView(Graphic::Get().GetCurrentRenderTargetHandle(), clearColor, 0, nullptr);
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);

        commandList.Submit();

        Graphic::Get().PostUpdate();
    }

    Graphic::Get().Shutdown();
    Window::Get().Shutdown();

}
