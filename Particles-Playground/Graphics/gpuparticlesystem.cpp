#include "Graphics/gpuparticlesystem.h"
#include "System/engine.h"

GPUParticleSystem::GPUParticleSystem() 
    : mParticlesAllocator(0, MaxParticles)
    , mEmittersPool(MaxEmitters)
    , mEmitterTemplatesPool(MaxEmitterTemplates)
{

}

void GPUParticleSystem::Init()
{
    mEmittersPool.Init();
    mEmitterTemplatesPool.Init();

    mParticlesDataBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(ParticleData)), MaxParticles, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mParticlesDataBuffer->SetDebugName(L"ParticlesDataBuffer");

    mIndicesBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(int32_t)), MaxParticles, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mIndicesBuffer->SetDebugName(L"IndicesBuffer");

    mFreeIndicesBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(int32_t)), MaxParticles, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mFreeIndicesBuffer->SetDebugName(L"FreeIndicesBuffer");

    mEmitterIndexBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(uint32_t)), MaxEmitters, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mEmitterIndexBuffer->SetDebugName(L"EmitterIndexBuffer");

    mEmitterConstantBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(EmitterConstantData)), MaxEmitters, BufferUsage::Structured | BufferUsage::CopyDst);
    mEmitterConstantBuffer->SetDebugName(L"EmitterConstantBuffer");

    mEmitterStatusBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(EmitterStatusData)), MaxEmitters, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mEmitterStatusBuffer->SetDebugName(L"EmitterStatusBuffer");

    mDrawIndirectBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) + sizeof(uint32_t)), MaxEmitters, BufferUsage::Indirect | BufferUsage::UnorderedAccess);
    mDrawIndirectBuffer->SetDebugName(L"DrawIndirectBuffer");

    mSpawnIndirectBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(uint32_t)), MaxEmitters, BufferUsage::Indirect | BufferUsage::UnorderedAccess);
    mSpawnIndirectBuffer->SetDebugName(L"SpawnIndirectBuffer");
}

void GPUParticleSystem::Free()
{
    mEmitterTemplatesPool.Free();
    mEmittersPool.Free();

    mSpawnIndirectBuffer.reset();
    mDrawIndirectBuffer.reset();
    mEmitterConstantBuffer.reset();
    mEmitterStatusBuffer.reset();
    mEmitterIndexBuffer.reset();
    mFreeIndicesBuffer.reset();
    mIndicesBuffer.reset();
    mParticlesDataBuffer.reset();
}

void GPUParticleSystem::Update(CommandList& commandList)
{
    UpdateDirtyEmitters(commandList);

    std::vector<GPUEmitter*> enabledEmitters = mEmittersPool.GetObjects([](GPUEmitter* emitter){
        return emitter->GetEnabled();   
    });
    
    UpdateEmitters(commandList, enabledEmitters);
    SpawnParticles(commandList, enabledEmitters);
    UpdateParticles(commandList, enabledEmitters);
}

void GPUParticleSystem::UpdateDirtyEmitters(CommandList& commandList)
{
    PIXScopedEvent(commandList.Get(), 0, "UpdateDirtyEmitters");

    std::vector<GPUEmitter*> dirtyEmitters = mEmittersPool.GetObjects([](GPUEmitter* emitter){
        if (emitter->GetDirty())
        {
            assert(emitter->GetParticleAllocation().IsValid());
            return true;
        }
        return false;
    });

    if (dirtyEmitters.size() == 0)
        return;

    // Update Emitter's Constant buffer and reset Indirect Draw entry
    for (GPUEmitter* emitter : dirtyEmitters)
    {
        emitter->ClearDirty();

        const uint32_t offset = emitter->GetEmitterIndexGPU() * sizeof(EmitterConstantData);
        const uint32_t size = sizeof(EmitterConstantData);

        uint8_t* data = mEmitterConstantBuffer->Map(offset, offset + size);

        const EmitterConstantData& constantData = emitter->GetConstantData();

        memcpy(data, &constantData, sizeof(EmitterConstantData));

        mEmitterConstantBuffer->Unmap(commandList);

        const uint32_t ioffset = emitter->GetEmitterIndexGPU() * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        const uint32_t isize = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

        data = mDrawIndirectBuffer->Map(ioffset, ioffset + isize);
        memset(data, 0, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
        reinterpret_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>(data)->IndexCountPerInstance = 6;
        mDrawIndirectBuffer->Unmap(commandList);
    }

    struct ResetConstants
    {
        uint32_t offset;
        uint32_t indicesCount;
    } constants;

    ShaderParametersLayout resetFreeIndicesLayout;
    resetFreeIndicesLayout.SetConstant(0, 0, sizeof(ResetConstants), D3D12_SHADER_VISIBILITY_ALL);
    resetFreeIndicesLayout.SetUAV(1, 0, D3D12_SHADER_VISIBILITY_ALL);

    ComputePipelineState resetFreeIndicesState;
    resetFreeIndicesState.SetCS(L"resetfreeindices");
    resetFreeIndicesState.Bind(commandList, resetFreeIndicesLayout);

    // Reset free indices buffer
    for (GPUEmitter* emitter : dirtyEmitters)
    {
        const uint32_t offset = static_cast<uint32_t>(emitter->GetParticleAllocation().Start);
        const uint32_t size = static_cast<uint32_t>(emitter->GetParticleAllocation().Size);

        constants.offset = offset;
        constants.indicesCount = size;

        ShaderParameters resetFreeIndicesParams;
        resetFreeIndicesParams.SetConstant(0, constants);
        resetFreeIndicesParams.SetUAV(1, *mFreeIndicesBuffer);
        resetFreeIndicesParams.Bind<false>(commandList, resetFreeIndicesLayout);

        uint32_t dispatchCount = Align(size, 64) / 64;
        commandList->Dispatch(dispatchCount, 1, 1);
    }

    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(1);

        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mFreeIndicesBuffer->GetResource()));

        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }
}

