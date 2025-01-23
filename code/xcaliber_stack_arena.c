#include "xcaliber_stack_arena.h"
#include "xcaliber_common.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(__AVX2__)
#define DEFAULT_ALIGNMENT (4 * sizeof(void *))
#else
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

/* Stores the amount of bytes that has to be placed before the header
   in order to have the new allocation correctly aligned */
typedef struct stack_alloc_header {
	uint8_t padding;
} stack_alloc_header;

/* Small stack allocator */
struct stack_arena {
	unsigned char *buf;
	uint64_t buf_len;
	uint64_t offset;
};

static uint64_t
compute_padding_with_header(uintptr_t ptr, uintptr_t align, uintptr_t header_size)
{
	assert(XC_IS_POWER_OF_TWO(align) && "alignment isn't a power of 2");
	uintptr_t p = ptr, a = align, padding = 0, needed_space = (uintptr_t)header_size;
	uintptr_t const mod = p & (a - 1); /* p % a, but faster. Assume a is a power of 2 */

	if (mod != 0) {
		padding = a - mod;
	}

	if (padding < needed_space) {
		needed_space -= padding;

		if ((needed_space & (a - 1)) != 0) {
			padding += a * (1 + (needed_space / a));
		} else {
			padding += a * (needed_space / a);
		}
	}

	return (uint64_t)padding;
}

static void *
stack_alloc_align(stack_arena *a, uint64_t size, uint64_t align)
{
	assert(XC_IS_POWER_OF_TWO(align) && "alignment isn't a power of 2");
	assert(align <= 128 && "alignment cannot exceed 128!");

	stack_alloc_header *header;
	uintptr_t const curr_addr = (uintptr_t)a->buf + (uintptr_t)a->offset;
	uint64_t const padding = compute_padding_with_header(curr_addr, (uintptr_t)align, sizeof(stack_alloc_header));

	if (a->offset + padding + size > a->buf_len) {
		(void)fprintf(stderr, "There isn't enough space in the arena");
		return NULL;
	}

	/* Padding done */
	a->offset += padding;

	/* Now header placement */
	/* where our aligned memory will start (already considers the header) */
	uintptr_t const next_addr = curr_addr + (uintptr_t)padding;

	/* place the header just before the aligned memory */
	header = (stack_alloc_header *)(next_addr - sizeof(*header));
	header->padding = (uint8_t)padding;

	a->offset += size;

	return memset((void *)next_addr, 0, size);
}

stack_arena *
stack_arena_create(void)
{
	return malloc(sizeof(stack_arena));
}

void
stack_arena_init(stack_arena *a, void *buf, uint64_t buf_len)
{
	a->buf = (unsigned char *)buf;
	a->buf_len = buf_len;
	a->offset = 0;
}

void *
stack_arena_alloc(stack_arena *a, uint64_t size)
{
	return stack_alloc_align(a, size, DEFAULT_ALIGNMENT);
}

void
stack_arena_free(stack_arena *a, void *ptr)
{
	/* This doesn't actually enforce the LIFO principle, but whatever */
	if (!ptr) {
		return;
	}

	stack_alloc_header const *header;
	uintptr_t const start = (uintptr_t)a->buf;
	uintptr_t const end = start + (uintptr_t)a->buf_len;
	uintptr_t const curr = (uintptr_t)ptr;

	if (!(start <= curr && end > curr)) {
		assert(false && "This pointer doesn't belong to this arena!");
		return;
	}

	if (curr >= start + (uintptr_t)a->offset) {
		/* This is to say that I'm passing a pointer that was already freed, so I don't do anything. */
		return;
	}

	header = (stack_alloc_header *)(curr - sizeof(*header));
	uint64_t prev_offset = (uint64_t)(curr - header->padding - start);
	a->offset = prev_offset;
}

static void *
arena_resize_align(stack_arena *a, void *ptr, uint64_t old_size, uint64_t new_size, uint64_t align)
{
	if (ptr == NULL) {
		return stack_alloc_align(a, new_size, align);
	}

	if (new_size == 0) {
		stack_arena_free(a, ptr);
		return NULL;
	}

	uint64_t const min_size = new_size < old_size ? new_size : old_size;
	uintptr_t const start = (uintptr_t)a->buf;
	uintptr_t const end = start + a->buf_len;
	uintptr_t const curr = (uintptr_t)ptr;

	if (!(start <= curr && end > curr)) {
		assert(false && "This pointer doesn't belong to this arena!");
		return NULL;
	}

	if (old_size == new_size) {
		/* Trying to resize the old chunk of mem with the same size */
		return ptr;
	}

	if (curr >= start + (uintptr_t)a->offset) {
		/* This is to say that I'm passing a pointer that was already freed, so I don't do anything. */
		return NULL;
	}

	void *new_ptr = stack_alloc_align(a, new_size, align);
	memmove(new_ptr, ptr, min_size);
	return new_ptr;
}

void *
stack_arena_resize(stack_arena *a, void *ptr, uint64_t old_size, uint64_t new_size)
{
	return arena_resize_align(a, ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

void
stack_arena_free_all(stack_arena *a)
{
	a->offset = 0;
}

void
stack_arena_destroy(stack_arena *a)
{
	free(a);
}
