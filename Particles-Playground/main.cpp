#include "System/window.h"
#include "System/graphic.h"
#include "System/psomanager.h"
#include "System/commandlist.h"
#include "System/vertexformats.h"
#include "System/meshmanager.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    
    Window::Get().Startup();
    Graphic::Get().Startup();
    PSOManager::Get().Startup();
    MeshManager::Get().Startup();


    MeshManager::Get().PostInit();
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

        PSOManager::Get().Bind(commandList, PSOType::Default);

        commandList->OMSetRenderTargets(1, &Graphic::Get().GetCurrentRenderTargetHandle(), false, nullptr);

        MeshManager::Get().Bind(commandList, MeshType::Square);
        const float offset = 0.2f;
        commandList->SetGraphicsRoot32BitConstants(0, 1, &offset, 0);
        const float green = 0.5f;
        commandList->SetGraphicsRoot32BitConstants(1, 1, &green, 0);

        commandList->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Window::Get().GetWidth()), static_cast<float>(Window::Get().GetHeight())));
        commandList->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));

        MeshManager::Get().Draw(commandList, MeshType::Square);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);

        commandList.Submit();

        Graphic::Get().PostUpdate();
    }

    Graphic::Get().GetCurrentFence()->Flush(QueueType::Direct);

    MeshManager::Get().Shutdown();
    PSOManager::Get().Shutdown();
    Graphic::Get().Shutdown();
    Window::Get().Shutdown();

}
