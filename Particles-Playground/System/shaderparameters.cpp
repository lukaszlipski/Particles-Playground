#include "shaderparameters.h"
#include "gpubuffer.h"
#include "graphic.h"
#include "gpudescriptorheap.h"
#include "commandlist.h"

ShaderParameters& ShaderParameters::SetCBV(uint32_t idx, GPUBuffer& buffer)
{
    mParams[idx] = buffer.GetCBV();
    return *this;
}

ShaderParameters& ShaderParameters::SetSRV(uint32_t idx, GPUBuffer& buffer)
{
    mParams[idx] = buffer.GetSRV();
    return *this;
}

ShaderParameters& ShaderParameters::SetUAV(uint32_t idx, GPUBuffer& buffer)
{
    mParams[idx] = buffer.GetUAV();
    return *this;
}

void ShaderParameters::Bind(CommandList& commandList)
{
    for (const auto& [idx, var]: mParams)
    {
        if (std::holds_alternative<D3D12_CPU_DESCRIPTOR_HANDLE>(var))
        {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = std::get<D3D12_CPU_DESCRIPTOR_HANDLE>(var);

            GPUDescriptorHandleScoped gpuHandle = Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            Graphic::Get().GetDevice()->CopyDescriptorsSimple(1, gpuHandle, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            commandList->SetGraphicsRootDescriptorTable(idx, gpuHandle);
        }
        else if (std::holds_alternative<RootConstant>(var))
        {
            const RootConstant& constant = std::get<RootConstant>(var);
            const uint32_t size = static_cast<uint32_t>(constant.Data.size());
            const uint32_t* data = constant.Data.data();

            commandList->SetGraphicsRoot32BitConstants(idx, size, data, 0);
        }
    }
}
