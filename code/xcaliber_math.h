#ifndef XCALIBER_MATH_H
#define XCALIBER_MATH_H

#include "xcaliber_common.h"

#include <stdint.h>

#define XC_ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define XC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define XC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define XC_MAX3(a,b,c) XC_MAX(XC_MAX(a,b),c)
#define XC_MIN3(a, b, c) XC_MIN(XC_MIN(a, b), c)

typedef struct stack_arena stack_arena;

typedef struct xc_vec2f
{
  f32_t x;
  f32_t y;
} __attribute__((aligned(8))) xc_vec2f;

typedef struct xc_vec2i
{
  int32_t x;
  int32_t y;
} __attribute__((aligned(8))) xc_vec2i;

f32_t xc_sqrt (f32_t);

f32_t xc_rsqrt (f32_t);

xc_vec2i xc_vec2i_sub (xc_vec2i, xc_vec2i);

xc_vec2i xc_vec2i_add (xc_vec2i, xc_vec2i);

int32_t xc_vec2i_cross_product (xc_vec2i, xc_vec2i);

int32_t *xc_interpolate_array (stack_arena *, int32_t, int32_t, int32_t, int32_t, int32_t);

int32_t xc_vec2i_dot_product (xc_vec2i, xc_vec2i);

void xc_barycentric (xc_vec2i, xc_vec2i[3], f32_t *, f32_t *, f32_t *);

#endif
