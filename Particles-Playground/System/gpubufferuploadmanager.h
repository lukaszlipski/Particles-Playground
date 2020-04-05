#pragma once
#include "graphic.h"

class UploadBufferTemporaryRange
{
public:
    UploadBufferTemporaryRange(uint64_t startRange, uint64_t endRange)
        : mStartRange(startRange), mEndRange(endRange)
    { }

    uint8_t* Map();
    void Unmap();

    ID3D12Resource* GetResource() const;

    inline uint64_t GetStartRange() const { return mStartRange; }
    inline uint64_t GetEndRange() const { return mEndRange; }

private:
    uint64_t mStartRange = 0;
    uint64_t mEndRange = 0;

};

using UploadBufferTemporaryRangeHandle = std::unique_ptr<UploadBufferTemporaryRange>;

// Handles memory that is used for buffer's uploads. Each allocation is valid only for the maximum amount of frames in flight defined in Graphic
class GPUBufferUploadManager
{
    static const uint32_t UploadHeapSize = 1 * 1024 * 1024;

    struct FreeBlock
    {
        uint64_t Start = 0;
        uint64_t Size = 0;
    };

    struct Allocation
    {
        uint64_t Start = 0;
        uint64_t Size = 0;
        uint32_t FrameNumber = 0;
    };

public:
    GPUBufferUploadManager(const GPUBufferUploadManager&) = delete;
    GPUBufferUploadManager(GPUBufferUploadManager&&) = delete;

    GPUBufferUploadManager& operator=(const GPUBufferUploadManager&) = delete;
    GPUBufferUploadManager& operator=(GPUBufferUploadManager&&) = delete;

    bool Startup();
    bool Shutdown();

    void PreUpdate();

    UploadBufferTemporaryRangeHandle Reserve(uint32_t size)
    {
        ID3D12Device* const device = Graphic::Get().GetDevice();

        Allocation alloc = FindMemoryBlock(size);

        return std::make_unique<UploadBufferTemporaryRange>(alloc.Start, alloc.Start + alloc.Size);
    }

    inline ID3D12Resource* GetResource() const { return mUploadRes; }

    static GPUBufferUploadManager& Get()
    {
        static GPUBufferUploadManager* instance = new GPUBufferUploadManager();
        return *instance;
    }

private:
    explicit GPUBufferUploadManager() = default;

    Allocation FindMemoryBlock(uint32_t size);

    ID3D12Heap* mHeap = nullptr;
    ID3D12Resource* mUploadRes = nullptr;

    std::list<FreeBlock> mFreeList;
    std::list<Allocation> mAllocations;

};
