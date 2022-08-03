#include "circularallocator.h"
#include "memory.h"

//   ___    -> free range
//   ---    -> occupied range
// [range]  -> range to free/allocate

CircularAllocator::CircularAllocator(uint64_t startRange, uint64_t endRange) 
    : BaseAllocator(startRange, endRange), mWritePointer(startRange), mReadPointer(startRange)
{ }

Range CircularAllocator::Allocate(uint32_t size, uint32_t alignment /*= 1*/)
{
    uint64_t start = Align(mWritePointer, alignment);

    Range result{};
    if (mWritePointer >= mReadPointer) // [__R---W[range]__]
    {
        if (start + size > mEndRange) // not enough memory on the right side
        {
            start = mStartRange;
            if (start + size >= mReadPointer) { return result; }
        }
    }
    else // [---W[range]_R---]
    {
        if (start + size >= mReadPointer) { return result; } // not enough memory
    }

    result.Start = start;
    result.Size = size;

    mWritePointer = start + size;
    ++mAllocationNum;
    return result;
}

void CircularAllocator::Free(Range& range)
{
    if (!IsAllocationValid(range)) { return; }

    const uint64_t potentialReadPtr = range.Start + range.Size;
    if (mReadPointer > mWritePointer)
    {
        if (potentialReadPtr > mReadPointer && potentialReadPtr > mWritePointer) // [---W___R[range]--]
        {
            mReadPointer = potentialReadPtr;
        }
        else if (potentialReadPtr < mReadPointer && potentialReadPtr <= mWritePointer) // [[range]---W___R-]
        {
            mReadPointer = potentialReadPtr;
        }
    }
    else // [__R[range]--W__]
    {
        if (potentialReadPtr > mReadPointer) // otherwise, we have already cleared that range of memory
        {
            mReadPointer = potentialReadPtr;
        }
    }

    range.Invalidate();
    --mAllocationNum;
}
