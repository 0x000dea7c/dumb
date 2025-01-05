#include "xcaliber_linear_arena.h"
#include "xcaliber_common.h"
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#if defined(__AVX2__)
#define DEFAULT_ALIGNMENT (4 * sizeof(void *))
#else
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

static uintptr_t
align_forward(uintptr_t ptr, uint32_t align)
{
	assert(IS_POWER_OF_TWO(align) && "this ain't a power of 2");
	uintptr_t p = ptr, a = align;
	uintptr_t mod = p & (a - 1); /* p % a, but faster */
	if (mod != 0) {
		p += a - mod;
	}
	return p;
}

static void *
linear_arena_alloc_align(linear_arena *a, uint32_t size, uint32_t align)
{
	/* align curr_offset forward */
	uintptr_t curr_ptr = (uintptr_t)a->buf + (uintptr_t)a->curr_offset;
	uintptr_t offset = align_forward(curr_ptr, align);
	offset -= (uintptr_t)a->buf;

	if (offset + size > a->buf_len) {
		(void)fprintf(stderr, "No mem available in linear_arena!\n");
		return NULL;
	}

	void *ptr = &a->buf[offset];
	a->prev_offset = (uint32_t)offset;
	a->curr_offset = (uint32_t)offset + size;
	memset(ptr, 0, size);
	return ptr;
}

static void *
linear_arena_resize_align(linear_arena *a, void *old_m, uint32_t old_size,
			  uint32_t new_size, uint32_t align)
{
	assert(IS_POWER_OF_TWO(align));

	unsigned char *old_mem = (unsigned char *)old_m;

	if (old_m == NULL || old_size == 0) {
		return linear_arena_alloc(a, new_size);
	}

	/* if old memory isn't part of this linear_arena */
	if (!(a->buf <= old_mem && old_mem < a->buf + a->buf_len)) {
		assert(false &&
		       "This memory addr doesn't belong to this arena!");
		return NULL;
	}

	/* old memory is not my previous allocation, so I need to allocate a new
	   chunk of memory at the current position of the linear_arena, then copy the old
	   data into this new location. I don't want to get in here too often bc
	   the old memory location is now effectively wasted. */
	if (!(a->buf + a->prev_offset == old_mem)) {
		void *new_mem = linear_arena_alloc_align(a, new_size, align);
		uint32_t copy_size = old_size < new_size ? old_size : new_size;
		memmove(new_mem, old_mem, copy_size);
		return new_mem;
	}

	/* is this old memory my last allocation? then resize */
	a->curr_offset = a->prev_offset + new_size;

	if (new_size > old_size) {
		if (a->curr_offset + (new_size - old_size) > a->buf_len) {
			(void)fprintf(stderr, "Out of linear_arena space!");
			return NULL;
		}
		/* careful not to zero out existing memory */
		memset(&a->buf[a->curr_offset], 0, new_size - old_size);
	}

	return old_mem;
}

void
linear_arena_init(linear_arena *a, unsigned char *buf, uint32_t buf_len)
{
	a->buf = buf;
	a->buf_len = buf_len;
	a->curr_offset = a->prev_offset = 0;
}

void *
linear_arena_alloc(linear_arena *a, uint32_t size)
{
	return linear_arena_alloc_align(a, size, DEFAULT_ALIGNMENT);
}

void
linear_arena_free(linear_arena *a)
{
	a->curr_offset = a->prev_offset = 0;
}

void *
linear_arena_resize(linear_arena *a, void *old_mem, uint32_t old_size,
		    uint32_t new_size)
{
	return linear_arena_resize_align(a, old_mem, old_size, new_size,
					 DEFAULT_ALIGNMENT);
}
