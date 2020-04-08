#pragma once

template<typename T>
constexpr bool IsPow2(T number) {
    return ((number & (number - 1)) == 0) && number;
}

template<typename T1, typename T2>
constexpr T1 AlignPow2(T1 number, T2 alignment)
{
    assert(IsPow2(alignment));
    alignment -= 1;
    return (number + alignment) & ~alignment;
}

template<typename T1, typename T2>
constexpr T1 Align(T1 number, T2 alignment)
{
    return ((number + (alignment - 1)) / alignment) * alignment;
}
