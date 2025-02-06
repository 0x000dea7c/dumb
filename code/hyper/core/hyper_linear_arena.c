#include "hyper_linear_arena.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* NOTE: working with a CPU that supports AVX2 */
#define DEFAULT_ALIGNMENT (4 * sizeof (void *))

static uintptr_t
align_forward (uintptr_t ptr, u32 alignment)
{
  uintptr_t p = ptr;
  uintptr_t const a = alignment;
  uintptr_t const mod = p & (a - 1); /* p % a, but faster */

  if (mod != 0)
    {
      p += a - mod;
    }

  return p;
}

static void *
align_allocate (hyper_linear_arena *arena, u32 size, u32 alignment)
{
  uintptr_t current_ptr = (uintptr_t) arena->buffer + (uintptr_t) arena->current_offset;
  uintptr_t offset = align_forward (current_ptr, alignment);

  offset -= (uintptr_t) arena->buffer;

  if (offset + size > arena->buffer_length)
    {
      assert (false && "no memory available in your linear arena!\n");
      return NULL;
    }

  void *ptr = &arena->buffer[offset];
  arena->previous_offset = (u32) offset;
  arena->current_offset = (u32) offset + size;
  memset (ptr, 0, size);

  return ptr;
}

static void *
resize_alignment (hyper_linear_arena *arena, void *old_memory, u32 old_size, u32 new_size, u32 alignment)
{
  u8 *old_memory_c = (u8 *)old_memory;

  if (old_memory_c == NULL || old_size == 0)
    {
      return hyper_linear_arena_alloc (arena, new_size);
    }

  /* if old memory isn't part of this hyper_linear_arena */
  if (!(arena->buffer <= old_memory_c && old_memory_c < arena->buffer + arena->buffer_length))
    {
      assert (false && "This memory addr doesn't belong to this arena!");
      return NULL;
    }

  /* old memory is not my previous allocation, so I need to allocate a
     new chunk of memory at the current position of the linear_arena,
     then copy the old data into this new location. I don't want to
     get in here too often bc the old memory location is now
     effectively wasted. */
  if (! (arena->buffer + arena->previous_offset == old_memory))
    {
      void *new_memory = align_allocate (arena, new_size, alignment);
      u32 copy_size = old_size < new_size ? old_size : new_size;
      memmove (new_memory, old_memory, copy_size);
      return new_memory;
    }

  /* is this old memory my last allocation? then resize */
  arena->current_offset = arena->previous_offset + new_size;

  if (new_size > old_size)
    {
      if (arena->current_offset + (new_size - old_size) > arena->buffer_length)
        {
          assert (false && "out of linear arena space!");
          return NULL;
        }

      memset (&arena->buffer[arena->current_offset], 0, new_size - old_size);
    }

  return old_memory;
}

void
hyper_linear_arena_init (hyper_linear_arena *arena, u8 *buffer, u32 buffer_length)
{
  arena->buffer = buffer;
  arena->buffer_length = buffer_length;
  arena->current_offset = arena->previous_offset = 0;
}

void *
hyper_linear_arena_alloc (hyper_linear_arena *arena, u32 size)
{
  return align_allocate (arena, size, DEFAULT_ALIGNMENT);
}

void
hyper_linear_arena_free (hyper_linear_arena *arena)
{
  arena->current_offset = arena->previous_offset = 0;
}

void *
hyper_linear_arena_resize (hyper_linear_arena *arena, void *old_memory, u32 old_size, u32 new_size)
{
  return resize_alignment (arena, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}
