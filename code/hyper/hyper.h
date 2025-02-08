#pragma once

#include "hyper_common.h"
#include "hyper_stack_arena.h"
#include "hyper_math.h"
#include "hyper_colour.h"

#include <stdbool.h>

/* typedef enum */
/* { */
/*   HYPER_DRAW_FLAGS_FILLED  = 1 << 0, */
/*   HYPER_DRAW_FLAGS_OUTLINE = 1 << 1, */
/*   HYPER_DRAW_FLAGS_COLOUR_INTERPOLATED = 1 << 2, */
/*   HYPER_DRAW_FLAGS_COLOUR_BASE = 1 << 3 */
/* } hyper_draw_flags; */

typedef enum
{
  HYPER_TRIANGLE,
  HYPER_LINE,
  HYPER_QUAD,
  HYPER_CIRCLE,
} hyper_shape;

typedef struct
{
  hyper_vec3f *positions;
  hyper_vec2f *normals;
  hyper_vec2f *texture_coordinates;
  hyper_vec4f *colours;
  u32 count;
  hyper_shape shape;
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
