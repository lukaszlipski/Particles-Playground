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
        Graphic::Get().PreUpdate();

        ID3D12GraphicsCommandList* commandList = nullptr;
        ID3D12CommandAllocator* allocator = Graphic::Get().GetCurrentCommandAllocator(QueueType::Direct);
        Graphic::Get().GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, nullptr, IID_PPV_ARGS(&commandList));

        // Record commands
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        commandList->ClearRenderTargetView(Graphic::Get().GetCurrentRenderTargetHandle(), clearColor, 0, nullptr);
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();

        // Submit work on the queue
        Graphic::Get().GetQueue(QueueType::Direct)->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList**>(&commandList));

        commandList->Release();

        Graphic::Get().PostUpdate();
    }

    Graphic::Get().Shutdown();
    Window::Get().Shutdown();

}
