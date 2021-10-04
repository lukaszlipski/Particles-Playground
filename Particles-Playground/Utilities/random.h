#pragma once

namespace RngType
{
    struct PCG
    {
        uint32_t GetRandom(uint32_t seed)
        {
            uint32_t state = seed * 747796405U + 2891336453U;
            uint32_t word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;
            return (word >> 22U) ^ word;
        }
    };

    struct xxHash32
    {
        uint32_t GetRandom(uint32_t seed)
        {
            const uint32_t PRIME32_2 = 2246822519U;
            const uint32_t PRIME32_3 = 3266489917U;
            const uint32_t PRIME32_4 = 668265263U;
            const uint32_t PRIME32_5 = 374761393U;

            uint32_t h32 = seed + PRIME32_5;
            h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
            h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
            h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
            return h32 ^ (h32 >> 16);
        }
    };
}

template<typename Type = RngType::PCG>
class RandomNumberGenerator
{
public:
    RandomNumberGenerator(uint32_t seed)
        : mInternalSeed(seed)
    { }

    ~RandomNumberGenerator() = default;

    uint32_t GetRandom()
    {
        mInternalSeed = mEngine.GetRandom(mInternalSeed);
        return mInternalSeed;
    }

    RandomNumberGenerator(const RandomNumberGenerator&) = default;
    RandomNumberGenerator(RandomNumberGenerator&&) = default;

    RandomNumberGenerator& operator=(const RandomNumberGenerator&) = default;
    RandomNumberGenerator& operator=(RandomNumberGenerator&&) = default;

private:
    uint32_t mInternalSeed = 0;
    Type mEngine;
};