void GPUParticleSystem::UpdateEmitters(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters)
{
    PIXScopedEvent(commandList.Get(), 0, "UpdateEmitters");

    const uint32_t enabledEmittersCount = static_cast<uint32_t>(enabledEmitters.size());
    uint32_t* emitterData = reinterpret_cast<uint32_t*>(mEmitterIndexBuffer->Map(0, enabledEmittersCount * sizeof(uint32_t)));

    for (uint32_t i = 0; i < enabledEmitters.size(); ++i)
    {
        GPUEmitter* emitter = enabledEmitters[i];
        emitterData[i] = emitter->GetEmitterIndexGPU();
    }

    mEmitterIndexBuffer->Unmap(commandList);

    GlobalTimer& timer = Engine::Get().GetTimer();

    ShaderParametersLayout updateEmitterLayout;
    updateEmitterLayout.SetConstant(0, 0, sizeof(EmitterUpdateConstants), D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetSRV(2, 1, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetUAV(3, 1, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetUAV(4, 2, D3D12_SHADER_VISIBILITY_ALL);
    updateEmitterLayout.SetUAV(5, 3, D3D12_SHADER_VISIBILITY_ALL);

    ComputePipelineState updateEmitterState;
    updateEmitterState.SetCS(L"emitterupdate");
    updateEmitterState.Bind(commandList, updateEmitterLayout);

    EmitterUpdateConstants updateConstants;
    updateConstants.emittersCount = enabledEmittersCount;
    updateConstants.deltaTime = timer.GetDeltaTime();

    ShaderParameters updateEmitterParams;
    updateEmitterParams.SetConstant(0, updateConstants);
    updateEmitterParams.SetSRV(1, *mEmitterConstantBuffer);
    updateEmitterParams.SetSRV(2, *mEmitterIndexBuffer);
    updateEmitterParams.SetUAV(3, *mEmitterStatusBuffer);
    updateEmitterParams.SetUAV(4, *mDrawIndirectBuffer);
    updateEmitterParams.SetUAV(5, *mSpawnIndirectBuffer);
    updateEmitterParams.Bind<false>(commandList, updateEmitterLayout);

    const uint32_t dispatchCount = Align(enabledEmittersCount, 64) / 64;
    commandList->Dispatch(dispatchCount, 1, 1);

    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(3);

        mSpawnIndirectBuffer->SetCurrentUsage(BufferUsage::Indirect, barriers);
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mEmitterStatusBuffer->GetResource()));
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mDrawIndirectBuffer->GetResource()));

        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }
}

