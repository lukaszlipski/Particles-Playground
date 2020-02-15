#include "System/window.h"
#include "System/graphic.h"
#include "System/psomanager.h"
#include "System/commandlist.h"
#include "System/vertexformats.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    
    Window::Get().Startup();
    Graphic::Get().Startup();
    PSOManager::Get().Startup();

    DefaultVertex vertices[] =
    {
        { { 0.0f, 0.5f, 0.0f }, {0.f,0.f} },
        { { 0.5f, -0.5f, 0.0f }, {0.f,0.f} },
        { { -0.5f, -0.5f, 0.0f }, {0.f,0.f} }
    };

    // --- Upload vertex data
    ID3D12Resource* vertexBuffer = nullptr;
    ID3D12Resource* stageBuff = nullptr;

    Graphic::Get().GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));
    Graphic::Get().GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&stageBuff));

    D3D12_SUBRESOURCE_DATA data{};
    data.pData = vertices;
    data.RowPitch = sizeof(vertices);
    data.SlicePitch = data.RowPitch;

    Fence copyFence;
    {
        CommandList commandList(QueueType::Copy);
        UpdateSubresources<1>(commandList.Get(), vertexBuffer, stageBuff, 0, 0, 1, &data);

        commandList.Submit();

        copyFence.Signal(QueueType::Copy);
        copyFence.Wait(QueueType::Direct);
    }
    {
        CommandList commandList(QueueType::Direct);
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        commandList->ResourceBarrier(1, &barrier);
        commandList.Submit();
    }

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(vertices);
    vertexBufferView.StrideInBytes = sizeof(DefaultVertex);

    copyFence.Flush(QueueType::Direct);
    // ---

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

        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        const float offset = 0.2f;
        commandList->SetGraphicsRoot32BitConstants(0, 1, &offset, 0);
        const float green = 0.5f;
        commandList->SetGraphicsRoot32BitConstants(1, 1, &green, 0);

        commandList->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Window::Get().GetWidth()), static_cast<float>(Window::Get().GetHeight())));
        commandList->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);

        commandList.Submit();

        Graphic::Get().PostUpdate();
    }

    Graphic::Get().GetCurrentFence()->Flush(QueueType::Direct);

    stageBuff->Release();
    vertexBuffer->Release();

    PSOManager::Get().Shutdown();
    Graphic::Get().Shutdown();
    Window::Get().Shutdown();

}
