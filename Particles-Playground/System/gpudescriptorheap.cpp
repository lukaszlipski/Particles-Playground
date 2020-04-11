#include "gpudescriptorheap.h"

GPUDescriptorHeap::GPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) 
    : mType(type), mAllocator(0, DescriptorNum)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = DescriptorNum;
    desc.Type = mType;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Graphic::Get().GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap));
}

GPUDescriptorHeap::~GPUDescriptorHeap()
{
    if (mHeap) { mHeap->Release(); }

    for (uint32_t idx = 0; idx < Graphic::GetFrameCount(); ++idx)
    {
        for (GPUDescriptorHandle& handle : mDelayedRelease[idx])
        {
            mAllocator.Free(handle.GetAllocation());
        }
    }
}

GPUDescriptorHandle GPUDescriptorHeap::Allocate(uint32_t size /*= 1*/)
{
    Range alloc = mAllocator.Allocate(size);
    assert(alloc.IsValid()); // not enough descriptors

    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(mHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<uint32_t>(alloc.Start), Graphic::Get().GetHandleSize(mType));
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<uint32_t>(alloc.Start), Graphic::Get().GetHandleSize(mType));

    return GPUDescriptorHandle(alloc, cpuHandle, gpuHandle, mType);
}

void GPUDescriptorHeap::Free(GPUDescriptorHandle& handle)
{
    if (!handle.IsValid()) { return; }

    const uint32_t idx = Graphic::Get().GetCurrentFrameIndex();
    mDelayedRelease[idx].emplace_back(std::move(handle));
}

void GPUDescriptorHeap::ReleaseUnusedDescriptorHandles()
{
    const uint32_t idx = Graphic::Get().GetCurrentFrameIndex();

    for (GPUDescriptorHandle& handle : mDelayedRelease[idx])
    {
        mAllocator.Free(handle.GetAllocation());
    }

    mDelayedRelease[idx].clear();
}

GPUDescriptorHeap& GPUDescriptorHeap::operator=(GPUDescriptorHeap&& rhs)
{
    this->mAllocator = std::move(rhs.mAllocator);
    this->mHeap = rhs.mHeap;
    this->mType = rhs.mType;

    rhs.mHeap = nullptr;

    return *this;
}

GPUDescriptorHeap::GPUDescriptorHeap(GPUDescriptorHeap&& rhs)
    : mAllocator(0, DescriptorNum)
{
    *this = std::move(rhs);
}

GPUDescriptorHandle& GPUDescriptorHandle::operator=(GPUDescriptorHandle&& rhs)
{
    mAllocation = rhs.mAllocation;
    mGpuHandle = rhs.mGpuHandle;
    mCpuHandle = rhs.mCpuHandle;
    mType = rhs.mType;

    rhs.mAllocation.Invalidate();

    return *this;
}

GPUDescriptorHandle::GPUDescriptorHandle(GPUDescriptorHandle&& rhs)
{
    *this = std::move(rhs);
}

GPUDescriptorHandle::~GPUDescriptorHandle()
{
    assert(!mAllocation.IsValid());
}

GPUDescriptorHandleScoped::~GPUDescriptorHandleScoped()
{
    GPUDescriptorHeap* heap = Graphic::Get().GetGPUDescriptorHeap(mType);
    heap->Free(*this);
}
