#include "gpubuffer.h"
#include "commandlist.h"


GPUBuffer& GPUBuffer::operator=(GPUBuffer&& rhs)
{
    mResource = rhs.mResource;
    rhs.mResource = nullptr;

    return *this;
}

GPUBuffer::GPUBuffer(GPUBuffer&& rhs)
{
    *this = std::move(rhs);
}

GPUBuffer::GPUBuffer(uint32_t elemSize, uint32_t numElems /*= 1*/, D3D12_RESOURCE_STATES resourceState /*= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER*/) 
    : mNumElems(numElems), mElemSize(elemSize), mCurrentResourceState(resourceState)
{
    ID3D12Device* const device = Graphic::Get().GetDevice();
    device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(numElems * elemSize), mCurrentResourceState, nullptr, IID_PPV_ARGS(&mResource));
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

void GPUBuffer::Unmap(CommandList& cmdList)
{
    assert(mTemporaryMapResource); // Tried to unmap without mapping

    mTemporaryMapResource->Unmap();

    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mResource, mCurrentResourceState, D3D12_RESOURCE_STATE_COPY_DEST));
    cmdList->CopyBufferRegion(mResource, mMappedRangeStart, mTemporaryMapResource->GetResource(), mTemporaryMapResource->GetStartRange(), mMappedRangeEnd - mMappedRangeStart);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mResource, D3D12_RESOURCE_STATE_COPY_DEST, mCurrentResourceState));

    mTemporaryMapResource = nullptr;
}

GPUBuffer::~GPUBuffer()
{
    if (!mResource) { return; }

    mResource->Release();
    mResource = nullptr;
}
