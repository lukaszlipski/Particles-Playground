
template<typename ObjectType>
void ObjectPool<ObjectType>::Init()
{
    // Allocate memory for objects
    uint32_t size = mNumObjects * sizeof(ObjectType);
    void* memory = std::malloc(size);
    std::memset(memory, 0xFE, size);

    mObjectMemory = reinterpret_cast<ObjectType*>(memory);
 
    mGenerations.resize(mNumObjects);
    mValidObjects.resize(mNumObjects);
}

template<typename ObjectType>
void ObjectPool<ObjectType>::Free()
{
    // Make sure all objects were properly cleanup before calling the Free function
    assert(std::none_of(mValidObjects.cbegin(), mValidObjects.cend(), [](bool elem) { return elem; }));

    std::free(mObjectMemory);
    mObjectMemory = nullptr;
}

template<typename ObjectType>
template<typename... Args>
ObjectHandle<ObjectType> ObjectPool<ObjectType>::AllocateObject(Args&&... args)
{
    Range allocation = mAllocator.Allocate(1);
    assert(allocation.IsValid());

    // Convert allocation to index
    const IndexType index = static_cast<IndexType>(allocation.Start);

    // Call the constructor with given arguments
    new (&mObjectMemory[index]) (ObjectType) (std::forward<Args>(args)...);
    ObjectType& object = mObjectMemory[index];
    object.mIndex = index;

    mValidObjects[index] = true;

    return ObjectHandle<ObjectType>(index, mGenerations[index]);
}

template<typename ObjectType>
void ObjectPool<ObjectType>::FreeObject(ObjectHandle<ObjectType>& handle)
{
    if (handle.GetHandle() == ObjectHandle<ObjectType>::Invalid) { return; }

    assert(ValidateHandle(handle));

    // Call destructor on the object
    ObjectType& object = mObjectMemory[handle.GetIndex()];
    object.~ObjectType();
    std::memset(&object, 0xFE, sizeof(ObjectType));

    mValidObjects[handle.GetIndex()] = false;

    // Increase object's generation to mark that existing handles to this object are no longer valid
    ++mGenerations[handle.GetIndex()];

    // Free the allocation
    Range allocation;
    allocation.Start = handle.GetIndex();
    allocation.Size = 1;
    mAllocator.Free(allocation);

    // Return invalid handle
    handle = {};
}

template<typename ObjectType>
ObjectType* ObjectPool<ObjectType>::GetObject(ObjectHandle<ObjectType> handle)
{
    assert(ValidateHandle(handle));

    ObjectType& object = mObjectMemory[handle.GetIndex()];
    return &object;
}

template<typename ObjectType>
bool ObjectPool<ObjectType>::ValidateHandle(ObjectHandle<ObjectType> handle) const
{
    if( !(handle.GetIndex() < mNumObjects) ) { return false; }
    if( !mValidObjects[handle.GetIndex()] ) { return false; }
    if( mGenerations[handle.GetIndex()] != handle.GetGeneration() ) { return false; }
    return true;
}

template<typename ObjectType>
template<typename Pred>
std::vector<ObjectType*> ObjectPool<ObjectType>::GetObjects(Pred predicate) const
{
    std::vector<ObjectType*> result;
    result.reserve(mAllocator.GetAllocationNum());

    for(uint32_t i = 0; i < mNumObjects; ++i)
    {
        if (result.size() == mAllocator.GetAllocationNum()) { break; }

        ObjectType* object = &mObjectMemory[i];
        const bool isValid = mValidObjects[i];
        if(isValid && predicate(object))
        {
            result.push_back(object);
        }
    }

    return result;
}

