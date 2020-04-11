#pragma once
#include "allocatorcommon.h"

class CircularAllocator : public BaseAllocator
{
public:
    CircularAllocator(uint64_t startRange, uint64_t endRange);

    virtual Range Allocate(uint32_t size, uint32_t alignment = 1) override;
    virtual void Free(Range& range) override;

private:
    uint64_t mWritePointer = 0;
    uint64_t mReadPointer = 0;

};
