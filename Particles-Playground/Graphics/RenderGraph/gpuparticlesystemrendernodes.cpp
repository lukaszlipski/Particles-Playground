#include "Graphics/RenderGraph/gpuparticlesystemrendernodes.h"

void GPUParticleSystemUpdateDirtyEmittersNode::Execute(const RGExecuteContext& context)
{
    std::vector<GPUEmitter*> dirtyEmitters = context.GetSceneData().mGPUParticleSystem->GetDirtyEmitters();

    if (dirtyEmitters.empty())
    {
        return;
    }

    GPUBuffer* emitterConstantBuffer = context.GetGPUBuffer(RESOURCEID("EmitterConstantBuffer"));
    GPUBuffer* emitterStatusBuffer = context.GetGPUBuffer(RESOURCEID("EmitterStatusBuffer"));
    GPUBuffer* drawIndirectBuffer = context.GetGPUBuffer(RESOURCEID("DrawIndirectBuffer"));

    CommandList& commandList = context.GetCommandList();

    for (GPUEmitter* emitter : dirtyEmitters)
    {
        { // Reset constant data
            const uint32_t offset = emitter->GetEmitterIndexGPU() * sizeof(EmitterConstantData);
            const uint32_t size = sizeof(EmitterConstantData);

            uint8_t* data = emitterConstantBuffer->Map(offset, offset + size);
            const EmitterConstantData& constantData = emitter->GetConstantData();
            memcpy(data, &constantData, sizeof(EmitterConstantData));
            emitterConstantBuffer->Unmap(commandList);
        }

        { // Reset status data
            const uint32_t offset = emitter->GetEmitterIndexGPU() * sizeof(EmitterStatusData);
            const uint32_t size = sizeof(EmitterStatusData);
        
            uint8_t* data = emitterStatusBuffer->Map(offset, offset + size);
            EmitterStatusData statusData = emitter->GetDefaultStatusData();
            memcpy(data, &statusData, sizeof(EmitterStatusData));
            emitterStatusBuffer->Unmap(commandList);
        }
        
        { // Reset draw arguments
            const uint32_t ioffset = emitter->GetEmitterIndexGPU() * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
            const uint32_t isize = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        
            uint8_t* data = drawIndirectBuffer->Map(ioffset, ioffset + isize);
            memset(data, 0, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
            reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>(data)->IndexCountPerInstance = 6;
            drawIndirectBuffer->Unmap(commandList);
        }
    }
}

void GPUParticleSystemDirtyEmittersFreeIndicesNode::Execute(const RGExecuteContext& context)
{
    std::vector<GPUEmitter*> dirtyEmitters = context.GetSceneData().mGPUParticleSystem->GetDirtyEmitters();

    if (dirtyEmitters.empty())
    {
        return;
    }

    GPUBuffer* freeIndicesBuffer = context.GetGPUBuffer(RESOURCEID("FreeIndicesBuffer"));

    CommandList& commandList = context.GetCommandList();

    struct ResetConstants
    {
        BindlessDescriptorHandle freeIndicesHandle;
        uint32_t offset;
        uint32_t indicesCount;
    } constants;

    constants.freeIndicesHandle = freeIndicesBuffer->GetUAVIndex();

    ShaderParametersLayout resetFreeIndicesLayout;
    resetFreeIndicesLayout.SetConstant(0, 0, sizeof(ResetConstants), D3D12_SHADER_VISIBILITY_ALL);
    resetFreeIndicesLayout.SetBindlessHeap(1);

    ComputePipelineState resetFreeIndicesState;
    resetFreeIndicesState.SetCS(CS_ResetFreeIndices);
    resetFreeIndicesState.Bind(commandList, resetFreeIndicesLayout);

    for (GPUEmitter* emitter : dirtyEmitters)
    {
        const uint32_t offset = static_cast<uint32_t>(emitter->GetParticleAllocation().Start);
        const uint32_t size = static_cast<uint32_t>(emitter->GetParticleAllocation().Size);

        constants.offset = offset;
        constants.indicesCount = size;

        ShaderParameters resetFreeIndicesParams;
        resetFreeIndicesParams.SetConstant(0, constants);
        resetFreeIndicesParams.Bind<false>(commandList, resetFreeIndicesLayout);

        uint32_t dispatchCount = Align(size, 64) / 64;
        commandList->Dispatch(dispatchCount, 1, 1);
    }
}

void GPUParticleSystemUpdateEmittersNode::Execute(const RGExecuteContext& context)
{
    SceneData& sceneData = context.GetSceneData();
    std::vector<GPUEmitter*> enabledEmitters = sceneData.mGPUParticleSystem->GetEnabledEmitters();

    if (enabledEmitters.empty())
    {
        return;
    }

    GPUBuffer* emitterConstantBuffer = context.GetGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"));
    GPUBuffer* emitterIndexBuffer = context.GetGPUBuffer(RESOURCEID("EmitterIndexBuffer"));
    GPUBuffer* emitterStatusBuffer = context.GetGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterStatusBuffer"));
    GPUBuffer* drawIndirectBuffer = context.GetGPUBuffer(RESOURCEID("UpdateDirtyEmitters_DrawIndirectBuffer"));
    GPUBuffer* spawnIndirectBuffer = context.GetGPUBuffer(RESOURCEID("SpawnIndirectBuffer"));

    CommandList& commandList = context.GetCommandList();

    const uint32_t enabledEmittersCount = static_cast<uint32_t>(enabledEmitters.size());
    uint32_t* emitterData = reinterpret_cast<uint32_t*>(emitterIndexBuffer->Map(0, enabledEmittersCount * sizeof(uint32_t)));

    for (uint32_t i = 0; i < enabledEmitters.size(); ++i)
    {
        GPUEmitter* emitter = enabledEmitters[i];
        emitterData[i] = emitter->GetEmitterIndexGPU();
    }
    emitterIndexBuffer->Unmap(commandList);

    GlobalTimer& timer = Engine::Get().GetTimer();

    struct EmitterUpdateConstants
    {
        uint32_t emittersCount;
        float deltaTime;
    };

    ShaderParametersLayout updateEmitterLayout;
    updateEmitterLayout.SetConstant(0, 0, sizeof(EmitterUpdateConstants) / sizeof(uint32_t), D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetSRV(2, 1, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetUAV(3, 1, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetUAV(4, 2, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetUAV(5, 3, D3D12_SHADER_VISIBILITY_ALL);

    ComputePipelineState updateEmitterState;
    updateEmitterState.SetCS(CS_EmitterUpdate);
    updateEmitterState.Bind(commandList, updateEmitterLayout);

    EmitterUpdateConstants updateConstants;
    updateConstants.emittersCount = enabledEmittersCount;
    updateConstants.deltaTime = timer.GetDeltaTime();

    ShaderParameters updateEmitterParams;
    updateEmitterParams.SetConstant(0, updateConstants);
    updateEmitterParams.SetSRV(1, *emitterConstantBuffer);
    updateEmitterParams.SetSRV(2, *emitterIndexBuffer);
    updateEmitterParams.SetUAV(3, *emitterStatusBuffer);
    updateEmitterParams.SetUAV(4, *drawIndirectBuffer);
    updateEmitterParams.SetUAV(5, *spawnIndirectBuffer);
    updateEmitterParams.Bind<false>(commandList, updateEmitterLayout);

    const uint32_t dispatchCount = Align(enabledEmittersCount, 64) / 64;
    commandList->Dispatch(dispatchCount, 1, 1);
}

void GPUParticleSystemUpdateParticlesNode::Execute(const RGExecuteContext& context)
{
    GPUBuffer* emitterConstantBuffer = context.GetGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"));
    GPUBuffer* particlesDataBuffer = context.GetGPUBuffer(RESOURCEID("ParticlesDataBuffer"));
    GPUBuffer* emitterStatusBuffer = context.GetGPUBuffer(RESOURCEID("UpdateEmitters_EmitterStatusBuffer"));
    GPUBuffer* indicesBuffer = context.GetGPUBuffer(RESOURCEID("IndicesBuffer"));
    GPUBuffer* freeIndicesBuffer = context.GetGPUBuffer(RESOURCEID("DirtyEmittersFreeIndices_FreeIndicesBuffer"));
    GPUBuffer* drawIndirectBuffer = context.GetGPUBuffer(RESOURCEID("UpdateEmitters_DrawIndirectBuffer"));

    CommandList& commandList = context.GetCommandList();
    SceneData& sceneData = context.GetSceneData();
    std::vector<GPUEmitter*> enabledEmitters = sceneData.mGPUParticleSystem->GetEnabledEmitters();

    GlobalTimer& timer = Engine::Get().GetTimer();

    ShaderParametersLayout updateLayout;
    updateLayout.SetConstant(0, 0, 2, D3D12_SHADER_VISIBILITY_ALL);
    updateLayout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    updateLayout.SetUAV(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    updateLayout.SetUAV(3, 1, D3D12_SHADER_VISIBILITY_ALL);
    updateLayout.SetUAV(4, 2, D3D12_SHADER_VISIBILITY_ALL);
    updateLayout.SetUAV(5, 3, D3D12_SHADER_VISIBILITY_ALL);
    updateLayout.SetUAV(6, 4, D3D12_SHADER_VISIBILITY_ALL);

    struct UpdateConstants
    {
        uint32_t emitterIndex;
        float deltaTime;
    } constants;

    constants.deltaTime = timer.GetDeltaTime();

    for (GPUEmitter* emitter : enabledEmitters)
    {
        GPUEmitterTemplate* emitterTemplate = sceneData.mGPUParticleSystem->GetEmitterTemplate(emitter->GetTemplateHandle());

        ComputePipelineState updateState;
        updateState.SetCS(emitterTemplate->GetUpdateShader());
        updateState.Bind(commandList, updateLayout);

        constants.emitterIndex = emitter->GetEmitterIndexGPU();

        ShaderParameters updateParams;
        updateParams.SetConstant(0, constants);
        updateParams.SetSRV(1, *emitterConstantBuffer);
        updateParams.SetUAV(2, *particlesDataBuffer);
        updateParams.SetUAV(3, *emitterStatusBuffer);
        updateParams.SetUAV(4, *indicesBuffer);
        updateParams.SetUAV(5, *freeIndicesBuffer);
        updateParams.SetUAV(6, *drawIndirectBuffer);
        updateParams.Bind<false>(commandList, updateLayout);

        commandList->Dispatch(emitter->GetMaxParticles(), 1, 1);
    }
}

void GPUParticleSystemSpawnParticlesNode::Execute(const RGExecuteContext& context)
{
    GPUBuffer* emitterConstantBuffer = context.GetGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"));
    GPUBuffer* spawnIndirectBuffer = context.GetGPUBuffer(RESOURCEID("SpawnIndirectBuffer"));
    GPUBuffer* particlesDataBuffer = context.GetGPUBuffer(RESOURCEID("Update_ParticlesDataBuffer"));
    GPUBuffer* freeIndicesBuffer = context.GetGPUBuffer(RESOURCEID("Update_FreeIndicesBuffer"));
    GPUBuffer* indicesBuffer = context.GetGPUBuffer(RESOURCEID("IndicesBuffer"));
    GPUBuffer* drawIndirectBuffer = context.GetGPUBuffer(RESOURCEID("Update_DrawIndirectBuffer"));
    GPUBuffer* emitterStatusBuffer = context.GetGPUBuffer(RESOURCEID("Update_EmitterStatusBuffer"));

    CommandList& commandList = context.GetCommandList();
    SceneData& sceneData = context.GetSceneData();
    std::vector<GPUEmitter*> enabledEmitters = sceneData.mGPUParticleSystem->GetEnabledEmitters();

    ShaderParametersLayout spawnLayout;
    spawnLayout.SetConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(3, 1, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(4, 2, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(5, 3, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(6, 4, D3D12_SHADER_VISIBILITY_ALL);

    for (GPUEmitter* emitter : enabledEmitters)
    {
        GPUEmitterTemplate* emitterTemplate = sceneData.mGPUParticleSystem->GetEmitterTemplate(emitter->GetTemplateHandle());

        ComputePipelineState spawnState;
        spawnState.SetCS(emitterTemplate->GetSpawnShader());
        spawnState.Bind(commandList, spawnLayout);

        ShaderParameters spawnParams;
        spawnParams.SetConstant(0, emitter->GetEmitterIndexGPU());
        spawnParams.SetSRV(1, *emitterConstantBuffer);
        spawnParams.SetUAV(2, *particlesDataBuffer);
        spawnParams.SetUAV(3, *freeIndicesBuffer);
        spawnParams.SetUAV(4, *indicesBuffer);
        spawnParams.SetUAV(5, *drawIndirectBuffer);
        spawnParams.SetUAV(6, *emitterStatusBuffer);
        spawnParams.Bind<false>(commandList, spawnLayout);

        const uint32_t dispatchOffset = emitter->GetEmitterIndexGPU() * sizeof(D3D12_DISPATCH_ARGUMENTS);
        commandList->ExecuteIndirect(Graphic::Get().GetDefaultDispatchCommandSignature(), 1, spawnIndirectBuffer->GetResource(), dispatchOffset, nullptr, 0);
    }
}

void GPUParticleSystemDrawParticlesNode::Execute(const RGExecuteContext& context)
{
    Texture2D* renderTarget = context.GetTexture2D(RESOURCEID("RenderTarget"));
    GPUBuffer* particlesDataBuffer = context.GetGPUBuffer(RESOURCEID("Spawn_ParticlesDataBuffer"));
    GPUBuffer* indicesBuffer = context.GetGPUBuffer(RESOURCEID("Spawn_IndicesBuffer"));
    GPUBuffer* drawIndirectBuffer = context.GetGPUBuffer(RESOURCEID("Spawn_DrawIndirectBuffer"));
    GPUBuffer* sceneBuffer = context.GetGPUBuffer(RESOURCEID("SceneBuffer"));

    CommandList& commandList = context.GetCommandList();
    SceneData& sceneData = context.GetSceneData();
    std::vector<GPUEmitter*> emitters = sceneData.mGPUParticleSystem->GetEmitters();

    Sampler defaultSampler;

    ShaderParametersLayout drawLayout;
    drawLayout.SetConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
    drawLayout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    drawLayout.SetSRV(2, 1, D3D12_SHADER_VISIBILITY_VERTEX);
    drawLayout.SetSRV(3, 2, D3D12_SHADER_VISIBILITY_VERTEX);
    //drawLayout.SetSRV(3, 2, D3D12_SHADER_VISIBILITY_PIXEL);
    drawLayout.SetStaticSampler(0, defaultSampler, D3D12_SHADER_VISIBILITY_PIXEL);

    GraphicPipelineState drawState;
    drawState.SetVS(VS_DrawParticle);
    drawState.SetPS(PS_DrawParticle);

    D3D12_RENDER_TARGET_BLEND_DESC blendDesc{};
    blendDesc.BlendEnable = TRUE;
    blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    drawState.SetRTBlendState(0, blendDesc);
    drawState.Bind(commandList, drawLayout);

    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 1> rtvHandles = { renderTarget->GetRTV() };
    commandList->OMSetRenderTargets(static_cast<uint32_t>(rtvHandles.size()), rtvHandles.data(), true, nullptr);
    MeshManager::Get().Bind(commandList, MeshType::Square);

    for (GPUEmitter* emitter : emitters)
    {
        const uint32_t constant = static_cast<uint32_t>(emitter->GetParticleAllocation().Start);

        ShaderParameters drawParams;
        drawParams.SetConstant(0, constant);
        drawParams.SetSRV(1, *sceneBuffer);
        drawParams.SetSRV(2, *particlesDataBuffer);
        drawParams.SetSRV(3, *indicesBuffer);
        //drawParams.SetSRV(4, *texture);
        drawParams.Bind<true>(commandList, drawLayout);

        const uint32_t drawOffset = emitter->GetEmitterIndexGPU() * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        commandList->ExecuteIndirect(Graphic::Get().GetDefaultDrawCommandSignature(), 1, drawIndirectBuffer->GetResource(), drawOffset, nullptr, 0);
    }
}
