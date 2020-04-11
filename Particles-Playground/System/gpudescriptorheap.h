#pragma once
#include "System/graphic.h"
#include "Utilities/circularallocator.h"

class GPUDescriptorHandle
{
public:
    GPUDescriptorHandle(const Range& allocation, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE type)
        : mAllocation(allocation), mCpuHandle(cpuHandle), mGpuHandle(gpuHandle), mType(type)
    { }

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

class GPUDescriptorHeap
{
    static const uint32_t DescriptorNum = 100 * Graphic::GetFrameCount();

public:
    GPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
    ~GPUDescriptorHeap();

    GPUDescriptorHeap(const GPUDescriptorHeap& rhs) = delete;
    GPUDescriptorHeap& operator=(const GPUDescriptorHeap& rhs) = delete;

    GPUDescriptorHeap(GPUDescriptorHeap&& rhs);
    GPUDescriptorHeap& operator=(GPUDescriptorHeap&& rhs);

    inline ID3D12DescriptorHeap* GetHeap() const { return mHeap; }

    GPUDescriptorHandle Allocate(uint32_t size = 1);
    void Free(GPUDescriptorHandle& handle);

    void ReleaseUnusedDescriptorHandles();

private:
    using DelayedReleaseContainer = std::array<std::vector<GPUDescriptorHandle>, Graphic::GetFrameCount()>;

    CircularAllocator mAllocator;
    D3D12_DESCRIPTOR_HEAP_TYPE mType;
    ID3D12DescriptorHeap* mHeap = nullptr;
    DelayedReleaseContainer mDelayedRelease;

};
