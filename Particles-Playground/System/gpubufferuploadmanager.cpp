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

    mFreeList.push_back({ 0, UploadHeapSize });

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
        if (alloc.FrameNumber + frameCount >= currentFrameNum)
        {
            const uint64_t start = alloc.Start;
            const uint64_t size = alloc.Size;

            auto left = std::find_if(mFreeList.begin(), mFreeList.end(), [start](const FreeBlock& block) {
                return block.Start + block.Size == start;
            });

            auto right = std::find_if(mFreeList.rbegin(), mFreeList.rend(), [start, size](const FreeBlock& block) {
                return block.Start == start + size;
            });

            if (left != mFreeList.end() && right == mFreeList.rend()) // we can merge only with left block
            {
                left->Size += size;
            }
            else if (left == mFreeList.end() && right != mFreeList.rend()) // we can merge only with right block
            {
                right->Start = start;
            }
            else if (left != mFreeList.end() && right != mFreeList.rend()) // we can merge from both sides
            {
                left->Size += size + right->Size;
                mFreeList.erase((++right).base());
            }
            else if (left == mFreeList.end() && right == mFreeList.rend()) // we can't merge with any block
            {
                auto firstGreater = std::find_if(mFreeList.begin(), mFreeList.end(), [start](const FreeBlock& block) {
                    return start > block.Start;
                });

                if (firstGreater != mFreeList.end())
                {
                    mFreeList.insert(firstGreater, { start,size });
                }
                else
                {
                    mFreeList.push_back({ start, size });
                }
            }
        }
    }

    mAllocations.remove_if([currentFrameNum, frameCount](const Allocation& alloc) {
        return alloc.FrameNumber + frameCount >= currentFrameNum;
    });

}

GPUBufferUploadManager::Allocation GPUBufferUploadManager::FindMemoryBlock(uint32_t size)
{
    auto firstBestFit = std::find_if(mFreeList.begin(), mFreeList.end(), [size](const FreeBlock& block) {
        return block.Size >= size;
    });
    assert(firstBestFit != mFreeList.end()); // Cannot find a block which contains enough memory

    const uint32_t currentFrameNum = Graphic::Get().GetCurrentFrameNumber();
    mAllocations.push_back({ firstBestFit->Start, size, currentFrameNum });

    firstBestFit->Size -= size;
    firstBestFit->Start += size;

    if (firstBestFit->Size == 0)
    {
        mFreeList.erase(firstBestFit);
    }

    return mAllocations.back();
}
