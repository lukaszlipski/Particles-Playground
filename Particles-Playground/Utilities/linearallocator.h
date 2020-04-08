#pragma once
#include "allocatorcommon.h"

class LinearAllocator : public BaseAllocator
{
public:
    LinearAllocator(uint64_t startRange, uint64_t endRange)
        : BaseAllocator(startRange, endRange)
    { }

    virtual Range Allocate(uint32_t size, uint32_t alignment = 1) override;
    virtual void Free(Range& range) override;

    void Clear();

private:
    uint64_t mCurrentPointer = 0;

};
