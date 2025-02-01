#ifndef XCALIBER_H
#define XCALIBER_H

#include "xcaliber_common.h"

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct xcr_context xcr_context;

/* XC stands for (X)-(C)aliber */

/* Platform Abstraction Layer */
typedef struct xc_ctx
{
  xcr_context *renderer_ctx;
  uint64_t last_frame_time;
  f32_t fixed_timestep;
  f32_t physics_accumulator;
  f32_t alpha;
  bool running;
} xc_ctx;

typedef struct xc_cfg
{
  f32_t target_fps;
  int32_t width;
  int32_t height;
  bool vsync;
} xc_cfg;

typedef struct xc_framebuffer
{
  uint32_t *pixels;
  uint32_t pixel_count;
  uint32_t byte_size;
  int32_t width;
  int32_t height;
  int32_t pitch; /* Or stride. Number of bytes between the start of one row of pixels to the next one. */
  int32_t simd_chunks;
} xc_framebuffer;

#endif
