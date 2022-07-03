#include "System/engine.h"

int32_t WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nShowCmd)
{
    Engine::Get().Startup();

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
    particle.position = float3(GetRandomFloat() * 3, 0.0f, 0.0f) + emitterConstant.position; \n\
    particle.color = emitterConstant.color; \n\
    particle.lifeTime = emitterConstant.particleLifeTime; \n\
    particle.velocity = float3(cos(phi), sin(phi), 0) * 15.0f; \n\
    particle.scale = 0.3f; \n";

    const char* updateLogic2 = "particle.position += particle.velocity * Constants.deltaTime; \n\
    float3 diff = emitterConstant.position - particle.position; \n\
    float dist = max(distance(diff, 0), 0.01f); \n\
    float distClamped = clamp(dist, 4, 9); \n\
    particle.velocity += (diff / dist) * ((25 * 25) / (distClamped * distClamped)) * Constants.deltaTime; \n\
    particle.color = lerp(emitterConstant.color, float4(0.5f, 0, 1, 0.01f), dist / 22); \n\
    particle.scale = lerp(0.2f, 2.0f, dist / 22); \n";

    const char* spawnLogic2 = "float random = GetRandomFloat() * 6.28f; \n\
    particle.position = emitterConstant.position + float3( 20 * cos(random), 10 * sin(random), 0 ); \n\
    float3 diff = emitterConstant.position - particle.position; \n\
    float dist = max(distance(diff, 0), 0.01f); \n\
    particle.color = lerp(emitterConstant.color, float4(0.5f, 0, 1, 0.01f), dist / 22); \n\
    particle.lifeTime = emitterConstant.particleLifeTime; \n\
    particle.velocity = float3(GetRandomFloat() * 2 - 1, GetRandomFloat() * 2 - 1, 0) * 5; \n\
    particle.scale = lerp(0.2f, 2.0f, dist / 22); \n";

    GPUEmitterTemplateHandle emitterTemplateHandle1 = gpuParticlesSystem.CreateEmitterTemplate();
    GPUEmitterTemplate* emitterTemplate1 = gpuParticlesSystem.GetEmitterTemplate(emitterTemplateHandle1);
    emitterTemplate1->SetSpawnShader(spawnLogic);
    emitterTemplate1->SetUpdateShader(updateLogic);

    GPUEmitterHandle emitter1 = gpuParticlesSystem.CreateEmitter(emitterTemplateHandle1, 800);
    gpuParticlesSystem.GetEmitter(emitter1)->SetSpawnRate(100.0f).SetParticleLifeTime(2.0f).SetParticleColor({ 1,0.1f,0.1f,1 }).SetPosition({ -20,0,0 }).SetLoopTime(3);
    
    GPUEmitterTemplateHandle emitterTemplateHandle2 = gpuParticlesSystem.CreateEmitterTemplate();
    GPUEmitterTemplate* emitterTemplate2 = gpuParticlesSystem.GetEmitterTemplate(emitterTemplateHandle2);
    emitterTemplate2->SetSpawnShader(spawnLogic2);
    emitterTemplate2->SetUpdateShader(updateLogic2);

    GPUEmitterHandle emitter2 = gpuParticlesSystem.CreateEmitter(emitterTemplateHandle2, 200);
    gpuParticlesSystem.GetEmitter(emitter2)->SetSpawnRate(20.0f).SetParticleLifeTime(1.0f).SetParticleColor({ 0,0.5f,1,1 }).SetPosition({ 20,0,0 }).SetLoopTime(10);

    Engine::Get().PostStartup();
    
    TransientResourceAllocator transientAllocator;
    transientAllocator.Init();

    RenderGraph graph;
    graph.AddExternalGPUBuffer(RESOURCEID("EmitterConstantBuffer"), gpuParticlesSystem.GetEmitterConstantBuffer());
    graph.AddExternalGPUBuffer(RESOURCEID("EmitterStatusBuffer"), gpuParticlesSystem.GetEmitterStatusBuffer());
    graph.AddExternalGPUBuffer(RESOURCEID("EmitterIndexBuffer"), gpuParticlesSystem.GetEmitterIndexBuffer());
    graph.AddExternalGPUBuffer(RESOURCEID("DrawIndirectBuffer"), gpuParticlesSystem.GetDrawIndirectBuffer());
    graph.AddExternalGPUBuffer(RESOURCEID("FreeIndicesBuffer"), gpuParticlesSystem.GetFreeIndicesBuffer());
    graph.AddExternalGPUBuffer(RESOURCEID("ParticlesDataBuffer"), gpuParticlesSystem.GetParticlesDataBuffer());

    graph.AddNode<PrepareSceneBufferNode>();
    graph.AddNode<GPUParticleSystemUpdateDirtyEmittersNode>();
    graph.AddNode<GPUParticleSystemDirtyEmittersFreeIndicesNode>();
    graph.AddNode<GPUParticleSystemUpdateEmittersNode>();
    graph.AddNode<GPUParticleSystemUpdateParticlesNode>();
    graph.AddNode<GPUParticleSystemSpawnParticlesNode>();
    graph.AddNode<GPUParticleSystemDrawParticlesNode>();
    graph.AddNode<PresentToScreenNode>(true);

    graph.Setup();

    Camera camera({ 0, 0, -30, 1 }, { 0, 0, 1, 0 });

    SceneData sceneData;
    sceneData.mGPUParticleSystem = &gpuParticlesSystem;
    sceneData.mCamera = &camera;

    while (Window::Get().IsRunning())
    {
        Engine::Get().PreUpdate();

        transientAllocator.PreUpdate();

        //GlobalTimer& timer = Engine::Get().GetTimer();
        //OutputDebugMessage("Elapsed: %f, Delta: %f\n", timer.GetElapsedTime(), timer.GetDeltaTime());

        graph.Execute(transientAllocator, sceneData);

        gpuParticlesSystem.PostUpdate();
        Engine::Get().PostUpdate();
    }

    gpuParticlesSystem.FreeEmitter(emitter1);
    gpuParticlesSystem.FreeEmitter(emitter2);
    gpuParticlesSystem.FreeEmitterTemplate(emitterTemplateHandle1);
    gpuParticlesSystem.FreeEmitterTemplate(emitterTemplateHandle2);

    Engine::Get().PreShutdown();

    transientAllocator.Free();
    gpuParticlesSystem.Free();

    Engine::Get().Shutdown();

}
