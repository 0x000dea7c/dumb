#include "xcaliber_linear_arena.h"
#include "xcaliber_common.h"

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_ALIGNMENT (4 * sizeof(void *))

struct linear_arena
{
  unsigned char *buffer;
  uint32_t buffer_len;
  uint32_t previous_offset;
  uint32_t current_offset;
};

static uintptr_t
align_forward (uintptr_t ptr, uint32_t align)
{
  assert (XC_IS_POWER_OF_TWO (align) && "this ain't a power of 2");

  uintptr_t p = ptr, a = align;
  uintptr_t mod = p & (a - 1); /* p % a, but faster */

  if (mod != 0)
    {
      p += a - mod;
    }

  return p;
}

static void *
linear_arena_alloc_align (linear_arena *arena, uint32_t size, uint32_t align)
{
  /* align current_offset forward */
  uintptr_t current_ptr = (uintptr_t)arena->buffer + (uintptr_t)arena->current_offset;
  uintptr_t offset = align_forward (current_ptr, align);

  offset -= (uintptr_t) arena->buffer;

  if (offset + size > arena->buffer_len)
    {
      (void) fprintf (stderr, "no memory available in linear_arena!\n");
      return NULL;
    }

  void *ptr = &arena->buffer[offset];
  arena->previous_offset = (uint32_t) offset;
  arena->current_offset = (uint32_t) offset + size;
  memset(ptr, 0, size);

  return ptr;
}

static void *
linear_arena_resize_align (linear_arena *arena, void *oold_memory, uint32_t old_size, uint32_t new_size, uint32_t align)
{
  assert (XC_IS_POWER_OF_TWO (align));

  unsigned char *old_memory = (unsigned char *)oold_memory;

  if (old_memory == NULL || old_size == 0)
    {
      return linear_arena_alloc (arena, new_size);
    }

  /* if old memory isn't part of this linear_arena */
  if (!(arena->buffer <= old_memory && old_memory < arena->buffer + arena->buffer_len))
    {
      assert(false && "This memory addr doesn't belong to this arena!");
      return NULL;
    }

  /* old memory is not my previous allocation, so I need to allocate a new
     chunk of memory at the current position of the linear_arena, then copy the old
     data into this new location. I don't want to get in here too often bc
     the old memory location is now effectively wasted. */
  if (!(arena->buffer + arena->previous_offset == old_memory))
    {
      void *new_memory = linear_arena_alloc_align (arena, new_size, align);
      uint32_t copy_size = old_size < new_size ? old_size : new_size;
      memmove (new_memory, old_memory, copy_size);
      return new_memory;
    }

  /* is this old memory my last allocation? then resize */
  arena->current_offset = arena->previous_offset + new_size;

  if (new_size > old_size)
    {
      if (arena->current_offset + (new_size - old_size) > arena->buffer_len)
        {
          (void) fprintf (stderr, "out of linear_arena space!");
          return NULL;
        }

      /* careful not to zero out existing memory */
      memset (&arena->buffer[arena->current_offset], 0, new_size - old_size);
    }

  return old_memory;
}

void *
linear_arena_create (void)
{
  return malloc (sizeof (linear_arena));
}

void
linear_arena_init (linear_arena *arena, unsigned char *buffer, uint32_t buffer_len)
{
  arena->buffer = buffer;
  arena->buffer_len = buffer_len;
  arena->current_offset = arena->previous_offset = 0;
}

void *
linear_arena_alloc (linear_arena *arena, uint32_t size)
{
  return linear_arena_alloc_align(arena, size, DEFAULT_ALIGNMENT);
}

void
linear_arena_free (linear_arena *arena)
{
  arena->current_offset = arena->previous_offset = 0;
}

void *
linear_arena_resize (linear_arena *arena, void *old_memory, uint32_t old_size, uint32_t new_size)
{
  return linear_arena_resize_align (arena, old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

void
linear_arena_destroy (linear_arena *arena)
{
  free (arena);
}
