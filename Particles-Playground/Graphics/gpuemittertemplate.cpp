#include "gpuemittertemplate.h"

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

GPUEmitterTemplate::GPUEmitterTemplate() 
{
    // #TODO: fix the way the default shader is obtained. The Index is invalid in the constructor
    SetSpawnShader(defaultSpawnLogic);
    SetUpdateShader(defaultUpdateLogic);
}

GPUEmitterTemplate& GPUEmitterTemplate::SetUpdateShader(std::string_view updateLogic)
{
    ShaderToken updateToken = { "TOKEN_UPDATE_LOGIC", updateLogic };
    ShaderHandle shader = ShaderManager::Get().GetShader(L"updateTemplate", ShaderType::Compute, L"main", GetIndex(), { updateToken });

    if (shader)
    {
        mUpdateShader = shader;
    }

    return *this;
}

GPUEmitterTemplate& GPUEmitterTemplate::SetSpawnShader(std::string_view spawnLogic)
{
    ShaderToken spawnToken = { "TOKEN_SPAWN_LOGIC", spawnLogic };
    ShaderHandle shader = ShaderManager::Get().GetShader(L"spawnTemplate", ShaderType::Compute, L"main", GetIndex(), { spawnToken });

    if (shader)
    {
        mSpawnShader = shader;
    }

    return *this;
}
