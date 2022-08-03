#pragma once
#include "System/graphic.h"
#include "Utilities/circularallocator.h"
#include "Utilities/freelistallocator.h"

class GPUBindlessDescriptorHandle
{
public:
    GPUBindlessDescriptorHandle(const Range& allocation, D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap);

    GPUBindlessDescriptorHandle(const GPUBindlessDescriptorHandle&) = delete;
    GPUBindlessDescriptorHandle& operator=(const GPUBindlessDescriptorHandle&) = delete;

    GPUBindlessDescriptorHandle(GPUBindlessDescriptorHandle&& rhs);
    GPUBindlessDescriptorHandle& operator=(GPUBindlessDescriptorHandle&& rhs);

    inline Range& GetAllocation() { return mAllocation; }
    inline bool IsValid() const { return mAllocation.IsValid(); }

    inline uint32_t GetIndex() const { return static_cast<uint32_t>(mAllocation.Start); }
    operator int32_t() const { return GetIndex(); }
    operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return mCpuHandle; }

private:
    Range mAllocation;
    D3D12_CPU_DESCRIPTOR_HANDLE mCpuHandle;
    D3D12_DESCRIPTOR_HEAP_TYPE mType;
};

class GPUDescriptorHandle
{
public:
    GPUDescriptorHandle(const Range& allocation, D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap);

    virtual ~GPUDescriptorHandle();

    GPUDescriptorHandle(const GPUDescriptorHandle&) = delete;
    GPUDescriptorHandle& operator=(const GPUDescriptorHandle&) = delete;

    GPUDescriptorHandle(GPUDescriptorHandle&& rhs);
    GPUDescriptorHandle& operator=(GPUDescriptorHandle&& rhs);

    inline Range& GetAllocation() { return mAllocation; }
    inline bool IsValid() const { return mAllocation.IsValid(); }

    operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return mCpuHandle; }
    operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return mGpuHandle; }

protected:
    Range mAllocation;
    D3D12_CPU_DESCRIPTOR_HANDLE mCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mGpuHandle;
    D3D12_DESCRIPTOR_HEAP_TYPE mType;

};

class GPUDescriptorHandleScoped : public GPUDescriptorHandle
{
public:

    GPUDescriptorHandleScoped(GPUDescriptorHandle&& handle)
        : GPUDescriptorHandle(std::move(handle))
    { }

    ~GPUDescriptorHandleScoped();

};

struct GPUDescriptorType
{};

struct GPUBindlessDescriptor : GPUDescriptorType
{
    using DescHandle = GPUBindlessDescriptorHandle;
};

struct GPUStandardDescriptor : GPUDescriptorType
{
    using DescHandle = GPUDescriptorHandle;
};

class GPUDescriptorHeap
{
public:
    static const uint32_t StandardDescriptorNum = 100 * Graphic::GetFrameCount();
    static const uint32_t BindlessDescriptorNum = 100;

private:
    static const uint32_t StandardDescriptorOffset = 0;
    static const uint32_t BindlessDescriptorOffset = StandardDescriptorNum;
    static const uint32_t DescriptorNum = StandardDescriptorNum + BindlessDescriptorNum;

    using DelayedReleaseContainer = std::array<std::vector<GPUDescriptorHandle>, Graphic::GetFrameCount()>;

public:
    GPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
    ~GPUDescriptorHeap();

    GPUDescriptorHeap(const GPUDescriptorHeap& rhs) = delete;
    GPUDescriptorHeap& operator=(const GPUDescriptorHeap& rhs) = delete;

    GPUDescriptorHeap(GPUDescriptorHeap&& rhs);
    GPUDescriptorHeap& operator=(GPUDescriptorHeap&& rhs);

    inline ID3D12DescriptorHeap* GetHeap() const { return mHeap; }

    template<typename DescType, typename = std::enable_if_t<std::is_base_of_v<GPUDescriptorType, DescType> && !std::is_same_v<GPUDescriptorType, DescType>>>
    typename DescType::DescHandle Allocate(uint32_t size = 1);
    void Free(GPUDescriptorHandle& handle);
    void Free(GPUBindlessDescriptorHandle& handle);

    void ReleaseUnusedDescriptorHandles();

private:

    template<typename DescType>
    Range InternalAllocate(uint32_t size)
    {
        return Range::Invalid;
    }
    template<> Range InternalAllocate<GPUBindlessDescriptor>(uint32_t size);
    template<> Range InternalAllocate<GPUStandardDescriptor>(uint32_t size);

    CircularAllocator mStandardAllocator;
    FreeListAllocator<FirstFitStrategy> mBindlessAllocator;
    D3D12_DESCRIPTOR_HEAP_TYPE mType;
    ID3D12DescriptorHeap* mHeap = nullptr;
    DelayedReleaseContainer mDelayedRelease;
};
