#include "shaderparameters.h"
#include "gpubuffer.h"
#include "texture.h"

// ---
template<typename T>
ShaderParameters& ShaderParameters::SetCBV(uint32_t idx, T& resource)
{
    mParams[idx] = SingleDescriptor<T>{ DescriptorType::CBV, &resource };
    return *this;
}

template ShaderParameters& ShaderParameters::SetCBV<GPUBuffer>(uint32_t idx, GPUBuffer& resource);

// ---
template<typename T>
ShaderParameters& ShaderParameters::SetSRV(uint32_t idx, T& resource)
{
    mParams[idx] = SingleDescriptor<T>{ DescriptorType::SRV, &resource };
    return *this;
}

template ShaderParameters& ShaderParameters::SetSRV<GPUBuffer>(uint32_t idx, GPUBuffer& resource);
template ShaderParameters& ShaderParameters::SetSRV<Texture2D>(uint32_t idx, Texture2D& resource);

// ---
template<typename T>
ShaderParameters& ShaderParameters::SetUAV(uint32_t idx, T& resource)
{
    mParams[idx] = SingleDescriptor<T>{ DescriptorType::UAV, &resource };
    return *this;
}

template ShaderParameters& ShaderParameters::SetUAV<GPUBuffer>(uint32_t idx, GPUBuffer& resource);

// ---
template <bool isGraphics>
void ShaderParameters::Bind(CommandList& commandList, ShaderParametersLayout& layout)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    for (const auto& [idx, var] : mParams)
    {
        if (std::holds_alternative<RootConstant>(var))
        {
            const RootConstant& constant = std::get<RootConstant>(var);
            const uint32_t size = static_cast<uint32_t>(constant.Data.size());
            const uint32_t* data = constant.Data.data();

            if constexpr (isGraphics) {
                commandList->SetGraphicsRoot32BitConstants(idx, size, data, 0);
            }
            else {
                commandList->SetComputeRoot32BitConstants(idx, size, data, 0);
            }
        }
        else
        {
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};

            if (std::holds_alternative<SingleDescriptor<GPUBuffer>>(var))
            {
                const SingleDescriptor<GPUBuffer>& descriptor = std::get<SingleDescriptor<GPUBuffer>>(var);
                const DescriptorType type = descriptor.Type;
                GPUBuffer* resource = descriptor.Resource;

                switch (type)
                {
                    case DescriptorType::CBV: 
                    {
                        resource->SetCurrentUsage(BufferUsage::Constant, barriers);
                        cpuHandle = resource->GetCBV(); 
                        break; 
                    }
                    case DescriptorType::SRV: 
                    {
                        resource->SetCurrentUsage(BufferUsage::Structured, barriers);
                        cpuHandle = resource->GetSRV(); 
                        break; 
                    }
                    case DescriptorType::UAV: 
                    {
                        resource->SetCurrentUsage(BufferUsage::UnorderedAccess, barriers);
                        cpuHandle = resource->GetUAV(); 
                        break; 
                    }
                    default: { assert(false); }
                }

            }
            else if (std::holds_alternative<SingleDescriptor<Texture2D>>(var))
            {
                const SingleDescriptor<Texture2D>& descriptor = std::get<SingleDescriptor<Texture2D>>(var);
                const DescriptorType type = descriptor.Type;
                Texture2D* resource = descriptor.Resource;
                const bool isPixelShader = layout.GetVisibilityForParameterIndex(idx) == D3D12_SHADER_VISIBILITY_PIXEL;
                
                switch (type)
                {
                case DescriptorType::SRV: 
                {
                    resource->SetCurrentUsage(TextureUsage::ShaderResource, isPixelShader, barriers);       
                    cpuHandle = resource->GetSRV();
                    break; 
                }
                default: { assert(false); }
                }
            }
            else { assert(false); }

            GPUDescriptorHandleScoped gpuHandle = Graphic::Get().GetGPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            Graphic::Get().GetDevice()->CopyDescriptorsSimple(1, gpuHandle, cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            if constexpr (isGraphics) {
                commandList->SetGraphicsRootDescriptorTable(idx, gpuHandle);
            }
            else {
                commandList->SetComputeRootDescriptorTable(idx, gpuHandle);
            }

        }
        
    }

    if(!barriers.empty())
    {
        commandList->ResourceBarrier(static_cast<uint32_t>(barriers.size()), barriers.data());
    }

}

template void ShaderParameters::Bind<true>(CommandList& commandList, ShaderParametersLayout& layout);
template void ShaderParameters::Bind<false>(CommandList& commandList, ShaderParametersLayout& layout);
