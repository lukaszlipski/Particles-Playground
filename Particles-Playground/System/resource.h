#pragma once

struct HeapAllocationInfo
{
    ID3D12Heap* mHeap = nullptr;
    uint32_t mOffset = std::numeric_limits<uint32_t>::max();

    bool IsValid()
    {
        return mHeap != nullptr && mOffset != std::numeric_limits<uint32_t>::max();
    }
};

enum class ResourceType
{
    GPUBuffer,
    Texture2D
};

class ResourceBase
{
public:
    explicit ResourceBase(ResourceType type)
        : mType(type)
    { }

    virtual ~ResourceBase()
    {
        if (mResource)
        {
            mResource->Release();
            mResource = nullptr;
        }
    }

    ResourceBase(const ResourceBase&) = delete;
    ResourceBase& operator=(const ResourceBase&) = delete;

    ResourceBase(ResourceBase&& rhs) = delete;
    ResourceBase& operator=(ResourceBase&& rhs) = delete;

    inline ID3D12Resource* GetResource() const { return mResource; }
    inline ResourceType GetType() const { return mType; }

    inline void SetDebugName(std::wstring_view name)
    {
        Assert(mResource);
        mResource->SetName(name.data());
    }

protected:
    ID3D12Resource* mResource = nullptr;
    ResourceType mType;

};

template<typename T, typename = std::enable_if_t<std::is_base_of_v<ResourceBase, T>>>
struct ResourceTraits
{

};

