#pragma once
#include "graphic.h"
#include "resource.h"
#include "gpubufferuploadmanager.h"

class CommandList;
class CPUDescriptorHandle;

using BufferUsageType = uint8_t;

enum class BufferUsage : BufferUsageType
{
    Constant        = 1 << 0,
    Structured      = 1 << 1,
    UnorderedAccess = 1 << 2,
    Vertex          = 1 << 3,
    Index           = 1 << 4,
    Indirect        = 1 << 5,
    CopyDst         = 1 << 6,
    CopySrc         = 1 << 7,
    All             = std::numeric_limits<BufferUsageType>::max()
};

constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
    return static_cast<BufferUsage>(static_cast<BufferUsageType>(lhs) | static_cast<BufferUsageType>(rhs));
}

constexpr BufferUsage operator&(BufferUsage lhs, BufferUsage rhs) {
    return static_cast<BufferUsage>(static_cast<BufferUsageType>(lhs) & static_cast<BufferUsageType>(rhs));
}

class GPUBuffer : public ResourceBase
{
public:
    GPUBuffer(uint32_t elemSize, uint32_t numElems = 1, BufferUsage usage = BufferUsage::All, HeapAllocationInfo* heapAllocInfo = nullptr);

    ~GPUBuffer();

    inline uint32_t GetBufferSize() const { return mElemSize * mNumElems; }
    inline bool HasBufferUsage(BufferUsage usage) const { return static_cast<BufferUsageType>(mUsage & usage) == static_cast<BufferUsageType>(usage); }
    inline D3D12_RESOURCE_STATES GetCurrentResourceState() const { return GetResourceState(mCurrentUsage); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(uint32_t elemIdx = 0);
    uint8_t* Map(uint32_t start, uint32_t end);
    uint8_t* Map();
    void Unmap(CommandList& cmdList);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCBV();
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV();
    D3D12_CPU_DESCRIPTOR_HANDLE GetUAV();
    void SetCurrentUsage(BufferUsage usage, std::vector<D3D12_RESOURCE_BARRIER>& barriers);

private:
    D3D12_RESOURCE_STATES GetResourceState(BufferUsage usage) const;
    void CreateViews();

    uint32_t mMappedRangeStart = 0;
    uint32_t mMappedRangeEnd = 0;
    UploadBufferTemporaryRangeHandle mTemporaryMapResource = nullptr;

    uint32_t mNumElems = 0;
    uint32_t mElemSize = 0;
    BufferUsage mUsage;
    BufferUsage mCurrentUsage;
    std::unique_ptr<CPUDescriptorHandle> mCBVHandle;
    std::unique_ptr<CPUDescriptorHandle> mSRVHandle;
    std::unique_ptr<CPUDescriptorHandle> mUAVHandle;

};

template<>
struct ResourceTraits<GPUBuffer>
{
    static const ResourceType Type = ResourceType::GPUBuffer;
};

