#pragma once
#include "System/gpubuffer.h"
#include "System/texture.h"
#include "Utilities/freelistallocator.h"
#include "Utilities/objectpool.h"

struct TransientResource : IObject<TransientResource>
{
    std::unique_ptr<ResourceBase> mResource;
    Range mAllocation;
    uint64_t mFrameNumber = std::numeric_limits<uint64_t>::max();
};
using TransientResourceHandle = ObjectHandle<TransientResource>;

class CommandList;

class TransientResourceAllocator
{
    struct ResourceToFree
    {
        std::unique_ptr<ResourceBase> mResource;
        uint64_t mOffset = 0;
        uint64_t mSize = 0;
        uint64_t mFrameNumber = std::numeric_limits<uint64_t>::max();
        bool mAliased = false;
    };

public:
    static const uint32_t TransientResourceMemorySize = 256 * 1024 * 1024;
    static const uint32_t MaxTransientGPUBuffers = 1024;

    TransientResourceAllocator()
        : mAllocator(0, TransientResourceMemorySize)
        , mTransientResources(MaxTransientGPUBuffers)
    { }

    void Init();
    void Free();

    void PreUpdate();

    void Step(std::vector<D3D12_RESOURCE_BARRIER>& barriers);

    [[nodiscard]] TransientResourceHandle AllocateGPUBuffer(uint32_t elemSize, uint32_t numElems, BufferUsage usage);
    [[nodiscard]] TransientResourceHandle AllocateTexture2D(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage);
    void FreeResource(TransientResourceHandle& handle);

    template<typename ResType>
    ResType* GetResource(TransientResourceHandle handle);

private:
    template<typename ResType, typename... Args>
    TransientResourceHandle AllocateResource(uint32_t resourceSize, Args... args);

    ID3D12Heap* mHeap = nullptr;
    FreeListAllocator<FirstFitStrategy> mAllocator;
    ObjectPool<TransientResource> mTransientResources;

    std::list<ResourceToFree> mResourcesToFree;
    std::vector<D3D12_RESOURCE_BARRIER> mAliasingBarriers;
};
