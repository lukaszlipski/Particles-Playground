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

    const char* updateLogic = "particle.position += particle.velocity * Constants.deltaTime;\n\
    float3 forces = float3(0.0f, -9.8f, 0.0f) + float3(-4.0f, 0.0f, 0.0f);\n\
    float mass = 1.0f;\n\
    particle.velocity += (forces / mass) * Constants.deltaTime;\n\
    float localLifeTime = particle.lifeTime / emitterConstant.particleLifeTime;\n\
    particle.color = lerp(float4(0,1,1,0), emitterConstant.color, localLifeTime);\n\
    particle.scale = lerp(2.0f, 0.3f, localLifeTime);\n\
    particle.lifeTime -= Constants.deltaTime;\n";

    const char* spawnLogic = "float phi = GetRandomFloat() * 3.14f; \n\
    particle.position = float3(0.0f, 0.0f, 0.0f) + emitterConstant.position; \n\
    particle.color = emitterConstant.color; \n\
    particle.lifeTime = emitterConstant.particleLifeTime; \n\
    particle.velocity = float3(cos(phi), sin(phi), 0) * 15.0f; \n\
    particle.scale = 1.0f; \n";

    const char* updateLogic2 = "particle.position += particle.velocity * Constants.deltaTime; \n\
    float3 diff = emitterConstant.position - particle.position; \n\
    float dist = max(distance(diff, 0), 0.01f); \n\
    float distClamped = clamp(dist, 4, 9); \n\
    particle.velocity += (diff / dist) * ((25 * 25) / (distClamped * distClamped)) * Constants.deltaTime; \n\
    particle.color = lerp(emitterConstant.color, float4(0.5f, 0, 1, 0.3f), dist / 22); \n\
    particle.scale = lerp(0.2f, 2.0f, dist / 22); \n";

    const char* spawnLogic2 = "float random = GetRandomFloat() * 6.28f; \n\
    particle.position = emitterConstant.position + float3( 20 * cos(random), 10 * sin(random), 0 ); \n\
    particle.color = emitterConstant.color; \n\
    particle.lifeTime = emitterConstant.particleLifeTime; \n\
    particle.velocity = float3(GetRandomFloat() * 2 - 1, GetRandomFloat() * 2 - 1, 0) * 5; \n\
    particle.scale = 1.0f; \n";

    GPUEmitterTemplateHandle emitterTemplateHandle1 = gpuParticlesSystem.CreateEmitterTemplate();
    GPUEmitterTemplate* emitterTemplate1 = gpuParticlesSystem.GetEmitterTemplate(emitterTemplateHandle1);
    emitterTemplate1->SetSpawnShader(spawnLogic);
    emitterTemplate1->SetUpdateShader(updateLogic);

    GPUEmitterHandle emitter1 = gpuParticlesSystem.CreateEmitter(emitterTemplateHandle1, 800);
    gpuParticlesSystem.GetEmitter(emitter1)->SetSpawnRate(100.0f).SetParticleLifeTime(2.0f).SetParticleColor({ 1,0,0,1 }).SetPosition({ -20,0,0 }).SetLoopTime(3);
    
    GPUEmitterTemplateHandle emitterTemplateHandle2 = gpuParticlesSystem.CreateEmitterTemplate();
    GPUEmitterTemplate* emitterTemplate2 = gpuParticlesSystem.GetEmitterTemplate(emitterTemplateHandle2);
    emitterTemplate2->SetSpawnShader(spawnLogic2);
    emitterTemplate2->SetUpdateShader(updateLogic2);

    GPUEmitterHandle emitter2 = gpuParticlesSystem.CreateEmitter(emitterTemplateHandle2, 100);
    gpuParticlesSystem.GetEmitter(emitter2)->SetSpawnRate(5.0f).SetParticleLifeTime(1.0f).SetParticleColor({ 0,0.5f,1,1 }).SetPosition({ 20,0,0 }).SetLoopTime(10);

    std::unique_ptr<Texture2D> renderTarget = std::make_unique<Texture2D>(Window::Get().GetWidth(), Window::Get().GetHeight(), TextureFormat::R8G8B8A8, TextureUsage::RenderTarget | TextureUsage::ShaderResource);
    renderTarget->SetDebugName(L"TestRenderTarget");

    std::unique_ptr<Texture2D> depthBuffer = std::make_unique<Texture2D>(Window::Get().GetWidth(), Window::Get().GetHeight(), TextureFormat::D32, TextureUsage::DepthWrite);
    depthBuffer->SetDebugName(L"DepthBuffer");

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

                commandList->ClearDepthStencilView(depthBuffer->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
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
            screenState.SetVS(VS_Screen);
            screenState.SetPS(PS_Screen);
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
    gpuParticlesSystem.FreeEmitterTemplate(emitterTemplateHandle1);
    gpuParticlesSystem.FreeEmitterTemplate(emitterTemplateHandle2);

    Engine::Get().PreShutdown();

    gpuParticlesSystem.Free();
    texture.reset();
    depthBuffer.reset();
    renderTarget.reset();
    constantBuffer.reset();

    Engine::Get().Shutdown();

}
