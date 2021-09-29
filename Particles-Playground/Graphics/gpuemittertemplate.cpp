#include "Graphics/gpuemittertemplate.h"

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
    Assert(SetUpdateShader(defaultUpdateLogic) == std::nullopt);
    Assert(SetSpawnShader(defaultSpawnLogic) == std::nullopt);
}

GPUEmitterTemplate::~GPUEmitterTemplate()
{
    ShaderManager::Get().FreeShader(mUpdateShader);
    ShaderManager::Get().FreeShader(mSpawnShader);
}

std::optional<std::string> GPUEmitterTemplate::SetUpdateShader(std::string_view updateLogic)
{
    ShaderToken updateToken = { "TOKEN_UPDATE_LOGIC", updateLogic };
    ShaderCompilationResult result = ShaderManager::Get().CompileShader(L"updateTemplate", ShaderType::Compute, L"main", { updateToken });

    if (result.IsValid())
    {
        ShaderManager::Get().FreeShader(mUpdateShader);
        mUpdateShader = result.GetHandle();
    }
    else
    {
        return std::optional<std::string>(result.GetError());
    }

    return std::nullopt;
}

std::optional<std::string> GPUEmitterTemplate::SetSpawnShader(std::string_view spawnLogic)
{
    ShaderToken spawnToken = { "TOKEN_SPAWN_LOGIC", spawnLogic };
    ShaderCompilationResult result = ShaderManager::Get().CompileShader(L"spawnTemplate", ShaderType::Compute, L"main", { spawnToken });

    if (result.IsValid())
    {
        ShaderManager::Get().FreeShader(mSpawnShader);
        mSpawnShader = result.GetHandle();
    }
    else
    {
        return std::optional<std::string>(result.GetError());
    }

    return std::nullopt;
}
