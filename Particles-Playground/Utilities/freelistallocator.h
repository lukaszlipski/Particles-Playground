#pragma once
#include "memory.h"

struct Range
{
    static const uint64_t Invalid = std::numeric_limits<uint64_t>::max();

    uint64_t Start = Invalid;
    uint64_t Size = Invalid;

    bool IsValid() const {
        return Start != Invalid && Size != Invalid;
    }
};

struct FirstFitStrategy
{
    static std::list<Range>::iterator FindFreeRange(std::list<Range>& freeList, uint32_t size, uint32_t alignment)
    {
        auto firstFit = std::find_if(freeList.begin(), freeList.end(), [size, alignment](const Range& block) {
            const uint64_t diff = Align(block.Start, static_cast<uint64_t>(alignment)) - block.Start;
            return (block.Size - diff) >= size;
        });

        return firstFit;
    }
};

template<typename AllocStrategy>
class FreeListAllocator
{
public:
    FreeListAllocator(uint64_t startRange, uint64_t endRange)
        : mStartRange(startRange), mEndRange(endRange)
    {
        mFreeList.push_back({ mStartRange, mEndRange });
    }

    ~FreeListAllocator() = default;

    Range Allocate(uint32_t size, uint32_t alignment = 1);

    void Free(Range range);

private:

    std::list<Range> mFreeList;
    uint64_t mStartRange = 0;
    uint64_t mEndRange = 0;

};

#include "freelistallocator.inl"
