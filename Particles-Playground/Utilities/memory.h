#pragma once

template<typename T>
constexpr bool IsPow2(T number) {
    return ((number & (number - 1)) == 0) && number;
}

template<typename T>
constexpr T AlignPow2(T number, T alignment)
{
    assert(IsPow2(alignment));
    alignment -= 1;
    return (number + alignment) & ~alignment;
}

template<typename T>
constexpr T Align(T number, T alignment)
{
    return ((number + (alignment - 1)) / alignment) * alignment;
}
