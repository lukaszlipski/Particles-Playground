#include "System/engine.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    Engine::Get().Startup();
   
    uint32_t constantBufferCount = Graphic::Get().GetFrameCount();
    uint32_t constantBufferSize = (16 + 16) * sizeof(float);
    constantBufferSize = (constantBufferSize + 256) & ~255;

    std::unique_ptr<GPUBuffer> constantBuffer = std::make_unique<GPUBuffer>(constantBufferSize, constantBufferCount);

    // Create descriptor heap
    ID3D12DescriptorHeap* heap = nullptr;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = constantBufferCount;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Graphic::Get().GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap->GetCPUDescriptorHandleForHeapStart());

    // Create frames' constant buffer views
    for (uint32_t i = 0; i < constantBufferCount; ++i)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC buffDesc{};
        buffDesc.BufferLocation = constantBuffer->GetGPUAddress(i);
        buffDesc.SizeInBytes = constantBufferSize;

        Graphic::Get().GetDevice()->CreateConstantBufferView(&buffDesc, cpuHandle);

        cpuHandle.Offset(1, Graphic::Get().GetCBVHandleSize());
    }

    // Prepare a camera projection
    const float fov = XMConvertToRadians(90.0f);
    const float ratio = Window::Get().GetWidth() / static_cast<float>(Window::Get().GetHeight());
    const float nearDist = 1.0f;
    const float farDist = 100.0f;
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, ratio, nearDist, farDist);

    Engine::Get().PostStartup();
    
    while (Window::Get().IsRunning())
    {
        Engine::Get().PreUpdate();

        // Record commands
        uint32_t currentFrame = Graphic::Get().GetCurrentFrameIndex();
        CommandList commandList(QueueType::Direct);

        // Update camera constant buffer
        { 
            const float tmpXPos = XMScalarCos(Graphic::Get().GetCurrentFrameNumber() / 50.0f);
            const XMVECTOR camPos = { tmpXPos,0,-3,1 };
            const XMVECTOR camDir = { 0, 0, 1,0 };
            const XMVECTOR camUp = { 0, 1, 0,0 };
            const XMMATRIX view = XMMatrixLookAtLH(camPos, camPos + camDir, camUp);

            const uint32_t startOffset = constantBufferSize * currentFrame;
            const uint32_t endOffset = startOffset + constantBufferSize - 1;
            
            uint8_t* dstData = constantBuffer->Map(startOffset, endOffset);

            memcpy(dstData, &proj, 16 * sizeof(float));
            memcpy(dstData + (16 * sizeof(float)), &view, 16 * sizeof(float));

            constantBuffer->Unmap(commandList);
        }

        std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        commandList->ClearRenderTargetView(Graphic::Get().GetCurrentRenderTargetHandle(), clearColor, 0, nullptr);

        const PSOKey key{ PSOType::Default, MeshManager::Get().GetVertexFormatDescRef(MeshType::Square) };
        PSOManager::Get().Bind(commandList, key);

        commandList->OMSetRenderTargets(1, &Graphic::Get().GetCurrentRenderTargetHandle(), false, nullptr);

        MeshManager::Get().Bind(commandList, MeshType::Square);

        commandList->SetDescriptorHeaps(1, &heap);
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(heap->GetGPUDescriptorHandleForHeapStart());
        gpuHandle.Offset(currentFrame, Graphic::Get().GetCBVHandleSize());
        commandList->SetGraphicsRootDescriptorTable(0, gpuHandle);

        const float green = 0.5f;
        commandList->SetGraphicsRoot32BitConstants(1, 1, &green, 0);

        commandList->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Window::Get().GetWidth()), static_cast<float>(Window::Get().GetHeight())));
        commandList->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));

        MeshManager::Get().Draw(commandList, MeshType::Square);

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

        commandList.Submit();

        Engine::Get().PostUpdate();
    }

    Engine::Get().PreShutdown();

    heap->Release();
    constantBuffer.reset();

    Engine::Get().Shutdown();

}
