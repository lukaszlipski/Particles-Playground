#pragma once

class GPUBuffer;
class CommandList;

class ShaderParameters
{
    struct RootConstant
    {
        std::vector<uint32_t> Data;
    };

    enum class DescriptorType
    {
        CBV = 0,
        SRV,
        UAV
    };

    struct SingleDescriptor
    {
        DescriptorType Type;
        GPUBuffer* Resource = nullptr;
    };

    using ParameterVar = std::variant<SingleDescriptor, RootConstant>;

public:
    ShaderParameters() = default;
    ~ShaderParameters() = default;
    ShaderParameters(const ShaderParameters&) = default;
    ShaderParameters(ShaderParameters&&) = default;

    ShaderParameters& SetCBV(uint32_t idx, GPUBuffer& buffer);
    ShaderParameters& SetSRV(uint32_t idx, GPUBuffer& buffer);
    ShaderParameters& SetUAV(uint32_t idx, GPUBuffer& buffer);

    template<typename T>
    ShaderParameters& SetConstant(uint32_t idx, const T& data);

    template<bool isGraphics>
    void Bind(CommandList& commandList);

private:
    std::map<uint32_t, ParameterVar> mParams;
    
};

#include "shaderparameters.inl"
