#pragma once

#include "hyper_common.h"
#include "hyper_stack_arena.h"
#include <stdbool.h>

/* TODO: I think some of this stuff doesn't belong here, but I don't
   have the big picture yet */

typedef struct
{
  f32 *positions;
  f32 *normals;
  f32 *texture_coordinates;
  f32 *colours;
  u32 count;
} hyper_vertex_buffer;

typedef struct
{
  u32 *pixels;
  u32 pixel_count;
  u32 byte_size;
  i32 width;
  i32 height;
  i32 pitch; /* number of bytes between the start of one row of pixels to the next one. */
  i32 simd_chunks;
} hyper_framebuffer;

typedef struct
{
  hyper_framebuffer *framebuffer;
  hyper_vertex_buffer *vertex_buffer;
  hyper_stack_arena *stack_arena;
} hyper_renderer_context;

typedef struct
{
  hyper_renderer_context *renderer_context;
  u64 last_frame_time;
  f32 fixed_timestep;
  f32 physics_accumulator;
  f32 alpha; /* interpolated rendering */
  bool running;
} hyper_frame_context;

typedef struct
{
  f32 target_fps;
  i32 width;
  i32 height;
  bool vsync;
} hyper_config;
