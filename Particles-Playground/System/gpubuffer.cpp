#include "gpubuffer.h"
#include "commandlist.h"
#include "cpudescriptorheap.h"

GPUBuffer::GPUBuffer(uint32_t elemSize, uint32_t numElems /*= 1*/, BufferUsage usage /*= BufferUsage::All*/)
    : mNumElems(numElems), mElemSize(elemSize), mUsage(usage)
{
    if      (HasBufferUsage(BufferUsage::Constant))         { mCurrentUsage = BufferUsage::Constant; }
    else if (HasBufferUsage(BufferUsage::Structured))       { mCurrentUsage = BufferUsage::Structured; }
    else if (HasBufferUsage(BufferUsage::UnorderedAccess))  { mCurrentUsage = BufferUsage::UnorderedAccess; }
    else if (HasBufferUsage(BufferUsage::Vertex))           { mCurrentUsage = BufferUsage::Vertex; }
    else if (HasBufferUsage(BufferUsage::Index))            { mCurrentUsage = BufferUsage::Index; }
    else if (HasBufferUsage(BufferUsage::Indirect))         { mCurrentUsage = BufferUsage::Indirect; }
    else if (HasBufferUsage(BufferUsage::All))              { mCurrentUsage = BufferUsage::All; }

    ID3D12Device* const device = Graphic::Get().GetDevice();

    const uint32_t size = mNumElems * mElemSize;
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    if(HasBufferUsage(BufferUsage::UnorderedAccess))
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    const HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, GetCurrentResourceState(), nullptr, IID_PPV_ARGS(&mResource));
    assert(SUCCEEDED(hr));

    CreateViews();

}

D3D12_GPU_VIRTUAL_ADDRESS GPUBuffer::GetGPUAddress(uint32_t elemIdx)
{
    assert(elemIdx < mNumElems);
    return mResource->GetGPUVirtualAddress() + elemIdx * mElemSize;
}

uint8_t* GPUBuffer::Map(uint32_t start, uint32_t end)
{
    assert(!mTemporaryMapResource); // Tried to map twice

    mMappedRangeStart = start;
    mMappedRangeEnd = end;

    mTemporaryMapResource = GPUBufferUploadManager::Get().Reserve(mMappedRangeEnd - mMappedRangeStart);
    return mTemporaryMapResource->Map();
}

uint8_t* GPUBuffer::Map()
{
    return Map(0, GetBufferSize());
}

void GPUBuffer::Unmap(CommandList& cmdList)
{
    assert(mTemporaryMapResource); // Tried to unmap without mapping

    mTemporaryMapResource->Unmap();

    const CD3DX12_RESOURCE_BARRIER before = CD3DX12_RESOURCE_BARRIER::Transition(mResource, GetCurrentResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
    const CD3DX12_RESOURCE_BARRIER after = CD3DX12_RESOURCE_BARRIER::Transition(mResource, D3D12_RESOURCE_STATE_COPY_DEST, GetCurrentResourceState());

    cmdList->ResourceBarrier(1, &before);
    cmdList->CopyBufferRegion(mResource, mMappedRangeStart, mTemporaryMapResource->GetResource(), mTemporaryMapResource->GetStartRange(), mMappedRangeEnd - mMappedRangeStart);
    cmdList->ResourceBarrier(1, &after);

    mTemporaryMapResource = nullptr;
}

D3D12_RESOURCE_STATES GPUBuffer::GetResourceState(BufferUsage usage) const
{
    assert(IsPow2(static_cast<BufferUsageType>(usage))); // Only one bit can be set

    switch (usage)
    {
    case BufferUsage::Constant:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case BufferUsage::Structured:
        return D3D12_RESOURCE_STATE_COMMON;
    case BufferUsage::UnorderedAccess:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case BufferUsage::Vertex:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case BufferUsage::Index:
        return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case BufferUsage::Indirect:
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case BufferUsage::All:
        return D3D12_RESOURCE_STATE_GENERIC_READ;
    default:
        assert(0);
    }
    return D3D12_RESOURCE_STATE_GENERIC_READ;
}

void GPUBuffer::CreateViews()
{
    if (HasBufferUsage(BufferUsage::Constant))
    {
        mCBVHandle = std::make_unique<CPUDescriptorHandle>(Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate());

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
        desc.BufferLocation = mResource->GetGPUVirtualAddress();
        desc.SizeInBytes = mNumElems * mElemSize;
        Graphic::Get().GetDevice()->CreateConstantBufferView(&desc, *mCBVHandle);
    }
    if (HasBufferUsage(BufferUsage::Structured))
    {
        mSRVHandle = std::make_unique<CPUDescriptorHandle>(Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate());

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Buffer.NumElements = mNumElems;
        desc.Buffer.StructureByteStride = mElemSize;
        desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        Graphic::Get().GetDevice()->CreateShaderResourceView(mResource, &desc, *mSRVHandle);
    }
    if (HasBufferUsage(BufferUsage::UnorderedAccess))
    {
        mUAVHandle = std::make_unique<CPUDescriptorHandle>(Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate());

        D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.Buffer.CounterOffsetInBytes = 0;
        desc.Buffer.NumElements = mNumElems;
        desc.Buffer.StructureByteStride = mElemSize;
        desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        //desc.Buffer.CounterOffsetInBytes = mNumElems * mElemSize;

        Graphic::Get().GetDevice()->CreateUnorderedAccessView(mResource, nullptr, &desc, *mUAVHandle);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUBuffer::GetCBV()
{
    assert(HasBufferUsage(BufferUsage::Constant));
    return *mCBVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUBuffer::GetSRV()
{
    assert(HasBufferUsage(BufferUsage::Structured));
    return *mSRVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE GPUBuffer::GetUAV()
{
    assert(HasBufferUsage(BufferUsage::UnorderedAccess));
    return *mUAVHandle;
}

void GPUBuffer::SetCurrentUsage(BufferUsage usage, std::vector<D3D12_RESOURCE_BARRIER>& barriers)
{
    assert(IsPow2(static_cast<BufferUsageType>(usage))); // Only one bit can be set
    assert(HasBufferUsage(usage));

    const D3D12_RESOURCE_STATES stateBefore = GetResourceState(mCurrentUsage);
    const D3D12_RESOURCE_STATES stateAfter = GetResourceState(usage);

    if(stateBefore != stateAfter)
    {
        D3D12_RESOURCE_BARRIER& barrier = barriers.emplace_back();
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), stateBefore, stateAfter);
    }
    mCurrentUsage = usage;
}

GPUBuffer::~GPUBuffer()
{
    if (mResource)
    {
        mResource->Release();
        mResource = nullptr;
    }

    if (mCBVHandle) { Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(*mCBVHandle); }
    if (mSRVHandle) { Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(*mSRVHandle); }
    if (mUAVHandle) { Graphic::Get().GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(*mUAVHandle); }
    
}
