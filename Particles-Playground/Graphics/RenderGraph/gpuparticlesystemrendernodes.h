#pragma once
#include "System/engine.h"

class GPUParticleSystemUpdateDirtyEmittersNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        context.InputOutputGPUBuffer(RESOURCEID("EmitterConstantBuffer"), RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"), BufferUsage::CopyDst);
        context.InputOutputGPUBuffer(RESOURCEID("EmitterStatusBuffer"), RESOURCEID("UpdateDirtyEmitters_EmitterStatusBuffer"), BufferUsage::CopyDst);
        context.InputOutputGPUBuffer(RESOURCEID("DrawIndirectBuffer"), RESOURCEID("UpdateDirtyEmitters_DrawIndirectBuffer"), BufferUsage::CopyDst);
    }

    void Execute(const RGExecuteContext& context) override;
};

class GPUParticleSystemDirtyEmittersFreeIndicesNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        context.InputOutputGPUBuffer(RESOURCEID("FreeIndicesBuffer"), RESOURCEID("DirtyEmittersFreeIndices_FreeIndicesBuffer"), BufferUsage::UnorderedAccess);
    }

    void Execute(const RGExecuteContext& context) override;
};

class GPUParticleSystemUpdateEmittersNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        RGNewGPUBuffer& newBuffer = context.OutputGPUBuffer(RESOURCEID("SpawnIndirectBuffer"), BufferUsage::UnorderedAccess);
        newBuffer.mElemSize = static_cast<uint32_t>(sizeof(D3D12_DISPATCH_ARGUMENTS) + sizeof(uint32_t));
        newBuffer.mNumElems = GPUParticleSystem::MaxEmitters;
        newBuffer.mUsage = BufferUsage::Indirect | BufferUsage::UnorderedAccess;

        context.InputGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"), BufferUsage::Structured);
        context.InputGPUBuffer(RESOURCEID("EmitterIndexBuffer"), BufferUsage::Structured);
        context.InputOutputGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterStatusBuffer"), RESOURCEID("UpdateEmitters_EmitterStatusBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("UpdateDirtyEmitters_DrawIndirectBuffer"), RESOURCEID("UpdateEmitters_DrawIndirectBuffer"), BufferUsage::UnorderedAccess);
    }

    void Execute(const RGExecuteContext& context) override;
};

class GPUParticleSystemUpdateParticlesNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        RGNewGPUBuffer& newBuffer = context.OutputGPUBuffer(RESOURCEID("IndicesBuffer"), BufferUsage::UnorderedAccess);
        newBuffer.mElemSize = static_cast<uint32_t>(sizeof(int32_t));
        newBuffer.mNumElems = GPUParticleSystem::MaxParticles;
        newBuffer.mUsage = BufferUsage::Structured | BufferUsage::UnorderedAccess;

        context.InputGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"), BufferUsage::Structured);
        context.InputOutputGPUBuffer(RESOURCEID("ParticlesDataBuffer"), RESOURCEID("Update_ParticlesDataBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("UpdateEmitters_EmitterStatusBuffer"), RESOURCEID("Update_EmitterStatusBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("DirtyEmittersFreeIndices_FreeIndicesBuffer"), RESOURCEID("Update_FreeIndicesBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("UpdateEmitters_DrawIndirectBuffer"), RESOURCEID("Update_DrawIndirectBuffer"), BufferUsage::UnorderedAccess);
    }

    void Execute(const RGExecuteContext& context) override;
};

class GPUParticleSystemSpawnParticlesNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        context.InputGPUBuffer(RESOURCEID("UpdateDirtyEmitters_EmitterConstantBuffer"), BufferUsage::Structured);
        context.InputGPUBuffer(RESOURCEID("SpawnIndirectBuffer"), BufferUsage::Indirect);
        context.InputOutputGPUBuffer(RESOURCEID("Update_ParticlesDataBuffer"), RESOURCEID("Spawn_ParticlesDataBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("Update_FreeIndicesBuffer"), RESOURCEID("Spawn_FreeIndicesBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("IndicesBuffer"), RESOURCEID("Spawn_IndicesBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("Update_DrawIndirectBuffer"), RESOURCEID("Spawn_DrawIndirectBuffer"), BufferUsage::UnorderedAccess);
        context.InputOutputGPUBuffer(RESOURCEID("Update_EmitterStatusBuffer"), RESOURCEID("Spawn_EmitterStatusBuffer"), BufferUsage::UnorderedAccess);
    }

    void Execute(const RGExecuteContext& context) override;
};

class GPUParticleSystemDrawParticlesNode : public IRenderNodeBase
{
public:
    void Setup(RGSetupContext& context) override
    {
        RGNewTexture2D& newTexture = context.OutputTexture2D(RESOURCEID("RenderTarget"), TextureUsage::RenderTarget);
        newTexture.mWidth = Window::Get().GetWidth();
        newTexture.mHeight = Window::Get().GetHeight();
        newTexture.mFormat = TextureFormat::R8G8B8A8;
        newTexture.mUsage = TextureUsage::RenderTarget | TextureUsage::ShaderResource;

        context.InputGPUBuffer(RESOURCEID("SceneBuffer"), BufferUsage::Structured);
        context.InputGPUBuffer(RESOURCEID("Spawn_ParticlesDataBuffer"), BufferUsage::Structured);
        context.InputGPUBuffer(RESOURCEID("Spawn_IndicesBuffer"), BufferUsage::Structured);
        context.InputGPUBuffer(RESOURCEID("Spawn_DrawIndirectBuffer"), BufferUsage::Indirect);
    }

    void Execute(const RGExecuteContext& context) override;
};
