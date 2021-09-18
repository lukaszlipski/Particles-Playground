#pragma once

struct Range
{
    static const uint64_t Invalid = std::numeric_limits<uint64_t>::max();

    uint64_t Start = Invalid;
    uint64_t Size = Invalid;

    bool IsValid() const {
        return Start != Invalid && Size != Invalid;
    }

    void Invalidate()
    {
        Start = Invalid;
        Size = Invalid;
    }

};

class BaseAllocator
{
public:
    BaseAllocator(uint64_t startRange, uint64_t endRange)
        : mStartRange(startRange), mEndRange(endRange)
    { }

    virtual ~BaseAllocator()
    {
        assert(mAllocationNum == 0);
    }

    virtual Range Allocate(uint32_t size, uint32_t alignment = 1) = 0;
    virtual void Free(Range& range) = 0;
    inline uint32_t GetAllocationNum() const { return mAllocationNum; }

protected:
    uint64_t mStartRange = 0;
    uint64_t mEndRange = 0;
    uint32_t mAllocationNum = 0;
};
