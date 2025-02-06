#pragma once

#include <stdint.h>

#define HYPER_ARRAY_COUNT(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define HYPER_KILOBYTES(number) ((number) * 1024ull)
#define HYPER_MEGABYTES(number) (HYPER_KILOBYTES(number) * 1024ull)
#define HYPER_IS_POWER_OF_TWO(a) (((a) != 0) && ((a) & ((a) - 1)) == 0)
#define HYPER_SWAP(type, a, b)                  \
  do {                                          \
    type SWAP_tmp = b;                          \
    b = a;                                      \
    a = SWAP_tmp;                               \
  } while (0)
#define HYPER_GAME_UPDATE_FUNCTION_NAME "hyper_game_update"
#define HYPER_GAME_RENDER_FUNCTION_NAME "hyper_game_render"

#define f32 float
#define f64 double
#define i32 int32_t
#define u32 uint32_t
#define u8  uint8_t
#define u64 uint64_t
#define i64 int64_t
