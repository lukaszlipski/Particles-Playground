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
    std::unique_ptr<GPUBuffer> uavBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(int32_t)), 64, BufferUsage::UnorderedAccess);

    Engine::Get().PostStartup();
    
    while (Window::Get().IsRunning())
    {
        Engine::Get().PreUpdate();

        GlobalTimer& timer = Engine::Get().GetTimer();
        //OutputDebugMessage("Elapsed: %f, Delta: %f\n", timer.GetElapsedTime(), timer.GetDeltaTime());

        // Update particles' data
        const float tmp = XMScalarCos(timer.GetElapsedTime()) / 1000.0f;
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

            ShaderParametersLayout layout;
            layout.SetCBV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            layout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            layout.SetConstant(2, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

            GraphicPipelineState state;
            D3D12_RENDER_TARGET_BLEND_DESC blendDesc{};
            blendDesc.BlendEnable = TRUE;
            blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            state.SetRTBlendState(0, blendDesc);
            state.Bind(commandList, layout);

            commandList->OMSetRenderTargets(1, &Graphic::Get().GetCurrentRenderTargetHandle(), false, nullptr);

            MeshManager::Get().Bind(commandList, MeshType::Square);

            std::array<ID3D12DescriptorHeap*, 1> descHeaps = { Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap() };
            commandList->SetDescriptorHeaps(static_cast<uint32_t>(descHeaps.size()), descHeaps.data());

            ShaderParameters params;
            params.SetCBV(0, *constantBuffer);
            params.SetSRV(1, *srvBuffer);
            params.SetConstant(2, 0.5f);
            params.Bind<true>(commandList);

            MeshManager::Get().Draw(commandList, MeshType::Square, static_cast<uint32_t>(data.size()));

            barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            
            // Test compute shader
            ShaderParametersLayout cLayout;
            cLayout.SetUAV(0, 0, D3D12_SHADER_VISIBILITY_ALL);
            ComputePipelineState cState;
            cState.Bind(commandList, cLayout);
            
            ShaderParameters cParams;
            cParams.SetUAV(0, *uavBuffer);
            cParams.Bind<false>(commandList);

            commandList->Dispatch(1, 1, 1);

            commandList.Submit();
        }

        Engine::Get().PostUpdate();
    }

    Engine::Get().PreShutdown();

    uavBuffer.reset();
    srvBuffer.reset();
    constantBuffer.reset();

    Engine::Get().Shutdown();

}
