#include "linearallocator.h"
#include "memory.h"

Range LinearAllocator::Allocate(uint32_t size, uint32_t alignment)
{
    uint64_t result = Align(mCurrentPointer, alignment);

    if (result + size > mEndRange) { return {}; }

    mCurrentPointer = result + size;
    ++mAllocationNum;
    return { result, size };
}

void LinearAllocator::Free(Range& range)
{
    if (IsAllocationValid(range))
    {
        --mAllocationNum;
        range.Invalidate();
    }
}

void LinearAllocator::Clear()
{
    Assert(mAllocationNum != 0);
    mCurrentPointer = mStartRange;
}
