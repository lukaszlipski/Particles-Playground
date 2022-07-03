#include "System/transientresourceallocator.h"
#include "System/graphic.h"
#include "System/commandlist.h"
#include "System/resource.h"

void TransientResourceAllocator::Init()
{
    mTransientResources.Init();

    CD3DX12_HEAP_DESC heapDesc(TransientResourceMemorySize, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
    ID3D12Device* const device = Graphic::Get().GetDevice();
    HRESULT res = device->CreateHeap(&heapDesc, IID_PPV_ARGS(&mHeap));
    Assert(SUCCEEDED(res));
}

void TransientResourceAllocator::Free()
{
    mResourcesToFree.clear();
    mTransientResources.Free();
    mHeap->Release();
}

void TransientResourceAllocator::PreUpdate()
{
    Assert(mAllocator.GetAllocationNum() == 0);
    Assert(mTransientResources.GetObjects().size() == 0);

    const uint64_t currentFrameNum = Graphic::Get().GetCurrentFrameNumber();
    const uint32_t frameCount = Graphic::Get().GetFrameCount();

    mResourcesToFree.remove_if([allocator = &mAllocator, currentFrameNum, frameCount](ResourceToFree& resToFree) {
        return resToFree.mFrameNumber + frameCount <= currentFrameNum;
    });
}

void TransientResourceAllocator::Step(std::vector<D3D12_RESOURCE_BARRIER>& barriers)
{
    barriers = std::move(mAliasingBarriers);
    mAliasingBarriers.clear();
}

TransientResourceHandle TransientResourceAllocator::AllocateGPUBuffer(uint32_t elemSize, uint32_t numElems, BufferUsage usage)
{
    const uint32_t size = elemSize * numElems;
    return AllocateResource<GPUBuffer>(size, elemSize, numElems, usage);
}

TransientResourceHandle TransientResourceAllocator::AllocateTexture2D(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage)
{
    const uint32_t size = width * height * Texture2D::GetSizeForFormat(format);
    return AllocateResource<Texture2D>(size, width, height, format, usage);
}

void TransientResourceAllocator::FreeResource(TransientResourceHandle& handle)
{
    TransientResource* transientResource = mTransientResources.GetObject(handle);
    if (!transientResource) { return; }

    // Transient resources should be released within the same frame they were allocated
    Assert(Graphic::Get().GetCurrentFrameNumber() == transientResource->mFrameNumber);

    ResourceToFree& toFree = mResourcesToFree.emplace_back();
    toFree.mResource = std::move(transientResource->mResource);
    toFree.mFrameNumber = transientResource->mFrameNumber;
    toFree.mOffset = transientResource->mAllocation.Start;
    toFree.mSize = transientResource->mAllocation.Size;

    mAllocator.Free(transientResource->mAllocation);
    mTransientResources.FreeObject(handle);
}

template<typename ResType>
ResType* TransientResourceAllocator::GetResource(TransientResourceHandle handle)
{
    TransientResource* transientGpuBuffer = mTransientResources.GetObject(handle);
    if (transientGpuBuffer)
    {
        ResourceBase* resource = transientGpuBuffer->mResource.get();
        if (resource->GetType() == ResourceTraits<ResType>::Type)
        {
            return static_cast<ResType*>(resource);
        }
    }
    return nullptr;
}
template GPUBuffer* TransientResourceAllocator::GetResource<GPUBuffer>(TransientResourceHandle handle);
template Texture2D* TransientResourceAllocator::GetResource<Texture2D>(TransientResourceHandle handle);

template<typename ResType, typename... Args>
TransientResourceHandle TransientResourceAllocator::AllocateResource(uint32_t resourceSize, Args... args)
{
    const TransientResourceHandle handle = mTransientResources.AllocateObject();
    Assert(mTransientResources.ValidateHandle(handle));

    TransientResource* transientResource = mTransientResources.GetObject(handle);

    transientResource->mAllocation = mAllocator.Allocate(resourceSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    Assert(transientResource->mAllocation.IsValid());

    HeapAllocationInfo info{};
    info.mHeap = mHeap;
    info.mOffset = static_cast<uint32_t>(transientResource->mAllocation.Start);

    transientResource->mResource = std::make_unique<ResType>(args..., &info);
    transientResource->mFrameNumber = Graphic::Get().GetCurrentFrameNumber();
    Assert(transientResource->mResource->GetResource());

    // Check if aliasing barriers are needed
    const uint64_t startRange = transientResource->mAllocation.Start;
    const uint64_t endRange = startRange + transientResource->mAllocation.Size;

    for (auto itr = mResourcesToFree.rbegin(); itr != mResourcesToFree.rend(); ++itr)
    {
        ResourceToFree& resToFree = *itr;
        const bool sameFrame = resToFree.mFrameNumber == Graphic::Get().GetCurrentFrameNumber();
        if (!sameFrame)
            break; // Lower indices contain resources from previous frames so we can stop searching

        const uint64_t currentStartRange = resToFree.mOffset;
        const uint64_t currentEndRange = currentStartRange + resToFree.mSize;

        if (!resToFree.mAliased && startRange <= currentEndRange && endRange >= currentStartRange)
        {
            mAliasingBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Aliasing(resToFree.mResource->GetResource(), transientResource->mResource->GetResource()));
            resToFree.mAliased = true;
        }
    }

    return handle;
}
