#include "cpudescriptorheap.h"
#include "System/graphic.h"

CPUDescriptorHeap::CPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : mType(type), mAllocator(0, DescriptorNum)
{
    ID3D12Device* const device = Graphic::Get().GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = DescriptorNum;
    desc.Type = mType;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mHeap));
    assert(SUCCEEDED(hr));
}

CPUDescriptorHeap::~CPUDescriptorHeap()
{
    mHeap->Release();
}

CPUDescriptorHeap::CPUDescriptorHeap(CPUDescriptorHeap&& rhs)
    : mAllocator(0, DescriptorNum)
{
    *this = std::move(rhs);
}

CPUDescriptorHeap& CPUDescriptorHeap::operator=(CPUDescriptorHeap&& rhs)
{
    this->mAllocator = std::move(rhs.mAllocator);
    this->mHeap = rhs.mHeap;
    this->mType = rhs.mType;

    rhs.mHeap = nullptr;

    return *this;
}

CPUDescriptorHandle CPUDescriptorHeap::Allocate()
{
    Range alloc = mAllocator.Allocate(1);
    assert(alloc.IsValid());

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<uint32_t>(alloc.Start), GetHandleSize(mType));
    return CPUDescriptorHandle(alloc, handle);
}

void CPUDescriptorHeap::Free(CPUDescriptorHandle& handle)
{
    if (!handle.IsValid()) { return; }

    mAllocator.Free(handle.GetAllocation());
}

uint32_t CPUDescriptorHeap::GetHandleSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    switch (type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
        return Graphic::Get().GetCBVHandleSize();
    default:
        return Graphic::Get().GetRTVHandleSize();
    }
}
