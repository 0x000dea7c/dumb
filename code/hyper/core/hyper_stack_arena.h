#pragma once

#include "hyper_common.h"

typedef struct
{
  u8 *buffer;
  u64 buffer_length;
  u64 offset;
} hyper_stack_arena;

void  hyper_stack_arena_init (hyper_stack_arena *, void *, u64);

void *hyper_stack_arena_alloc (hyper_stack_arena *, u64);

void  hyper_stack_arena_free (hyper_stack_arena *, void *);

void *hyper_stack_arena_resize (hyper_stack_arena *, void *, u64, u64);

void  hyper_stack_arena_free_all (hyper_stack_arena *);
