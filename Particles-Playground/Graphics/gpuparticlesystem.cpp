#include "Graphics/gpuparticlesystem.h"
#include "System/engine.h"

GPUParticleSystem::GPUParticleSystem() 
    : mParticlesAllocator(0, MaxParticles)
    , mEmittersPool(MaxEmitters)
    , mEmitterTemplatesPool(MaxEmitterTemplates)
    , mRNG(0xDEADC0DE)
{ }

void GPUParticleSystem::Init()
{
    mEmittersPool.Init();
    mEmitterTemplatesPool.Init();

    mParticlesDataBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(ParticleData)), MaxParticles, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mParticlesDataBuffer->SetDebugName(L"ParticlesDataBuffer");

    mFreeIndicesBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(int32_t)), MaxParticles, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mFreeIndicesBuffer->SetDebugName(L"FreeIndicesBuffer");

    mEmitterIndexBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(uint32_t)), MaxEmitters, BufferUsage::Structured | BufferUsage::UnorderedAccess);
    mEmitterIndexBuffer->SetDebugName(L"EmitterIndexBuffer");

    mEmitterConstantBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(EmitterConstantData)), MaxEmitters, BufferUsage::Structured | BufferUsage::CopyDst);
    mEmitterConstantBuffer->SetDebugName(L"EmitterConstantBuffer");

    mEmitterStatusBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(EmitterStatusData)), MaxEmitters, BufferUsage::Structured | BufferUsage::UnorderedAccess | BufferUsage::CopyDst);
    mEmitterStatusBuffer->SetDebugName(L"EmitterStatusBuffer");

    mDrawIndirectBuffer = std::make_unique<GPUBuffer>(static_cast<uint32_t>(sizeof(D3D12_DRAW_INDEXED_ARGUMENTS) + sizeof(uint32_t)), MaxEmitters, BufferUsage::Indirect | BufferUsage::UnorderedAccess | BufferUsage::CopyDst);
    mDrawIndirectBuffer->SetDebugName(L"DrawIndirectBuffer");
}

void GPUParticleSystem::Free()
{
    mEmitterTemplatesPool.Free();
    mEmittersPool.Free();

    mDrawIndirectBuffer.reset();
    mEmitterConstantBuffer.reset();
    mEmitterStatusBuffer.reset();
    mEmitterIndexBuffer.reset();
    mFreeIndicesBuffer.reset();
    mParticlesDataBuffer.reset();
}

void GPUParticleSystem::PostUpdate()
{
    std::vector<GPUEmitter*> dirtyEmitters = GetDirtyEmitters();

    for (GPUEmitter* emitter : dirtyEmitters)
    {
        emitter->ClearDirty();
    }
}

std::vector<GPUEmitter*> GPUParticleSystem::GetEnabledEmitters() const
{
    return mEmittersPool.GetObjects([](GPUEmitter* emitter) {
        return emitter->GetEnabled();
        });
}

std::vector<GPUEmitter*> GPUParticleSystem::GetDirtyEmitters() const
{
    return mEmittersPool.GetObjects([](GPUEmitter* emitter) {
        if (emitter->GetDirty())
        {
            Assert(emitter->GetParticleAllocation().IsValid());
            return true;
        }
        return false;
        });
}

std::vector<GPUEmitter*> GPUParticleSystem::GetEmitters() const
{
    return mEmittersPool.GetObjects();
}

