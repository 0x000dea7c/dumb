#ifndef XCALIBER_LINEAR_ARENA_H
#define XCALIBER_LINEAR_ARENA_H

#include <stdint.h>

typedef struct linear_arena linear_arena;

void *linear_arena_create(void);

void linear_arena_init(linear_arena *, unsigned char *, uint32_t);

void *linear_arena_alloc(linear_arena *, uint32_t);

void linear_arena_free(linear_arena *);

void *linear_arena_resize(linear_arena *, void *, uint32_t, uint32_t);

void linear_arena_destroy(linear_arena *);

#endif
