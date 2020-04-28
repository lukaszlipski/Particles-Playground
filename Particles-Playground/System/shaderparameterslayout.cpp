#include "System/shaderparameterslayout.h"
#include "System/psomanager.h"
#include "Utilities/memory.h"

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

uint32_t ShaderParametersLayout::Hash() const
{
    if (mParams.empty()) { return 0; }

    auto maxElem = std::max_element(mParams.begin(), mParams.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    const uint32_t biggestIdx = maxElem->first;
    
    // Allocate on the stack enough memory to hold an aligned array that will be used to calculate a hash value
    const uint32_t size = sizeof(ParameterVar) * (biggestIdx + 1);
    const uint32_t extendedSize = AlignPow2(static_cast<uint32_t>(size), 4U);
    void* addr = alloca(extendedSize + 4 - 1);
    memset(addr, 0, extendedSize + 4 - 1);
    
    // Prepare pointers
    uint8_t* start = reinterpret_cast<uint8_t*>(AlignPow2(reinterpret_cast<uintptr_t>(addr), 4));
    uint8_t* end = start + AlignPow2(size, 4);
    ParameterVar* parameters = reinterpret_cast<ParameterVar*>(start);
    
    // Fill the aligned array
    for (const auto& [idx, param] : mParams)
    {
        ParameterVar* current = reinterpret_cast<ParameterVar*>(start + idx);
        *current = param;
    }

    return HashRange(reinterpret_cast<uint32_t*>(start), reinterpret_cast<uint32_t*>(end));
}

RootParameters ShaderParametersLayout::GetParameters() const
{
    if (mParams.empty()) { return {}; }

    auto maxElem = std::max_element(mParams.begin(), mParams.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    const uint32_t biggestIdx = maxElem->first;

    RootParameters result{};
    auto& [rootParams, ranges] = result;
    rootParams.resize(biggestIdx + 1);

    for (auto& [idx, param] : mParams)
    {
        if (std::holds_alternative<SingleRangeDesc>(param))
        {
            const SingleRangeDesc& range = std::get<SingleRangeDesc>(param);
            ranges.push_back(range.Range);
            rootParams[idx].InitAsDescriptorTable(1, &ranges.back(), range.Visibility);
        }
        else if (std::holds_alternative<ConstantDesc>(param))
        {
            const ConstantDesc& range = std::get<ConstantDesc>(param);
            rootParams[idx] = range.Parameter;
        }
        else
        {
            assert(0); // Unsupported parameter type
        }
    }

    return result;
}
