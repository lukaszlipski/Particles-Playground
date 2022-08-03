#include "gpudescriptorheap.h"

GPUDescriptorHeap::GPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) 
    : mType(type)
    , mStandardAllocator(StandardDescriptorOffset, StandardDescriptorOffset + StandardDescriptorNum)
    , mBindlessAllocator(BindlessDescriptorOffset, BindlessDescriptorOffset + BindlessDescriptorNum)
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
            mBindlessAllocator.Free(handle.GetAllocation());
            mStandardAllocator.Free(handle.GetAllocation());
        }
    }
}

template<typename DescType, typename>
typename DescType::DescHandle GPUDescriptorHeap::Allocate(uint32_t size /*= 1*/)
{
    Range alloc = InternalAllocate<DescType>(size);
    Assert(alloc.IsValid()); // not enough descriptors

    return typename DescType::DescHandle(alloc, mType, mHeap);
}

template GPUBindlessDescriptorHandle GPUDescriptorHeap::Allocate<GPUBindlessDescriptor>(uint32_t);
template GPUDescriptorHandle GPUDescriptorHeap::Allocate<GPUStandardDescriptor>(uint32_t);

void GPUDescriptorHeap::Free(GPUDescriptorHandle& handle)
{
    if (!handle.IsValid()) { return; }

    const uint32_t idx = Graphic::Get().GetCurrentFrameIndex();
    mDelayedRelease[idx].emplace_back(std::move(handle));
}

void GPUDescriptorHeap::Free(GPUBindlessDescriptorHandle& handle)
{
    if (!handle.IsValid()) { return; }
    mBindlessAllocator.Free(handle.GetAllocation());
}

void GPUDescriptorHeap::ReleaseUnusedDescriptorHandles()
{
    const uint32_t idx = Graphic::Get().GetCurrentFrameIndex();

    for (GPUDescriptorHandle& handle : mDelayedRelease[idx])
    {
        mBindlessAllocator.Free(handle.GetAllocation());
        mStandardAllocator.Free(handle.GetAllocation());
    }

    mDelayedRelease[idx].clear();
}

GPUDescriptorHeap& GPUDescriptorHeap::operator=(GPUDescriptorHeap&& rhs)
{
    this->mStandardAllocator = std::move(rhs.mStandardAllocator);
    this->mBindlessAllocator = std::move(rhs.mBindlessAllocator);
    this->mHeap = rhs.mHeap;
    this->mType = rhs.mType;

    rhs.mHeap = nullptr;

    return *this;
}

GPUDescriptorHeap::GPUDescriptorHeap(GPUDescriptorHeap&& rhs)
    : mStandardAllocator(0, 0)
    , mBindlessAllocator(0, 0)
{
    *this = std::move(rhs);
}

template<>
Range GPUDescriptorHeap::InternalAllocate<GPUBindlessDescriptor>(uint32_t size)
{
    return mBindlessAllocator.Allocate(size);
}

template<>
Range GPUDescriptorHeap::InternalAllocate<GPUStandardDescriptor>(uint32_t size)
{
    return mStandardAllocator.Allocate(size);
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

GPUDescriptorHandle::GPUDescriptorHandle(const Range& allocation, D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap) 
    : mAllocation(allocation)
    , mType(type)
{ 
    mCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart(), static_cast<uint32_t>(allocation.Start), Graphic::Get().GetHandleSize(mType));
    mGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(), static_cast<uint32_t>(allocation.Start), Graphic::Get().GetHandleSize(mType));
}

GPUDescriptorHandle::~GPUDescriptorHandle()
{
    Assert(!mAllocation.IsValid());
}

GPUDescriptorHandleScoped::~GPUDescriptorHandleScoped()
{
    GPUDescriptorHeap* heap = Graphic::Get().GetGPUDescriptorHeap(mType);
    heap->Free(*this);
}

GPUBindlessDescriptorHandle::GPUBindlessDescriptorHandle(const Range& allocation, D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap) : mAllocation(allocation)
, mType(type)
{
    mCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart(), static_cast<uint32_t>(allocation.Start), Graphic::Get().GetHandleSize(mType));
}

GPUBindlessDescriptorHandle::GPUBindlessDescriptorHandle(GPUBindlessDescriptorHandle&& rhs)
{
    *this = std::move(rhs);
}

GPUBindlessDescriptorHandle& GPUBindlessDescriptorHandle::operator=(GPUBindlessDescriptorHandle&& rhs)
{
    mAllocation = rhs.mAllocation;
    mCpuHandle = rhs.mCpuHandle;
    mType = rhs.mType;

    rhs.mAllocation.Invalidate();

    return *this;
}
