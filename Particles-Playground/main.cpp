#include "System/engine.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    Engine::Get().Startup();
   
    uint32_t constantBufferSize = (16 + 16) * sizeof(float);
    constantBufferSize = AlignPow2(constantBufferSize, 256U);

    std::unique_ptr<GPUBuffer> constantBuffer = std::make_unique<GPUBuffer>(constantBufferSize, 1, BufferUsage::Constant);
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

    GPUParticleSystem gpuParticlesSystem;
    gpuParticlesSystem.Init();

    GPUEmitterHandle emitter1 = gpuParticlesSystem.CreateEmitter(200);
    emitter1->SetSpawnRate(10.0f).SetParticleLifeTime(2.0f).SetParticleColor({ 1,0,0,1 });

    GPUEmitterHandle emitter2 = gpuParticlesSystem.CreateEmitter(400);
    emitter2->SetSpawnRate(20.0f).SetParticleLifeTime(5.0f).SetParticleColor({ 0,1,0,1 });

    GPUEmitterHandle emitter3 = gpuParticlesSystem.CreateEmitter(100);
    emitter3->SetSpawnRate(5.0f).SetParticleLifeTime(3.0f).SetParticleColor({ 0,0,1,1 });

    std::unique_ptr<Texture2D> renderTarget = std::make_unique<Texture2D>(Window::Get().GetWidth(), Window::Get().GetHeight(), TextureFormat::R8G8B8A8, TextureUsage::RenderTarget | TextureUsage::ShaderResource);
    renderTarget->SetDebugName(L"TestRenderTarget");

    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>(100, 100, TextureFormat::R32G32B32A32, TextureUsage::ShaderResource | TextureUsage::CopyDst);
    texture->SetDebugName(L"TestTexture");

    { // Generate a simple texture
        uint8_t* data = texture->Map();
    
        float* floatData = reinterpret_cast<float*>(data);

        for (uint32_t y = 0; y < texture->GetHeight(); ++y)
        {
            for (uint32_t x = 0; x < texture->GetWidth(); ++x, floatData += 4)
            {
                floatData[0] = x / static_cast<float>(texture->GetWidth());
                floatData[1] = y / static_cast<float>(texture->GetHeight());
                floatData[2] = 0.0f;
                floatData[3] = 1.0f;
            }
        
            floatData += texture->GetRowOffset() / sizeof(float);
        }
        
        CommandList commandList(QueueType::Direct);    
        texture->Unmap(commandList);
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

            gpuParticlesSystem.Update(commandList);

            {
                std::vector<D3D12_RESOURCE_BARRIER> barriers;
                renderTarget->SetCurrentUsage(TextureUsage::RenderTarget, false, barriers);

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());

                FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                commandList->ClearRenderTargetView(renderTarget->GetRTV(), clearColor, 0, nullptr);
            }

            gpuParticlesSystem.DrawParticles(commandList, constantBuffer.get(), renderTarget.get());

            {
                std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
                barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }

            // Render on screen
            Sampler defaultSampler;

            ShaderParametersLayout screenLayout;
            screenLayout.SetSRV(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
            screenLayout.SetStaticSampler(0, defaultSampler, D3D12_SHADER_VISIBILITY_PIXEL);

            GraphicPipelineState screenState;
            screenState.SetVS(L"vsscreen");
            screenState.SetPS(L"psscreen");
            screenState.Bind(commandList, screenLayout);
            
            ShaderParameters screenParams;
            screenParams.SetSRV(0, *renderTarget);
            screenParams.Bind<true>(commandList, screenLayout);
            
            MeshManager::Get().Bind(commandList, MeshType::Square);
            
            const CD3DX12_CPU_DESCRIPTOR_HANDLE rtHandle = Graphic::Get().GetCurrentRenderTargetHandle();
            commandList->OMSetRenderTargets(1, &rtHandle, false, nullptr);

            MeshManager::Get().Draw(commandList, MeshType::Square, 1);
            
            {
                std::array< CD3DX12_RESOURCE_BARRIER, 1> barriers;
                barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(Graphic::Get().GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

                commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
            }

            commandList.Submit();
        }

        Engine::Get().PostUpdate();
    }

    gpuParticlesSystem.FreeEmitter(emitter1);
    gpuParticlesSystem.FreeEmitter(emitter2);
    gpuParticlesSystem.FreeEmitter(emitter3);

    Engine::Get().PreShutdown();

    gpuParticlesSystem.Free();
    texture.reset();
    renderTarget.reset();
    constantBuffer.reset();

    Engine::Get().Shutdown();

}
