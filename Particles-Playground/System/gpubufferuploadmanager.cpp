#include "gpubufferuploadmanager.h"

uint8_t* UploadBufferTemporaryRange::Map()
{
    uint8_t* data = nullptr;
    HRESULT res = GetResource()->Map(0, &CD3DX12_RANGE(mStartRange, mEndRange), reinterpret_cast<void**>(&data));
    assert(SUCCEEDED(res));

    return data + mStartRange;
}

void UploadBufferTemporaryRange::Unmap()
{
    GetResource()->Unmap(0, &CD3DX12_RANGE(mStartRange, mEndRange));
}

ID3D12Resource* UploadBufferTemporaryRange::GetResource() const
{
    return GPUBufferUploadManager::Get().GetResource();
}

bool GPUBufferUploadManager::Startup()
{
    ID3D12Device* const device = Graphic::Get().GetDevice();

    D3D12_HEAP_DESC heapProps{};
    heapProps.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heapProps.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    heapProps.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    heapProps.SizeInBytes = UploadHeapSize;

    HRESULT res = device->CreateHeap(&heapProps, IID_PPV_ARGS(&mHeap));
    assert(SUCCEEDED(res));

    res = device->CreatePlacedResource(mHeap, 0, &CD3DX12_RESOURCE_DESC::Buffer(UploadHeapSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mUploadRes));
    assert(SUCCEEDED(res));

    return true;
}

bool GPUBufferUploadManager::Shutdown()
{
    if (mHeap) { mHeap->Release(); }
    if (mUploadRes) { mUploadRes->Release(); }
    return true;
}

void GPUBufferUploadManager::PreUpdate()
{
    const uint64_t currentFrameNum = Graphic::Get().GetCurrentFrameNumber();
    const uint32_t frameCount = Graphic::Get().GetFrameCount();

    for (Allocation& alloc : mAllocations)
    {
        // Merge not needed memory block
        if (alloc.FrameNumber + frameCount <= currentFrameNum)
        {
            mAllocator.Free({ alloc.Start, alloc.Size });
        }
    }

    mAllocations.remove_if([currentFrameNum, frameCount](const Allocation& alloc) {
        return alloc.FrameNumber + frameCount <= currentFrameNum;
    });

}

UploadBufferTemporaryRangeHandle GPUBufferUploadManager::Reserve(uint32_t size)
{
    ID3D12Device* const device = Graphic::Get().GetDevice();

    Range alloc = mAllocator.Allocate(size);
    assert(alloc.IsValid());

    const uint64_t currentFrameNum = Graphic::Get().GetCurrentFrameNumber();
    mAllocations.push_back({ alloc.Start, alloc.Size, currentFrameNum });

    return std::make_unique<UploadBufferTemporaryRange>(alloc.Start, alloc.Start + alloc.Size);
}
