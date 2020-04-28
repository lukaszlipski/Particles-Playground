#include "commandlist.h"
#include "gpubuffer.h"
#include "graphic.h"
#include "gpudescriptorheap.h"

template<typename T>
ShaderParameters& ShaderParameters::SetConstant(uint32_t idx, const T& data)
{
    const uint32_t size = std::max(static_cast<uint32_t>(sizeof(T) / 4), 1U);
    RootConstant constant;
    constant.Data.resize(size);

    memcpy(constant.Data.data(), &data, sizeof(T));

    mParams[idx] = std::move(constant);

    return *this;
}

template <bool isGraphics>
void ShaderParameters::Bind(CommandList& commandList)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    for (const auto& [idx, var] : mParams)
    {
        if (std::holds_alternative<SingleDescriptor>(var))
        {
            const SingleDescriptor& descriptor = std::get<SingleDescriptor>(var);

            GPUBuffer* const buffer = descriptor.Resource;
            const DescriptorType type = descriptor.Type;

            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};

            switch(descriptor.Type)
            {
            case DescriptorType::CBV:
            {
                buffer->SetCurrentUsage(BufferUsage::Constant, barriers);
                cpuHandle = descriptor.Resource->GetCBV();
                break;
            }
            case DescriptorType::SRV:
            {
                buffer->SetCurrentUsage(BufferUsage::Structured, barriers);
                cpuHandle = descriptor.Resource->GetSRV();
                break;
            }
            case DescriptorType::UAV:
            {
                buffer->SetCurrentUsage(BufferUsage::UnorderedAccess, barriers);
                cpuHandle = descriptor.Resource->GetUAV();
                break;
            }
            default: assert(false);
            }

            GPUDescriptorHandleScoped gpuHandle = Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            Graphic::Get().GetDevice()->CopyDescriptorsSimple(1, gpuHandle, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            if constexpr (isGraphics) {
                commandList->SetGraphicsRootDescriptorTable(idx, gpuHandle);
            } else {
                commandList->SetComputeRootDescriptorTable(idx, gpuHandle);
}
        }
        else if (std::holds_alternative<RootConstant>(var))
        {
            const RootConstant& constant = std::get<RootConstant>(var);
            const uint32_t size = static_cast<uint32_t>(constant.Data.size());
            const uint32_t* data = constant.Data.data();

            if constexpr (isGraphics) {
                commandList->SetGraphicsRoot32BitConstants(idx, size, data, 0);
            } else {
                commandList->SetComputeRoot32BitConstants(idx, size, data, 0);
}
            
        }
    }

    if(!barriers.empty())
    {
        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }

}
