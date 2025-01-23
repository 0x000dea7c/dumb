#ifndef XCALIBER_LINEAR_ARENA_H
#define XCALIBER_LINEAR_ARENA_H

#include <stdint.h>

typedef struct linear_arena linear_arena;

void *linear_arena_create(void);

void linear_arena_init(linear_arena *a, unsigned char *buf, uint32_t buf_len);

void *linear_arena_alloc(linear_arena *a, uint32_t size);

void linear_arena_free(linear_arena *a);

void *linear_arena_resize(linear_arena *a, void *old_mem, uint32_t old_size,
			  uint32_t new_size);

void linear_arena_destroy(linear_arena *a);

#endif
