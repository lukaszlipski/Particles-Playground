#include "shaderparameters.h"
#include "gpubuffer.h"

ShaderParameters& ShaderParameters::SetCBV(uint32_t idx, GPUBuffer& buffer)
{
    mParams[idx] = SingleDescriptor{ DescriptorType::CBV, &buffer };
    return *this;
}

ShaderParameters& ShaderParameters::SetSRV(uint32_t idx, GPUBuffer& buffer)
{
    mParams[idx] = SingleDescriptor{ DescriptorType::SRV, &buffer };
    return *this;
}

ShaderParameters& ShaderParameters::SetUAV(uint32_t idx, GPUBuffer& buffer)
{
    mParams[idx] = SingleDescriptor{ DescriptorType::UAV, &buffer };
    return *this;
}
