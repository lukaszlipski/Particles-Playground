#pragma once

class ResourceBase
{
public:
    ResourceBase() = default;

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

    inline void SetDebugName(std::wstring_view name)
    {
        assert(mResource);
        mResource->SetName(name.data());
    }

protected:
    ID3D12Resource* mResource = nullptr;

};
