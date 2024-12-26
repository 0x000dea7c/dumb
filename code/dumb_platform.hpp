#pragma once

#include <cstdint>
#include <cassert>
#include <type_traits>

using U0  = void;
using I32 = std::int32_t;
using U32 = std::uint32_t;
using I64 = std::int64_t;
using U64 = std::uint64_t;
using U8  = std::uint8_t;
using U16 = std::uint16_t;
using I16 = std::int16_t;
using B32 = std::int32_t;

#define global static
#define internal static
#define persistent static

template <typename T> inline constexpr U64 array_count(T const& arr) noexcept
{
    // This shit is complicated, so it deserves an explanation:
    // decltype(arr)                -> returns the type of `arr`
    // std::extent_v<decltype(arr)> -> checks if `arr` is an array with a known size at compile time
    // if it is                             -> then compute the count
    // if it's not (a pointer or reference) -> this will return 0 
    return std::extent_v<decltype(arr)> == 0 ? sizeof(arr) / sizeof(arr[0]) : std::extent_v<decltype(arr)>;
}

inline constexpr U64 kilobytes(U64 n) noexcept
{
    return n * 1024ULL;
}

inline constexpr U64 megabytes(U64 n) noexcept
{
    return kilobytes(n) * 1024ULL;
}

inline constexpr U64 gigabytes(U64 n) noexcept
{
    return megabytes(n) * 1024ULL;
}

template <typename T> inline constexpr T max(T a, T b) noexcept
{
    return a > b ? a : b;
}

template <typename T> inline constexpr T min(T a, T b) noexcept
{
    return a < b ? a : b;
}

#ifdef DUMB_DEBUG
#define ASSERT(expr) assert(expr) // call C++'s assert
#else
#define ASSERT(expr) ((void)0)  // don't do anything
#endif
