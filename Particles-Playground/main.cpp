#include "System/engine.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    Engine::Get().Startup();
   
    uint32_t constantBufferCount = Graphic::Get().GetFrameCount();
    uint32_t constantBufferSize = (16 + 16) * sizeof(float);
    constantBufferSize = AlignPow2(constantBufferSize, 256U);

    std::unique_ptr<GPUBuffer> constantBuffer = std::make_unique<GPUBuffer>(constantBufferSize, constantBufferCount, BufferUsage::Constant);
    constantBuffer->SetDebugName(L"ConstantBuffer");

    // Prepare a camera
    Camera camera({ 0, 0,-30,1 }, { 0, 0, 1,0 });
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

    // Common structures
    struct ParticleData
    {
        XMFLOAT3 Position;
        float LifeTime;
        XMFLOAT3 Velocity;
        float Scale;
        XMFLOAT4 Color;
    };

    struct EmitterData
    {
        uint32_t AliveParticles;
        uint32_t MaxParticles;
        uint32_t ParticlesToSpawn;
        float SpawnAccTime;
        float SpawnRate;
        float LifeTime;
    };

    const uint32_t maxParticleCount = 128;

    std::unique_ptr<GPUBuffer> particlesDataBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(ParticleData)), maxParticleCount, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    particlesDataBuffer->SetDebugName(L"ParticlesDataBuffer");

    std::unique_ptr<GPUBuffer> emitterDataBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(EmitterData)), 1, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    emitterDataBuffer->SetDebugName(L"EmitterDataBuffer");

    std::unique_ptr<GPUBuffer> indicesBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(int32_t)), maxParticleCount, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    indicesBuffer->SetDebugName(L"IndicesBuffer");

    std::unique_ptr<GPUBuffer> freeIndicesBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(int32_t)), maxParticleCount, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    freeIndicesBuffer->SetDebugName(L"FreeIndicesBuffer");

    std::unique_ptr<GPUBuffer> drawIndirectBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(D3D12_DRAW_INDEXED_ARGUMENTS)), 1, BufferUsage::Indirect | BufferUsage::UnorderedAccess);
    drawIndirectBuffer->SetDebugName(L"DrawIndirectBuffer");

    std::unique_ptr<GPUBuffer> dispatchIndirectBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(D3D12_DISPATCH_ARGUMENTS)), 1, BufferUsage::Indirect | BufferUsage::UnorderedAccess);
    dispatchIndirectBuffer->SetDebugName(L"DispatchIndirectBuffer");

    {
        CommandList commandList(QueueType::Direct);

        // Emitter's data
        uint8_t* data = emitterDataBuffer->Map();

        EmitterData emitterData;
        emitterData.AliveParticles = 0;
        emitterData.MaxParticles = maxParticleCount;
        emitterData.ParticlesToSpawn = 0;
        emitterData.SpawnAccTime = 0;
        emitterData.SpawnRate = 100.0f;
        emitterData.LifeTime = 3.0f;

        memcpy(data, &emitterData, sizeof(EmitterData));

        emitterDataBuffer->Unmap(commandList);

        // Free Indices
        data = freeIndicesBuffer->Map();

        std::vector<int32_t> indices(maxParticleCount);
        std::iota(indices.begin(), indices.end(), 0);

        memcpy(data, indices.data(), indices.size() * sizeof(int32_t));

        freeIndicesBuffer->Unmap(commandList);

        // Indirect arguments
        data = drawIndirectBuffer->Map();

        memset(data, 0, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
        reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>(data)->IndexCountPerInstance = 6;

        drawIndirectBuffer->Unmap(commandList);

        commandList.Submit();
    }

    Engine::Get().PostStartup();
    
    while (Window::Get().IsRunning())
    {
        Engine::Get().PreUpdate();

        GlobalTimer& timer = Engine::Get().GetTimer();
        //OutputDebugMessage("Elapsed: %f, Delta: %f\n", timer.GetElapsedTime(), timer.GetDeltaTime());

        {
            // Record commands
            CommandList commandList(QueueType::Direct);

            std::array<ID3D12DescriptorHeap*, 1> descHeaps = { Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetHeap() };
            commandList->SetDescriptorHeaps(static_cast<uint32_t>(descHeaps.size()), descHeaps.data());

            // Spawn particles
            ShaderParametersLayout updateEmitterLayout;
            updateEmitterLayout.SetConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
            updateEmitterLayout.SetUAV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
            updateEmitterLayout.SetUAV(2, 1, D3D12_SHADER_VISIBILITY_ALL);
            updateEmitterLayout.SetUAV(3, 2, D3D12_SHADER_VISIBILITY_ALL);
            ComputePipelineState updateEmitterState;
            updateEmitterState.SetCS(L"emitterupdate");
            updateEmitterState.Bind(commandList, updateEmitterLayout);

            ShaderParameters updateEmitterParams;
            updateEmitterParams.SetConstant(0, timer.GetDeltaTime());
            updateEmitterParams.SetUAV(1, *emitterDataBuffer);
            updateEmitterParams.SetUAV(2, *drawIndirectBuffer);
            updateEmitterParams.SetUAV(3, *dispatchIndirectBuffer);
            updateEmitterParams.Bind<false>(commandList);

            commandList->Dispatch(1, 1, 1);

            {
                std::array< CD3DX12_RESOURCE_BARRIER, 2> barriers;
                barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(dispatchIndirectBuffer->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(emitterDataBuffer->GetResource());

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }

            // Spawn particles
            ShaderParametersLayout spawnLayout;
            spawnLayout.SetUAV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
            spawnLayout.SetUAV(2, 1, D3D12_SHADER_VISIBILITY_ALL);
            spawnLayout.SetUAV(3, 2, D3D12_SHADER_VISIBILITY_ALL);
            ComputePipelineState spawnState;
            spawnState.SetCS(L"spawn");
            spawnState.Bind(commandList, spawnLayout);

            ShaderParameters spawnParams;
            spawnParams.SetUAV(1, *particlesDataBuffer);
            spawnParams.SetUAV(2, *emitterDataBuffer);
            spawnParams.SetUAV(3, *freeIndicesBuffer);
            spawnParams.Bind<false>(commandList);

            commandList->ExecuteIndirect(Graphic::Get().GetDefaultDispatchCommandSignature(), 1, dispatchIndirectBuffer->GetResource(), 0, nullptr, 0);

            {
                std::array< CD3DX12_RESOURCE_BARRIER, 4> barriers;
                barriers[0] = CD3DX12_RESOURCE_BARRIER::UAV(particlesDataBuffer->GetResource());
                barriers[1] = CD3DX12_RESOURCE_BARRIER::UAV(emitterDataBuffer->GetResource());
                barriers[2] = CD3DX12_RESOURCE_BARRIER::UAV(freeIndicesBuffer->GetResource());
                barriers[3] = CD3DX12_RESOURCE_BARRIER::UAV(drawIndirectBuffer->GetResource());

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }

            // Update particles' data
            ShaderParametersLayout updateLayout;
            updateLayout.SetConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
            updateLayout.SetUAV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
            updateLayout.SetUAV(2, 1, D3D12_SHADER_VISIBILITY_ALL);
            updateLayout.SetUAV(3, 2, D3D12_SHADER_VISIBILITY_ALL);
            updateLayout.SetUAV(4, 3, D3D12_SHADER_VISIBILITY_ALL);
            updateLayout.SetUAV(5, 4, D3D12_SHADER_VISIBILITY_ALL);
            ComputePipelineState updateState;
            updateState.SetCS(L"update");
            updateState.Bind(commandList, updateLayout);

            ShaderParameters updateParams;
            updateParams.SetConstant(0, timer.GetDeltaTime());
            updateParams.SetUAV(1, *particlesDataBuffer);
            updateParams.SetUAV(2, *emitterDataBuffer);
            updateParams.SetUAV(3, *indicesBuffer);
            updateParams.SetUAV(4, *freeIndicesBuffer);
            updateParams.SetUAV(5, *drawIndirectBuffer);
            updateParams.Bind<false>(commandList);

            uint32_t updateGroups = static_cast<uint32_t>(std::ceil(float(maxParticleCount) / 64.0f));
            commandList->Dispatch(updateGroups, 1, 1);

            {
                std::array< CD3DX12_RESOURCE_BARRIER, 2> barriers;
                barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(drawIndirectBuffer->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
                barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }

            FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            commandList->ClearRenderTargetView(Graphic::Get().GetCurrentRenderTargetHandle(), clearColor, 0, nullptr);

            ShaderParametersLayout layout;
            layout.SetCBV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            layout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
            layout.SetSRV(2, 1, D3D12_SHADER_VISIBILITY_VERTEX);
            layout.SetConstant(3, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL);

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

            ShaderParameters params;
            params.SetCBV(0, *constantBuffer);
            params.SetSRV(1, *particlesDataBuffer);
            params.SetSRV(2, *indicesBuffer);
            params.SetConstant(3, 0.5f);
            params.Bind<true>(commandList);

            commandList->ExecuteIndirect(Graphic::Get().GetDefaultDrawCommandSignature(), 1, drawIndirectBuffer->GetResource(), 0, nullptr, 0);

            {
                std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
                barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }

            commandList.Submit();
        }

        Engine::Get().PostUpdate();
    }

    Engine::Get().PreShutdown();

    dispatchIndirectBuffer.reset();
    drawIndirectBuffer.reset();
    emitterDataBuffer.reset();
    indicesBuffer.reset();
    freeIndicesBuffer.reset();
    particlesDataBuffer.reset();
    constantBuffer.reset();

    Engine::Get().Shutdown();

}
