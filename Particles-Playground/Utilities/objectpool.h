#pragma once
#include "Utilities/freelistallocator.h"

template<typename Type, uint32_t IndexBitsNum = 24, typename = std::enable_if_t<std::is_unsigned_v<Type> && std::is_integral_v<Type>>>
class HandleBase
{
    static_assert(IndexBitsNum < std::numeric_limits<Type>::digits - 1, "Too many bits reserved for Index for a given type, at least one bit has to be left for Generation");
    static const uint32_t GenerationBitsNum = std::numeric_limits<Type>::digits - IndexBitsNum;

public:
    static const Type Invalid = std::numeric_limits<Type>::max();
    static const Type IndexMax = (1 << IndexBitsNum) - 1;
    static const Type GenerationMax = (1 << GenerationBitsNum) - 1;
    using InnerType = typename Type;

    HandleBase()
        : mHandle(Invalid)
    { }

    HandleBase(Type index, Type generation)
        : mIndex(index), mGeneration(generation)
    { }

    ~HandleBase() { mHandle = Invalid; }

    HandleBase(HandleBase&& rhs)
    {
        *this = std::move(rhs);
    }

    HandleBase& operator=(HandleBase&& rhs)
    {
        mHandle = rhs.mHandle;
        rhs.mHandle = Invalid;
        return *this;
    }

    HandleBase(const HandleBase&) = default;
    HandleBase& operator=(const HandleBase&) = default;

    inline Type GetIndex() const { return mIndex; }
    inline Type GetGeneration() const { return mGeneration; }
    inline Type GetHandle() const { return mHandle; }

protected:
    union
    {
        struct
        {
            Type mIndex : IndexBitsNum;
            Type mGeneration : GenerationBitsNum;
        };
        Type mHandle;
    };
};

template<typename ObjectType>
class ObjectPool;

template<typename ObjectType>
class IObject
{
    friend class ObjectPool<ObjectType>;
    
public:
    using HandleType = HandleBase<uint32_t, 24>;
    using IndexType = HandleType::InnerType;

    IObject()
        : mIndex(HandleType::Invalid)
    { }

    ~IObject() { mIndex = HandleType::Invalid; }

    IObject(const IObject&) = delete;
    IObject(IObject&&) = delete;

    IObject& operator=(const IObject&) = delete;
    IObject& operator=(IObject&& rhs) = delete;

    inline IndexType GetIndex() const { return mIndex; }

private:
    IndexType mIndex;
};

template<typename ObjectType>
using ObjectHandle = typename IObject<ObjectType>::HandleType;

template<typename ObjectType>
class ObjectPool
{
    static_assert(std::is_base_of_v<IObject<ObjectType>, ObjectType>, "ObjectType has to derive from IObject");
    static_assert(std::numeric_limits<uint8_t>::max() >= ObjectHandle<ObjectType>::GenerationMax, "Generation's type is too small to hold the Object's Handle Generation");

    using IndexType = typename ObjectHandle<ObjectType>::InnerType;

    struct DefaultPredicate
    {
        bool operator()(ObjectType* object) const
        {
            return true;
        }
    };

public:
    ObjectPool(uint32_t numObjects)
        : mNumObjects(numObjects)
        , mAllocator(0, numObjects)
    { 
        assert(mNumObjects != 0 && mNumObjects <= ObjectHandle<ObjectType>::IndexMax); // Number of objects is too big for current Object's Handle or equal zero
    }

    void Init();
    void Free();

    template<typename... Args>
    [[nodiscard]] ObjectHandle<ObjectType> AllocateObject(Args&&... args);

    void FreeObject(ObjectHandle<ObjectType>& handle);
    ObjectType* GetObject(ObjectHandle<ObjectType> handle);
    bool ValidateHandle(ObjectHandle<ObjectType> handle) const;

    template<typename Pred = DefaultPredicate>
    [[nodiscard]] std::vector<ObjectType*> GetObjects(Pred predicate = {}) const;
    
private:
    FreeListAllocator<FirstFitStrategy> mAllocator;
    uint32_t mNumObjects = 0;
    ObjectType* mObjectMemory = nullptr;
    std::vector<uint8_t> mGenerations;
    std::vector<bool> mValidObjects;
};

#include "Utilities/objectpool.inl"
