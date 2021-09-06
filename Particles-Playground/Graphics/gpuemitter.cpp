#include "gpuemitter.h"
#include "gpuparticlesystem.h"


const char* defaultUpdateLogic = "particle.position += particle.velocity * Constants.deltaTime;\n\
particle.velocity += float3(0, -9.8f, 0) * Constants.deltaTime;\n\
particle.color.a = max(0.0f, particle.lifeTime / emitterConstant.lifeTime);\n\
particle.lifeTime -= Constants.deltaTime;\n";

const char* defaultSpawnLogic = "float phi = (float(particleIndex) / emitterConstant.maxParticles) * 3.14f;\n\
particle.position = float3(0.0f, 0.0f, 0.0f) + emitterConstant.position;\n\
particle.color = emitterConstant.color;\n\
particle.lifeTime = emitterConstant.lifeTime;\n\
particle.velocity = float3(cos(phi), sin(phi), 0) * 15.0f;\n\
particle.scale = 1.0f;\n";

GPUEmitter::GPUEmitter()
{
    SetSpawnShader(defaultSpawnLogic);
    SetUpdateShader(defaultUpdateLogic);
}

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

GPUEmitter& GPUEmitter::SetPosition(const XMFLOAT3& position)
{
    mConstantData.Position = position;
    SetDitry();
    return *this;
}

GPUEmitter& GPUEmitter::SetUpdateShader(std::string_view updateLogic)
{
    ShaderToken updateToken = { "TOKEN_UPDATE_LOGIC", updateLogic };
    ShaderHandle shader = ShaderManager::Get().GetShader(L"updateTemplate", ShaderType::Compute, L"main", GetEmitterIndexGPU(), { updateToken });

    if (shader)
    {
        mUpdateShader = shader;
    }

    return *this;
}

GPUEmitter& GPUEmitter::SetSpawnShader(std::string_view spawnLogic)
{
    ShaderToken spawnToken = { "TOKEN_SPAWN_LOGIC", spawnLogic };
    ShaderHandle shader = ShaderManager::Get().GetShader(L"spawnTemplate", ShaderType::Compute, L"main", GetEmitterIndexGPU(), { spawnToken });

    if (shader)
    {
        mSpawnShader = shader;
    }

    return *this;
}
