#include "xcaliber_stack_arena.h"
#include "xcaliber_common.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* AVX2 */
#define DEFAULT_ALIGNMENT (4 * sizeof (void *))

/* stores the amount of bytes that has to be placed before the header
   in order to have the new allocation correctly aligned */
typedef struct stack_alloc_header
{
  uint8_t padding;
} stack_alloc_header;

/* small stack allocator */
struct stack_arena
{
  unsigned char *buffer;
  uint64_t buffer_length;
  uint64_t offset;
};

static uint64_t
compute_padding_with_header (uintptr_t ptr, uintptr_t align, uintptr_t header_size)
{
  assert (XC_IS_POWER_OF_TWO (align) && "alignment isn't a power of 2");

  uintptr_t p = ptr;
  uintptr_t a = align;
  uintptr_t padding = 0;
  uintptr_t needed_space = (uintptr_t) header_size;

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

  return (uint64_t) padding;
}

static void *
stack_alloc_align (stack_arena *arena, uint64_t size, uint64_t align)
{
  assert(XC_IS_POWER_OF_TWO (align) && "alignment isn't a power of 2");
  assert(align <= 128 && "alignment cannot exceed 128!");

  stack_alloc_header *header;
  uintptr_t const current_address = (uintptr_t) arena->buffer + (uintptr_t) arena->offset;
  uint64_t const padding = compute_padding_with_header (current_address,
                                                        (uintptr_t) align,
                                                        sizeof (stack_alloc_header));

  if (arena->offset + padding + size > arena->buffer_length)
    {
      (void) fprintf (stderr, "There isn't enough space in the arena");
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

stack_arena *
stack_arena_create (void)
{
  return malloc (sizeof (stack_arena));
}

void
stack_arena_init (stack_arena *arena, void *buffer, uint64_t buffer_length)
{
  arena->buffer = (unsigned char *) buffer;
  arena->buffer_length = buffer_length;
  arena->offset = 0;
}

void *
stack_arena_alloc (stack_arena *arena, uint64_t size)
{
  return stack_alloc_align (arena, size, DEFAULT_ALIGNMENT);
}

void
stack_arena_free (stack_arena *arena, void *ptr)
{
  /* This doesn't actually enforce the LIFO principle, but whatever */
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
      assert(false && "This pointer doesn't belong to this arena!");
      return;
    }

  if (current >= start + (uintptr_t) arena->offset)
    {
      /* this is to say that I'm passing a pointer that was already freed, so I don't do anything. */
      return;
    }

  header = (stack_alloc_header *) (current - sizeof (*header));
  uint64_t prev_offset = (uint64_t) (current - header->padding - start);
  arena->offset = prev_offset;
}

static void *
arena_resize_align (stack_arena *arena, void *ptr, uint64_t old_size, uint64_t new_size, uint64_t align)
{
  if (!ptr)
    {
      return stack_alloc_align(arena, new_size, align);
    }

  if (new_size == 0)
    {
      stack_arena_free(arena, ptr);
      return NULL;
    }

  uint64_t  const min_size = new_size < old_size ? new_size : old_size;
  uintptr_t const start = (uintptr_t) arena->buffer;
  uintptr_t const end = start + arena->buffer_length;
  uintptr_t const current = (uintptr_t) ptr;

  if (!(start <= current && end > current))
    {
      assert(false && "This pointer doesn't belong to this arena!");
      return NULL;
    }

  if (old_size == new_size)
    {
      /* Trying to resize the old chunk of mem with the same size */
      return ptr;
    }

  if (current >= start + (uintptr_t) arena->offset)
    {
      /* This is to say that I'm passing a pointer that was already freed, so I don't do anything. */
      return NULL;
    }

  void *new_ptr = stack_alloc_align (arena, new_size, align);
  memmove(new_ptr, ptr, min_size);
  return new_ptr;
}

void *
stack_arena_resize (stack_arena *arena, void *ptr, uint64_t old_size, uint64_t new_size)
{
  return arena_resize_align(arena, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void
stack_arena_free_all (stack_arena *arena)
{
  arena->offset = 0;
}

void
stack_arena_destroy (stack_arena *arena)
{
  free (arena);
}
