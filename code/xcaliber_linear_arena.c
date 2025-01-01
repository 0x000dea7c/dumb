#include "xcaliber_linear_arena.h"

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#if defined (__AVX2__)
#define DEFAULT_ALIGNMENT (4 * sizeof(void *))
#else
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

static bool
ispowof2(uintptr_t x)
{
	return (x & (x - 1)) == 0;
}

static uintptr_t
align_forward(uintptr_t ptr, size_t align)
{
	assert(ispowof2(align) && "this ain't a power of 2");
	uintptr_t p = ptr, a = align;
	uintptr_t mod = p & (a - 1); /* p % a, but faster */
	if (mod != 0) {
		p += a - mod;
	}
	return p;
}

static void *
linear_arena_alloc_align(linear_arena *a, size_t size, size_t align)
{
	/* align curr_offset forward */
	uintptr_t curr_ptr = (uintptr_t)a->buf + (uintptr_t)a->curr_offset;
	uintptr_t offset = align_forward(curr_ptr, align);
	offset -= (uintptr_t)a->buf;

	if (offset + size > a->buf_len) {
		/* FIXME: I shouldn't be using fprintf here directly */
		(void)fprintf(stderr, "No mem available in linear_arena!\n");
		return NULL;
	}

	void *ptr = &a->buf[offset];
	a->prev_offset = offset;
	a->curr_offset = offset + size;
	memset(ptr, 0, size);
	return ptr;
}

static void *
linear_arena_resize_align(linear_arena *a, void *old_m, size_t old_size,
			  size_t new_size, size_t align)
{
	/* FIXME: this function is pretty ugly, need to refactor! */
	assert(ispowof2(align));

	unsigned char *old_mem = (unsigned char *)old_m;

	if (old_m == NULL || old_size == 0) {
		return linear_arena_alloc(a, new_size);
	}

	/* if old memory is part of this linear_arena */
	if (a->buf <= old_mem && old_mem < a->buf + a->buf_len) {
		/* is this old memory my last allocation? then resize */
		if (a->buf + a->prev_offset == old_mem) {
			a->curr_offset = a->prev_offset + new_size;
			if (new_size > old_size) {
				if (a->curr_offset + (new_size - old_size) >
				    a->buf_len) {
					/* FIXME: shouldn't be using fprintf here */
					(void)fprintf(
						stderr,
						"Out of linear_arena space!");
					return NULL;
				}
				memset(&a->buf[a->curr_offset], 0,
				       new_size -
					       old_size); /* careful not to zero out existing memory */
			}
			return old_mem;
		}
		/* old memory is not my previous allocation, so I need to allocate a new
		   chunk of memory at the current position of the linear_arena, then copy the old
		   data into this new location. You don't want to get in here too often bc
		   the old memory location is now effectively wasted. */
		void *new_mem = linear_arena_alloc_align(a, new_size, align);
		size_t copy_size = old_size < new_size ? old_size : new_size;
		memmove(new_mem, old_mem, copy_size);
		return new_mem;
	}

	assert(false &&
	       "This memory addr doesn't belong to this linear_arena!");
	return NULL;
}

void
linear_arena_init(linear_arena *a, unsigned char *buf, size_t buf_len)
{
	a->buf = buf;
	a->buf_len = buf_len;
	a->curr_offset = a->prev_offset = 0;
}

void *
linear_arena_alloc(linear_arena *a, size_t size)
{
	return linear_arena_alloc_align(a, size, DEFAULT_ALIGNMENT);
}

void
linear_arena_free(linear_arena *a)
{
	a->curr_offset = a->prev_offset = 0;
}

void *
linear_arena_resize(linear_arena *a, void *old_mem, size_t old_size,
		    size_t new_size)
{
	return linear_arena_resize_align(a, old_mem, old_size, new_size,
					 DEFAULT_ALIGNMENT);
}
