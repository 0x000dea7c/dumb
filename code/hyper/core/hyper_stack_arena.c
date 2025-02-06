#include "hyper_stack_arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* assumes AVX2 */
#define DEFAULT_ALIGNMENT (4 * sizeof (void *))

/* stores the amount of bytes that has to be placed before the header
   in order to have the new allocation correctly aligned */
typedef struct
{
  u8 padding;
} stack_alloc_header;

static u64
compute_padding_with_header (uintptr_t ptr, uintptr_t alignment, uintptr_t header_size)
{
  uintptr_t const p = ptr;
  uintptr_t const a = alignment;
  uintptr_t needed_space = (uintptr_t) header_size;
  uintptr_t padding = 0;

  /* p % a, but faster. Assume a is a power of 2 */
  uintptr_t const mod = p & (a - 1);

  if (mod != 0)
    {
      padding = a - mod;
    }

  if (padding < needed_space)
    {
      needed_space -= padding;

      if ((needed_space & (a - 1)) != 0)
        {
          padding += a * (1 + (needed_space / a));
        }
      else
        {
          padding += a * (needed_space / a);
        }
    }

  return (u64) padding;
}

static void *
align_allocate (hyper_stack_arena *arena, u64 size, u64 alignment)
{
  assert (alignment <= 128 && "alignment cannot exceed 128!");

  stack_alloc_header *header;
  uintptr_t const current_address = (uintptr_t) arena->buffer + (uintptr_t) arena->offset;
  u64 const padding = compute_padding_with_header (current_address,
                                                        (uintptr_t) alignment,
                                                        sizeof (stack_alloc_header));

  if (arena->offset + padding + size > arena->buffer_length)
    {
      assert (false && "there isn't enough space in the arena");
      return NULL;
    }

  /* padding done */
  arena->offset += padding;

  /* now header placement, where my aligned memory will start (already considers the header) */
  uintptr_t const next_addr = current_address + (uintptr_t) padding;

  /* place the header just before the aligned memory */
  header = (stack_alloc_header *) (next_addr - sizeof (*header));
  header->padding = (uint8_t) padding;

  arena->offset += size;

  return memset ((void *) next_addr, 0, size);
}

static void *
resize_alignment (hyper_stack_arena *arena, void *ptr, u64 old_size, u64 new_size, u64 alignment)
{
  if (!ptr)
    {
      return align_allocate (arena, new_size, alignment);
    }

  if (new_size == 0)
    {
      hyper_stack_arena_free (arena, ptr);
      return NULL;
    }

  u64  const min_size = new_size < old_size ? new_size : old_size;
  uintptr_t const start = (uintptr_t) arena->buffer;
  uintptr_t const end = start + arena->buffer_length;
  uintptr_t const current = (uintptr_t) ptr;

  if (!(start <= current && end > current))
    {
      assert (false && "this pointer doesn't belong to this arena!");
      return NULL;
    }

  if (old_size == new_size)
    {
      /* trying to resize the old chunk of mem with the same size */
      return ptr;
    }

  if (current >= start + (uintptr_t) arena->offset)
    {
      /* this is to say that I'm passing a pointer that was already freed, so I don't do anything. */
      return NULL;
    }

  void *new_ptr = align_allocate (arena, new_size, alignment);
  memmove (new_ptr, ptr, min_size);

  return new_ptr;
}

void
hyper_stack_arena_init (hyper_stack_arena *arena, void *buffer, u64 buffer_length)
{
  arena->buffer = (u8 *) buffer;
  arena->buffer_length = buffer_length;
  arena->offset = 0;
}

void *
hyper_stack_arena_alloc (hyper_stack_arena *arena, u64 size)
{
  return align_allocate (arena, size, DEFAULT_ALIGNMENT);
}

void
hyper_stack_arena_free (hyper_stack_arena *arena, void *ptr)
{
  /* this doesn't actually enforce the LIFO principle, but whatever */
  if (!ptr)
    {
      return;
    }

  stack_alloc_header const *header;
  uintptr_t const start = (uintptr_t) arena->buffer;
  uintptr_t const end = start + (uintptr_t) arena->buffer_length;
  uintptr_t const current = (uintptr_t) ptr;

  if (!(start <= current && end > current))
    {
      assert (false && "this pointer doesn't belong to this arena!");
      return;
    }

  if (current >= start + (uintptr_t) arena->offset)
    {
      /* this is to say that I'm passing a pointer that was already freed, so I don't do anything. */
      return;
    }

  header = (stack_alloc_header *) (current - sizeof (*header));
  u64 prev_offset = (u64) (current - header->padding - start);
  arena->offset = prev_offset;
}

void *
hyper_stack_arena_resize (hyper_stack_arena *arena, void *ptr, u64 old_size, u64 new_size)
{
  return resize_alignment (arena, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void
hyper_stack_arena_free_all (hyper_stack_arena *arena)
{
  arena->offset = 0;
}
