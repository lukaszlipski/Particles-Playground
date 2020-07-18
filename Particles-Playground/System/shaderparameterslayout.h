#pragma once

class CommandList;
class Sampler;

struct RootParameters
{
    std::vector<CD3DX12_ROOT_PARAMETER1> Parameters;
    std::list<CD3DX12_DESCRIPTOR_RANGE1> Ranges;
    std::vector<CD3DX12_STATIC_SAMPLER_DESC> StaticSamplers;
};

class ShaderParametersLayout
{
    struct SingleRangeDesc
    {
        CD3DX12_DESCRIPTOR_RANGE1 Range;
        D3D12_SHADER_VISIBILITY Visibility;
    };

    struct ConstantDesc
    {
        CD3DX12_ROOT_PARAMETER1 Parameter;
    };

    using ParameterVar = std::variant<SingleRangeDesc, ConstantDesc>;

public:
    ShaderParametersLayout() = default;
    ~ShaderParametersLayout() = default;
    ShaderParametersLayout(const ShaderParametersLayout&) = default;
    ShaderParametersLayout(ShaderParametersLayout&&) = default;

    ShaderParametersLayout& SetCBV(uint32_t idx, uint32_t regIdx, D3D12_SHADER_VISIBILITY visibility);
    ShaderParametersLayout& SetSRV(uint32_t idx, uint32_t regIdx, D3D12_SHADER_VISIBILITY visibility);
    ShaderParametersLayout& SetUAV(uint32_t idx, uint32_t regIdx, D3D12_SHADER_VISIBILITY visibility);
    ShaderParametersLayout& SetConstant(uint32_t idx, uint32_t regIdx, uint32_t size, D3D12_SHADER_VISIBILITY visibility);
    ShaderParametersLayout& SetStaticSampler(uint32_t regIdx, Sampler& desc, D3D12_SHADER_VISIBILITY visibility);

    [[nodiscard]] uint32_t Hash() const;
    [[nodiscard]] RootParameters GetParameters() const;

    D3D12_SHADER_VISIBILITY GetVisibilityForParameterIndex(uint32_t idx);
    
private:
    std::map<uint32_t, ParameterVar> mParams;
    std::vector<CD3DX12_STATIC_SAMPLER_DESC> mStaticSamplers;

};
