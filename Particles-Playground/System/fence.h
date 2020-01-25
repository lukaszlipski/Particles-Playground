#pragma once

enum class QueueType;

class Fence
{
public:
    explicit Fence();
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Fence&& rhs);
    Fence& operator=(Fence&& rhs);

    void Flush(QueueType type);
    void Signal(QueueType type);
    void Wait(QueueType type);
    void WaitOnCPU();

private:
    ID3D12Fence* mFence = nullptr;
    uint64_t mValue = 0;
    HANDLE mEvent = nullptr;

};

using upFence = std::unique_ptr<Fence>;

