#include "gpuemitter.h"
#include "gpuparticlesystem.h"

GPUEmitter::~GPUEmitter()
{
    assert(!mEmitterAllocation.IsValid());
    assert(!mParticleAllocation.IsValid());
}

void GPUEmitter::AllocateResources(uint32_t maxParticles, GPUParticleSystem& particleSystem)
{
    mEmitterAllocation = particleSystem.AllocateEmitter();
    mParticleAllocation = particleSystem.AllocateParticles(maxParticles);

    mConstantData.MaxParticles = maxParticles;
    mConstantData.IndicesOffset = static_cast<uint32_t>(mParticleAllocation.Start);
}

void GPUEmitter::FreeResources(GPUParticleSystem& particleSystem)
{
    particleSystem.FreeEmitter(mEmitterAllocation);
    particleSystem.FreeParticles(mParticleAllocation);
}

GPUEmitter& GPUEmitter::SetSpawnRate(float spawnRate)
{
    mConstantData.SpawnRate = spawnRate;
    SetDitry();
    return *this;
}

GPUEmitter& GPUEmitter::SetParticleLifeTime(float lifeTime)
{
    mConstantData.LifeTime = lifeTime;
    SetDitry();
    return *this;
}

GPUEmitter& GPUEmitter::SetParticleColor(const XMFLOAT4& color)
{
    mConstantData.Color = color;
    SetDitry();
    return *this;   
}
