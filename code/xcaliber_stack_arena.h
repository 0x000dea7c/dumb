#ifndef XCALIBER_STACK_ARENA_H
#define XCALIBER_STACK_ARENA_H

#include <stdint.h>

typedef struct stack_arena stack_arena;

stack_arena *stack_arena_create (void);

void stack_arena_init (stack_arena *, void *, uint64_t);

void *stack_arena_alloc (stack_arena *, uint64_t);

void stack_arena_free (stack_arena *, void *);

void *stack_arena_resize (stack_arena *, void *, uint64_t, uint64_t);

void stack_arena_free_all (stack_arena *);

void stack_arena_destroy (stack_arena *);

#endif
