#pragma once

#include <cstdint>

using U0 = void;
using I32 = std::int32_t;
using U32 = std::uint32_t;
using I64 = std::int64_t;
using U64 = std::uint64_t;
using U8 = std::uint8_t;

#define ARRAY_COUNT(arr) (sizeof((arr)) / sizeof((arr[0])))
#define KILOBYTES(n) ((n) * 1024ull)
#define MEGABYTES(n) (KILOBYTES(n) * 1024ull)
#define GIGABYTES(n) (MEGABYTES(n) * 1024ull)
#define MAX(a, b) ((a) > (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (b) : (a))

#ifdef DUMB_DEBUG
#define ASSERT(expr)                            \
  if (!(expr)) {                                \
    *(int*)0 = 0;                               \
  }
#else
#define ASSERT(expr)
#endif
