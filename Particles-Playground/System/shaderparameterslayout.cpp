#include "System/shaderparameterslayout.h"
#include "System/sampler.h"
#include "System/gpudescriptorheap.h"
#include "Utilities/memory.h"
#include "Shaders/bindlesscommon.hlsli"

const D3D12_DESCRIPTOR_RANGE_FLAGS g_BindlessFlags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
std::array<CD3DX12_DESCRIPTOR_RANGE1, 3> g_BindlessRanges = {
    CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GPUDescriptorHeap::BindlessDescriptorNum, 0, BindlessDescriptorRegisterSpace, g_BindlessFlags, 0),
    CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, GPUDescriptorHeap::BindlessDescriptorNum, 0, BindlessDescriptorRegisterSpace, g_BindlessFlags, 0),
    CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GPUDescriptorHeap::BindlessDescriptorNum, 0, BindlessDescriptorRegisterSpace, g_BindlessFlags, 0)
};

ShaderParametersLayout& ShaderParametersLayout::SetCBV(uint32_t idx, uint32_t regIdx, D3D12_SHADER_VISIBILITY visibility)
{
    SingleRangeDesc desc{};
    desc.Range = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, regIdx);
    desc.Visibility = visibility;

    mParams[idx] = desc;
    return *this;
}

ShaderParametersLayout& ShaderParametersLayout::SetSRV(uint32_t idx, uint32_t regIdx, D3D12_SHADER_VISIBILITY visibility)
{
    SingleRangeDesc desc{};
    desc.Range = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, regIdx);
    desc.Visibility = visibility;

    mParams[idx] = desc;
    return *this;
}

ShaderParametersLayout& ShaderParametersLayout::SetUAV(uint32_t idx, uint32_t regIdx, D3D12_SHADER_VISIBILITY visibility)
{
    SingleRangeDesc desc{};
    desc.Range = CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, regIdx);
    desc.Visibility = visibility;

    mParams[idx] = desc;
    return *this;
}

ShaderParametersLayout& ShaderParametersLayout::SetConstant(uint32_t idx, uint32_t regIdx, uint32_t size, D3D12_SHADER_VISIBILITY visibility)
{
    ConstantDesc desc{};
    desc.Parameter.InitAsConstants(size, regIdx, 0, visibility);

    mParams[idx] = desc;
    return *this;
}

ShaderParametersLayout& ShaderParametersLayout::SetStaticSampler(uint32_t regIdx, Sampler& sampler, D3D12_SHADER_VISIBILITY visibility)
{
    const D3D12_SAMPLER_DESC desc = sampler.GetDesc();
    CD3DX12_STATIC_SAMPLER_DESC& newElem = mStaticSamplers.emplace_back();
    newElem.Init(regIdx, desc.Filter, desc.AddressU, desc.AddressV, desc.AddressW,
                              desc.MipLODBias,
                              desc.MaxAnisotropy, desc.ComparisonFunc,
                              D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK, desc.MinLOD,
                              desc.MaxLOD, visibility, 0);
    
    return *this;
}

ShaderParametersLayout& ShaderParametersLayout::SetBindlessHeap(uint32_t idx)
{
    Assert(idx != std::numeric_limits<uint32_t>::max());
    mBindlessIndex = idx;
    return *this;
}

uint32_t ShaderParametersLayout::Hash() const
{
    if (mParams.empty()) { return 0; }

    const auto maxElem = std::max_element(mParams.begin(), mParams.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    const uint32_t biggestIdx = maxElem->first;
    
    // Allocate on the stack enough memory to hold an aligned array that will be used to calculate a hash value
    const uint32_t parametersSize = sizeof(ParameterVar) * (biggestIdx + 1);
    const uint32_t staticSamplerSize = sizeof(CD3DX12_STATIC_SAMPLER_DESC) * static_cast<uint32_t>(mStaticSamplers.size());
    const uint32_t bindlessSize = sizeof(uint32_t);

    const uint32_t size = parametersSize + staticSamplerSize + bindlessSize;
    const uint32_t extendedSize = AlignPow2(static_cast<uint32_t>(size), 4U);
    void* addr = _malloca(extendedSize);
    Assert(addr);

    memset(addr, 0, extendedSize);
    
    // Prepare pointers
    uint8_t* start = reinterpret_cast<uint8_t*>(AlignPow2(reinterpret_cast<uintptr_t>(addr), 4));
    uint8_t* end = start + AlignPow2(size, 4);

    // Fill the aligned array
    for (const auto& [idx, param] : mParams)
    {
        ParameterVar* current = reinterpret_cast<ParameterVar*>(start) + idx;
        *current = param;
    }

    for (uint32_t i = 0; i < mStaticSamplers.size(); ++i)
    {
        CD3DX12_STATIC_SAMPLER_DESC* current = reinterpret_cast<CD3DX12_STATIC_SAMPLER_DESC*>(start + parametersSize) + i;
        *current = mStaticSamplers[i];
    }

    *reinterpret_cast<uint32_t*>(start + parametersSize + staticSamplerSize) = GetBindlessHeapIndex();

    return HashRange(reinterpret_cast<uint32_t*>(start), reinterpret_cast<uint32_t*>(end));
}

RootParameters ShaderParametersLayout::GetParameters() const
{
    if (mParams.empty()) { return {}; }

    const auto maxElem = std::max_element(mParams.begin(), mParams.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    uint32_t biggestIdx = maxElem->first;
    if (!Graphic::Get().SupportsResourceDescriptorHeap() && HasBindlessHeap())
    {
        biggestIdx = std::max(maxElem->first, GetBindlessHeapIndex());
    }

    RootParameters result{};
    result.Parameters.resize(biggestIdx + 1);

    for (auto& [idx, param] : mParams)
    {
        if (std::holds_alternative<SingleRangeDesc>(param))
        {
            const SingleRangeDesc& range = std::get<SingleRangeDesc>(param);
            result.Ranges.push_back(range.Range);
            result.Parameters[idx].InitAsDescriptorTable(1, &result.Ranges.back(), range.Visibility);
        }
        else if (std::holds_alternative<ConstantDesc>(param))
        {
            const ConstantDesc& constant = std::get<ConstantDesc>(param);
            result.Parameters[idx] = constant.Parameter;
        }
        else
        {
            Assert(0); // Unsupported parameter type
        }
    }

    if (!Graphic::Get().SupportsResourceDescriptorHeap() && HasBindlessHeap())
    {
        result.Parameters[GetBindlessHeapIndex()].InitAsDescriptorTable(static_cast<uint32_t>(g_BindlessRanges.size()), g_BindlessRanges.data(), D3D12_SHADER_VISIBILITY_ALL);
    }

    result.StaticSamplers = mStaticSamplers;
    
    return result;
}

D3D12_SHADER_VISIBILITY ShaderParametersLayout::GetVisibilityForParameterIndex(uint32_t idx)
{
    Assert(mParams.count(idx));

    const ParameterVar& param = mParams[idx];
    
    if (std::holds_alternative<SingleRangeDesc>(param))
    {
        const SingleRangeDesc& range = std::get<SingleRangeDesc>(param);
        return range.Visibility;
    }
    else if (std::holds_alternative<ConstantDesc>(param))
    {
        const ConstantDesc& constant = std::get<ConstantDesc>(param);
        return constant.Parameter.ShaderVisibility;
    }
    
    Assert(0); // Unsupported parameter type
    return D3D12_SHADER_VISIBILITY_ALL;
}
