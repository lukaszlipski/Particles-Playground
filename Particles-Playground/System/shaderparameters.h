#pragma once

class Texture2D;
class GPUBuffer;
class CommandList;
class ShaderParametersLayout;

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

    template<typename ResourceT>
    struct SingleDescriptor
    {
        DescriptorType Type;
        ResourceT* Resource = nullptr;
    };

    using ParameterVar = std::variant<SingleDescriptor<GPUBuffer>, SingleDescriptor<Texture2D>, RootConstant>;

public:
    ShaderParameters() = default;
    ~ShaderParameters() = default;
    ShaderParameters(const ShaderParameters&) = default;
    ShaderParameters(ShaderParameters&&) = default;

    template<typename T>
    ShaderParameters& SetCBV(uint32_t idx, T& resource);

    template<typename T>
    ShaderParameters& SetSRV(uint32_t idx, T& resource);

    template<typename T>
    ShaderParameters& SetUAV(uint32_t idx, T& resource);

    template<typename T>
    ShaderParameters& SetConstant(uint32_t idx, const T& data);

    template<bool isGraphics>
    void Bind(CommandList& commandList, ShaderParametersLayout& layout);

private:
    std::map<uint32_t, ParameterVar> mParams;
    
};

#include "shaderparameters.inl"