void GPUParticleSystem::SpawnParticles(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters)
{
    PIXScopedEvent(commandList.Get(), 0, "SpawnParticles");

    ShaderParametersLayout spawnLayout;
    spawnLayout.SetConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetSRV(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(3, 1, D3D12_SHADER_VISIBILITY_ALL);
    spawnLayout.SetUAV(4, 2, D3D12_SHADER_VISIBILITY_ALL);

    for (GPUEmitter* emitter : enabledEmitters)
    {
        GPUEmitterTemplate* emitterTemplate = GetEmitterTemplate(emitter->GetTemplateHandle());

        ComputePipelineState spawnState;
        spawnState.SetCS(emitterTemplate->GetSpawnShader());
        spawnState.Bind(commandList, spawnLayout);

        ShaderParameters spawnParams;
        spawnParams.SetConstant(0, emitter->GetEmitterIndexGPU());
        spawnParams.SetSRV(1, *mEmitterConstantBuffer);
        spawnParams.SetUAV(2, *mParticlesDataBuffer);
        spawnParams.SetUAV(3, *mEmitterStatusBuffer);
        spawnParams.SetUAV(4, *mFreeIndicesBuffer);
        spawnParams.Bind<false>(commandList, spawnLayout);

        const uint32_t dispatchOffset = emitter->GetEmitterIndexGPU() * sizeof(D3D12_DISPATCH_ARGUMENTS);
        commandList->ExecuteIndirect(Graphic::Get().GetDefaultDispatchCommandSignature(), 1, mSpawnIndirectBuffer->GetResource(), dispatchOffset, nullptr, 0);
    }

    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(4);

        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mParticlesDataBuffer->GetResource()));
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mEmitterStatusBuffer->GetResource()));
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mFreeIndicesBuffer->GetResource()));

        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }

}

void GPUParticleSystem::UpdateParticles(CommandList& commandList, const std::vector<GPUEmitter*>& enabledEmitters)
{
    PIXScopedEvent(commandList.Get(), 0, "UpdateParticles");

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
        GPUEmitterTemplate* emitterTemplate = GetEmitterTemplate(emitter->GetTemplateHandle());

        ComputePipelineState updateState;
        updateState.SetCS(emitterTemplate->GetUpdateShader());
        updateState.Bind(commandList, updateLayout);

        constants.emitterIndex = emitter->GetEmitterIndexGPU();

        ShaderParameters updateParams;
        updateParams.SetConstant(0, constants);
        updateParams.SetSRV(1, *mEmitterConstantBuffer);
        updateParams.SetUAV(2, *mParticlesDataBuffer);
        updateParams.SetUAV(3, *mEmitterStatusBuffer);
        updateParams.SetUAV(4, *mIndicesBuffer);
        updateParams.SetUAV(5, *mFreeIndicesBuffer);
        updateParams.SetUAV(6, *mDrawIndirectBuffer);
        updateParams.Bind<false>(commandList, updateLayout);

        commandList->Dispatch(emitter->GetMaxParticles(), 1, 1);
    }

    {
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(1);

        mDrawIndirectBuffer->SetCurrentUsage(BufferUsage::Indirect, barriers);

        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }
}

void GPUParticleSystem::DrawParticles(CommandList& commandList, GPUBuffer* cameraBuffer, Texture2D* renderTarget)
{
    PIXScopedEvent(commandList.Get(), 0, "DrawParticles");

    assert(cameraBuffer);
    assert(renderTarget);

    Sampler defaultSampler;

    ShaderParametersLayout drawLayout;
    drawLayout.SetConstant(0, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
    drawLayout.SetCBV(1, 1, D3D12_SHADER_VISIBILITY_VERTEX);
    drawLayout.SetSRV(2, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    drawLayout.SetSRV(3, 1, D3D12_SHADER_VISIBILITY_VERTEX);
    //drawLayout.SetSRV(3, 2, D3D12_SHADER_VISIBILITY_PIXEL);
    drawLayout.SetStaticSampler(0, defaultSampler, D3D12_SHADER_VISIBILITY_PIXEL);

    GraphicPipelineState drawState;
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

    std::vector<GPUEmitter*> emitters = mEmittersPool.GetObjects();
    for (GPUEmitter* emitter : emitters)
    {
        const uint32_t constant = static_cast<uint32_t>(emitter->GetParticleAllocation().Start);

        ShaderParameters drawParams;
        drawParams.SetConstant(0, constant);
        drawParams.SetCBV(1, *cameraBuffer);
        drawParams.SetSRV(2, *mParticlesDataBuffer);
        drawParams.SetSRV(3, *mIndicesBuffer);
        //drawParams.SetSRV(4, *texture);
        drawParams.Bind<true>(commandList, drawLayout);

        const uint32_t drawOffset = emitter->GetEmitterIndexGPU() * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
        commandList->ExecuteIndirect(Graphic::Get().GetDefaultDrawCommandSignature(), 1, mDrawIndirectBuffer->GetResource(), drawOffset, nullptr, 0);
    }
}
