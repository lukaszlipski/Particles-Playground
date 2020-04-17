#pragma once

class GPUBuffer;
class CommandList;

class ShaderParameters
{
    struct RootConstant
    {
        std::vector<uint32_t> Data;
    };

    using ParameterVar = std::variant<D3D12_CPU_DESCRIPTOR_HANDLE, RootConstant>;

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

    void Bind(CommandList& commandList);

private:
    std::map<uint32_t, ParameterVar> mParams;
    
};

template<typename T>
ShaderParameters& ShaderParameters::SetConstant(uint32_t idx, const T& data)
{
    const uint32_t size = std::max(static_cast<uint32_t>(sizeof(T) / 4), 1U);
    RootConstant constant;
    constant.Data.resize(size);
    
    memcpy(constant.Data.data() , &data, sizeof(T));

    mParams[idx] = std::move(constant);

    return *this;
}
