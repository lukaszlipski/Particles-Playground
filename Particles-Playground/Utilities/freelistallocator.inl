template<typename AllocStrategy>
void FreeListAllocator<AllocStrategy>::Free(Range& range)
{
    if (!range.IsValid()) { return; }

    const uint64_t start = range.Start;
    const uint64_t size = range.Size;

    auto left = std::find_if(mFreeList.begin(), mFreeList.end(), [start](const Range& block) {
        return block.Start + block.Size == start;
        });

    auto right = std::find_if(mFreeList.rbegin(), mFreeList.rend(), [start, size](const Range& block) {
        return block.Start == start + size;
        });

    if (left != mFreeList.end() && right == mFreeList.rend()) // we can merge only with left block
    {
        left->Size += size;
    }
    else if (left == mFreeList.end() && right != mFreeList.rend()) // we can merge only with right block
    {
        right->Start = start;
        right->Size += size;
    }
    else if (left != mFreeList.end() && right != mFreeList.rend()) // we can merge from both sides
    {
        left->Size += size + right->Size;
        mFreeList.erase((++right).base());
    }
    else if (left == mFreeList.end() && right == mFreeList.rend()) // we can't merge with any block
    {
        auto firstGreater = std::find_if(mFreeList.begin(), mFreeList.end(), [start](const Range& block) {
            return start > block.Start;
            });

        if (firstGreater != mFreeList.end())
        {
            mFreeList.insert(firstGreater, { start,size });
        }
        else
        {
            mFreeList.push_front({ start, size });
        }
    }

    --mAllocationNum;
    range.Invalidate();
}

template<typename AllocStrategy>
Range FreeListAllocator<AllocStrategy>::Allocate(uint32_t size, uint32_t alignment)
{
    Range result{};

    auto freeBlock = AllocStrategy::FindFreeRange(mFreeList, size, alignment);

    if (freeBlock == mFreeList.end()) // Cannot find a block which contains enough memory
    {
        return result;
    }

    // #TODO(llipski): return range with a properly align start

    result.Start = freeBlock->Start;
    result.Size = size;

    freeBlock->Size -= size;
    freeBlock->Start += size;

    if (freeBlock->Size == 0)
    {
        mFreeList.erase(freeBlock);
    }

    ++mAllocationNum;
    return result;
}
