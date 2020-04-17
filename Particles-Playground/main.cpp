#include "System/engine.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    Engine::Get().Startup();
   
    uint32_t constantBufferCount = Graphic::Get().GetFrameCount();
    uint32_t constantBufferSize = (16 + 16) * sizeof(float);
    constantBufferSize = AlignPow2(constantBufferSize, 256U);

    std::unique_ptr<GPUBuffer> constantBuffer = std::make_unique<GPUBuffer>(constantBufferSize, constantBufferCount, BufferUsage::Constant);

    // Prepare a camera
    Camera camera({ 0, 0,-3,1 }, { 0, 0, 1,0 });
    {
        CommandList commandList(QueueType::Direct);

        struct VSContants
        {
            XMMATRIX Projection;
            XMMATRIX View;
        };

        VSContants data;
        data.Projection = camera.GetProjection();
        data.View = camera.GetView();

        uint8_t* dstData = constantBuffer->Map();

        memcpy(dstData, &data, sizeof(VSContants));

        constantBuffer->Unmap(commandList);

        commandList.Submit();
    }

    // Init particles' data
    struct ParticleData
    {
        XMFLOAT3 Position;
        float Radius;
        float Fade;
    };

    uint32_t dataSize = Align(static_cast<uint32_t>(10 * sizeof(ParticleData)),256);
    std::array<ParticleData, 10> data;
    {
        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> distPos(-2.0f, 2.0f);
        std::uniform_real_distribution<float> distRadius(0.1f, 0.4f);
        std::uniform_real_distribution<float> distFade(0.1f, 1.0f);

        for (ParticleData& x : data)
        {
            x.Position = { distPos(rng), distPos(rng), 0.0f };
            x.Radius = distRadius(rng);
            x.Fade = distFade(rng);
        }
    }

    std::unique_ptr<GPUBuffer> srvBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(ParticleData)), 10, BufferUsage::Structured);

    Engine::Get().PostStartup();
    
    while (Window::Get().IsRunning())
    {
        Engine::Get().PreUpdate();

        // Update particles' data
        const float tmp = XMScalarCos(Graphic::Get().GetCurrentFrameNumber() / 100.0f) / 1000.0f;
        for (ParticleData& x : data)
        {
            x.Radius += tmp;
            x.Radius = std::clamp(x.Radius, 0.1f, 0.4f);
        }

        {
            // Record commands
            CommandList commandList(QueueType::Direct);

            {
                uint8_t* dstData = srvBuffer->Map();
                memcpy(dstData, data.data(), srvBuffer->GetBufferSize());
                srvBuffer->Unmap(commandList);
            }

            std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            commandList->ClearRenderTargetView(Graphic::Get().GetCurrentRenderTargetHandle(), clearColor, 0, nullptr);

            const PSOKey key{ PSOType::Default, MeshManager::Get().GetVertexFormatDescRef(MeshType::Square) };
            PSOManager::Get().Bind(commandList, key);

            commandList->OMSetRenderTargets(1, &Graphic::Get().GetCurrentRenderTargetHandle(), false, nullptr);

            MeshManager::Get().Bind(commandList, MeshType::Square);

            GPUDescriptorHandleScoped gpuHandle = Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            GPUDescriptorHandleScoped gpuHandleSRV = Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            Graphic::Get().GetDevice()->CopyDescriptorsSimple(1, gpuHandle, constantBuffer->GetCBV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            Graphic::Get().GetDevice()->CopyDescriptorsSimple(1, gpuHandleSRV, srvBuffer->GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            std::array<ID3D12DescriptorHeap*, 1> descHeaps = { Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap() };
            commandList->SetDescriptorHeaps(static_cast<uint32_t>(descHeaps.size()), descHeaps.data());

            commandList->SetGraphicsRootDescriptorTable(0, gpuHandle);
            commandList->SetGraphicsRootDescriptorTable(1, gpuHandleSRV);

            const float green = 0.5f;
            commandList->SetGraphicsRoot32BitConstants(2, 1, &green, 0);

            commandList->RSSetViewports(1, &CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(Window::Get().GetWidth()), static_cast<float>(Window::Get().GetHeight())));
            commandList->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX));

            MeshManager::Get().Draw(commandList, MeshType::Square, static_cast<uint32_t>(data.size()));

            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

            commandList.Submit();
        }

        Engine::Get().PostUpdate();
    }

    Engine::Get().PreShutdown();

    srvBuffer.reset();
    constantBuffer.reset();

    Engine::Get().Shutdown();

}
