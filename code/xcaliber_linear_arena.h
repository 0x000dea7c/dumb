#ifndef XCALIBER_LINEAR_ARENA_H
#define XCALIBER_LINEAR_ARENA_H

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	unsigned char *buf;
	uint32_t buf_len;
	uint32_t prev_offset;
	uint32_t curr_offset;
} linear_arena;

void linear_arena_init(linear_arena *a, unsigned char *buf, uint32_t buf_len);

void *linear_arena_alloc(linear_arena *a, uint32_t size);

void linear_arena_free(linear_arena *a);

void *linear_arena_resize(linear_arena *a, void *old_mem, uint32_t old_size,
			  uint32_t new_size);

#endif
