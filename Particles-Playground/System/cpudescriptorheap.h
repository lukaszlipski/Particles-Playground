#pragma once
#include "Utilities/freelistallocator.h"

class CPUDescriptorHandle
{
public:
    CPUDescriptorHandle(const Range& allocation, D3D12_CPU_DESCRIPTOR_HANDLE handle)
        : mAllocation(allocation), mHandle(handle)
    { }

    inline Range& GetAllocation() { return mAllocation; }
    inline D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const { return mHandle; }
    inline bool IsValid() const { return mAllocation.IsValid(); }

    operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return mHandle; }

private:
    Range mAllocation;
    D3D12_CPU_DESCRIPTOR_HANDLE mHandle;

};

class CPUDescriptorHeap
{
    static const uint32_t DescriptorNum = 10000;

public:
    CPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

    ~CPUDescriptorHeap();

    CPUDescriptorHeap(const CPUDescriptorHeap& rhs) = delete;
    CPUDescriptorHeap& operator=(const CPUDescriptorHeap& rhs) = delete;

    CPUDescriptorHeap(CPUDescriptorHeap&& rhs);
    CPUDescriptorHeap& operator=(CPUDescriptorHeap&& rhs);

    CPUDescriptorHandle Allocate();
    void Free(CPUDescriptorHandle& handle);

private:
    D3D12_DESCRIPTOR_HEAP_TYPE mType;
    ID3D12DescriptorHeap* mHeap = nullptr;
    FreeListAllocator<FirstFitStrategy> mAllocator;

};
