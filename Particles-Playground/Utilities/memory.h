#pragma once

template<typename T>
constexpr bool IsPow2(T number) {
    return ((number & (number - 1)) == 0) && number;
}

template<typename T1, typename T2>
constexpr T1 AlignPow2(T1 number, T2 alignment)
{
    Assert(IsPow2(alignment));
    alignment -= 1;
    return (number + alignment) & ~alignment;
}

template<typename T1, typename T2>
constexpr T1 Align(T1 number, T2 alignment)
{
    return ((number + (alignment - 1)) / alignment) * alignment;
}

inline uint32_t HashRange(const uint32_t* start, const uint32_t* end)
{
    uint32_t seed = 0;
    const uint32_t* current = start;
    while (current < end)
    {
        uint32_t value = *current;
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); // Boost's implementation (hash_combine)
        ++current;
    }

    return seed;
}
