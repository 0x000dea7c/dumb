#pragma once

#include "hyper_common.h"

typedef struct
{
  u8 *buffer;
  u32 buffer_length;
  u32 previous_offset;
  u32 current_offset;
} hyper_linear_arena;

void  hyper_linear_arena_init (hyper_linear_arena *, u8 *, u32);

void *hyper_linear_arena_alloc (hyper_linear_arena *, u32);

void  hyper_linear_arena_free (hyper_linear_arena *);

void *hyper_linear_arena_resize (hyper_linear_arena *, void *, u32, u32);

void  hyper_linear_arena_destroy (hyper_linear_arena *);
