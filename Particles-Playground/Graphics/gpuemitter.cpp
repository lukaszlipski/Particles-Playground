#include "Graphics/gpuemitter.h"
#include "Graphics/gpuparticlesystem.h"

GPUEmitter::GPUEmitter(GPUParticleSystem* particleSystem, GPUEmitterTemplateHandle emitterTemplate, uint32_t maxParticles)
    : mParticleSystem(particleSystem)
    , mTemplateHandle(emitterTemplate)
{
    mParticleAllocation = mParticleSystem->AllocateParticles(maxParticles);

    mConstantData.MaxParticles = maxParticles;
    mConstantData.IndicesOffset = static_cast<uint32_t>(mParticleAllocation.Start);

    mTemplateHandle = emitterTemplate;
}

GPUEmitter::~GPUEmitter()
{
    mParticleSystem->FreeParticles(mParticleAllocation);
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

GPUEmitter& GPUEmitter::SetPosition(const XMFLOAT3& position)
{
    mConstantData.Position = position;
    SetDitry();
    return *this;
}
