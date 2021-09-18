#pragma once
#include "System/shadermanager.h"
#include "Utilities/objectpool.h"

class GPUEmitterTemplate : public IObject<GPUEmitterTemplate>
{
public:
    GPUEmitterTemplate();
    ~GPUEmitterTemplate() = default;

    GPUEmitterTemplate(const GPUEmitterTemplate&) = delete;
    GPUEmitterTemplate(GPUEmitterTemplate&& rhs) = delete;
    GPUEmitterTemplate& operator=(const GPUEmitterTemplate&) = delete;
    GPUEmitterTemplate& operator=(GPUEmitterTemplate&& rhs) = delete;

    GPUEmitterTemplate& SetUpdateShader(std::string_view updateLogic);
    GPUEmitterTemplate& SetSpawnShader(std::string_view spawnLogic);

    inline ShaderHandle GetUpdateShader() const { return mUpdateShader; }
    inline ShaderHandle GetSpawnShader() const { return mSpawnShader; }

private:
    ShaderHandle mUpdateShader = nullptr;
    ShaderHandle mSpawnShader = nullptr;
};

using GPUEmitterTemplateHandle = ObjectHandle<GPUEmitterTemplate>;
