#pragma once
#include "graphic.h"
#include "gpubufferuploadmanager.h"

class CommandList;

class GPUBuffer
{
public:
    GPUBuffer(uint32_t elemSize, uint32_t numElems = 1, D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    ~GPUBuffer();

    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;

    GPUBuffer(GPUBuffer&& rhs);
    GPUBuffer& operator=(GPUBuffer&& rhs);

    inline ID3D12Resource* GetResource() const { return mResource; }
    inline uint32_t GetBufferSize() const { return mElemSize * mNumElems; }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(uint32_t elemIdx = 0);
    uint8_t* Map(uint32_t start, uint32_t end);
    void Unmap(CommandList& cmdList);

private:

    uint32_t mMappedRangeStart = 0;
    uint32_t mMappedRangeEnd = 0;
    UploadBufferTemporaryRangeHandle mTemporaryMapResource = nullptr;

    ID3D12Resource* mResource = nullptr;
    uint32_t mNumElems = 0;
    uint32_t mElemSize = 0;
    D3D12_RESOURCE_STATES mCurrentResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

};
