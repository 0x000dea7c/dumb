#pragma once

#include <stdlib.h>

typedef struct linear_arena linear_arena;

struct linear_arena {
	unsigned char *buf;
	size_t buf_len;
	size_t prev_offset;
	size_t curr_offset;
};

void linear_arena_init(linear_arena *a, unsigned char *buf, size_t buf_len);

void *linear_arena_alloc(linear_arena *a, size_t size);

void linear_arena_free(linear_arena *a);

void *linear_arena_resize(linear_arena *a, void *old_mem, size_t old_size,
			  size_t new_size);
