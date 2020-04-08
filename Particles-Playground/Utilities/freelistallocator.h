#pragma once
#include "memory.h"
#include "allocatorcommon.h"

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
class FreeListAllocator : public BaseAllocator
{
public:
    FreeListAllocator(uint64_t startRange, uint64_t endRange)
        : BaseAllocator(startRange, endRange)
    {
        mFreeList.push_back({ mStartRange, mEndRange });
    }

    ~FreeListAllocator() = default;

    virtual Range Allocate(uint32_t size, uint32_t alignment = 1) override;
    virtual void Free(Range& range) override;

private:
    std::list<Range> mFreeList;

};

#include "freelistallocator.inl"
