#ifndef XCALIBER_STACK_ARENA_H
#define XCALIBER_STACK_ARENA_H

#include <stdint.h>

typedef struct stack_arena stack_arena;

stack_arena *stack_arena_create(void);

void stack_arena_init(stack_arena *a, void *buf, uint64_t buf_len);

void *stack_arena_alloc(stack_arena *a, uint64_t size);

void stack_arena_free(stack_arena *a, void *ptr);

void *stack_arena_resize(stack_arena *a, void *ptr, uint64_t old_size, uint64_t new_size);

void stack_arena_free_all(stack_arena *a);

void stack_arena_destroy(stack_arena *a);

#endif
