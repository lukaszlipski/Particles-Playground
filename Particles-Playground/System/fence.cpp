#include "fence.h"
#include "graphic.h"

Fence::Fence()
{
    ID3D12Device* device = Graphic::Get().GetDevice();
    assert(SUCCEEDED(device->CreateFence(mValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence))));
    mEvent = CreateEvent(nullptr, false, false, nullptr);
}

Fence::Fence(Fence&& rhs)
{
    *this = std::move(rhs);
}

Fence::~Fence()
{
    if (mFence) { mFence->Release(); }
    if (mEvent) { CloseHandle(mEvent); }
}

Fence& Fence::operator=(Fence&& rhs)
{
    mFence = rhs.mFence;
    mEvent = rhs.mEvent;
    mValue = rhs.mValue;

    rhs.mFence = nullptr;
    rhs.mEvent = nullptr;
    rhs.mValue = 0;

    return *this;
}

void Fence::Flush(QueueType type)
{    
    Signal(type);

    WaitOnCPU();
}

void Fence::Signal(QueueType type)
{
    ID3D12CommandQueue* queue = Graphic::Get().GetQueue(type);
    assert(queue);

    ++mValue;
    queue->Signal(mFence, mValue);
}

void Fence::Wait(QueueType type)
{
    ID3D12CommandQueue* queue = Graphic::Get().GetQueue(type);
    assert(queue);

    queue->Wait(mFence, mValue);
}

void Fence::WaitOnCPU()
{
    if (mFence->GetCompletedValue() < mValue)
    {
        mFence->SetEventOnCompletion(mValue, mEvent);
        WaitForSingleObject(mEvent, INFINITE);
    }
}
