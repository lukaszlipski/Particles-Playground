#pragma once
#include "graphic.h"
#include "../Utilities/freelistallocator.h"

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

    struct Allocation
    {
        Range AllocationRange;
        uint64_t FrameNumber = 0;
    };

public:
    GPUBufferUploadManager(const GPUBufferUploadManager&) = delete;
    GPUBufferUploadManager(GPUBufferUploadManager&&) = delete;

    GPUBufferUploadManager& operator=(const GPUBufferUploadManager&) = delete;
    GPUBufferUploadManager& operator=(GPUBufferUploadManager&&) = delete;

    bool Startup();
    bool Shutdown();

    void PreUpdate();

    UploadBufferTemporaryRangeHandle Reserve(uint32_t size);

    inline ID3D12Resource* GetResource() const { return mUploadRes; }

    static GPUBufferUploadManager& Get()
    {
        static GPUBufferUploadManager* instance = new GPUBufferUploadManager();
        return *instance;
    }

private:
    explicit GPUBufferUploadManager()
        : mAllocator(0, UploadHeapSize)
    {}

    ID3D12Heap* mHeap = nullptr;
    ID3D12Resource* mUploadRes = nullptr;

    std::list<Allocation> mAllocations;
    FreeListAllocator<FirstFitStrategy> mAllocator;

};
