#pragma once
#include "System/shadermanager.h"
#include "Utilities/objectpool.h"

class GPUEmitterTemplate : public IObject<GPUEmitterTemplate>
{
public:
    GPUEmitterTemplate();
    ~GPUEmitterTemplate();

    GPUEmitterTemplate(const GPUEmitterTemplate&) = delete;
    GPUEmitterTemplate(GPUEmitterTemplate&& rhs) = delete;
    GPUEmitterTemplate& operator=(const GPUEmitterTemplate&) = delete;
    GPUEmitterTemplate& operator=(GPUEmitterTemplate&& rhs) = delete;

    std::optional<std::string> SetUpdateShader(std::string_view updateLogic);
    std::optional<std::string> SetSpawnShader(std::string_view spawnLogic);

    inline ShaderHandle GetUpdateShader() const { return mUpdateShader; }
    inline ShaderHandle GetSpawnShader() const { return mSpawnShader; }

private:
    ShaderHandle mUpdateShader;
    ShaderHandle mSpawnShader;
};

using GPUEmitterTemplateHandle = ObjectHandle<GPUEmitterTemplate>;
