#pragma once
#include "graphic.h"
#include "gpubufferuploadmanager.h"

class CommandList;
class CPUDescriptorHandle;

enum class BufferUsage : uint8_t
{
    Constant        = 1 << 0,
    Structured      = 1 << 1,
    UnorderedAccess = 1 << 2,
    Vertex          = 1 << 3,
    Index           = 1 << 4,
    Indirect        = 1 << 5,
    All             = 255
};

constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
    return static_cast<BufferUsage>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

constexpr BufferUsage operator&(BufferUsage lhs, BufferUsage rhs) {
    return static_cast<BufferUsage>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

class GPUBuffer
{
public:
    GPUBuffer(uint32_t elemSize, uint32_t numElems = 1, BufferUsage usage = BufferUsage::All);

    ~GPUBuffer();

    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;

    GPUBuffer(GPUBuffer&& rhs);
    GPUBuffer& operator=(GPUBuffer&& rhs);

    inline ID3D12Resource* GetResource() const { return mResource; }
    inline uint32_t GetBufferSize() const { return mElemSize * mNumElems; }
    inline bool HasBufferUsage(BufferUsage usage) const { return static_cast<uint8_t>(mUsage & usage) == static_cast<uint8_t>(usage); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(uint32_t elemIdx = 0);
    uint8_t* Map(uint32_t start, uint32_t end);
    uint8_t* Map();
    void Unmap(CommandList& cmdList);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCBV();
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV();
    D3D12_CPU_DESCRIPTOR_HANDLE GetUAV();

private:
    D3D12_RESOURCE_STATES GetResourceState(BufferUsage usage) const;
    void CreateViews();

    uint32_t mMappedRangeStart = 0;
    uint32_t mMappedRangeEnd = 0;
    UploadBufferTemporaryRangeHandle mTemporaryMapResource = nullptr;

    ID3D12Resource* mResource = nullptr;
    uint32_t mNumElems = 0;
    uint32_t mElemSize = 0;
    D3D12_RESOURCE_STATES mCurrentResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    BufferUsage mUsage;
    std::unique_ptr<CPUDescriptorHandle> mCBVHandle;
    std::unique_ptr<CPUDescriptorHandle> mSRVHandle;
    std::unique_ptr<CPUDescriptorHandle> mUAVHandle;

};
