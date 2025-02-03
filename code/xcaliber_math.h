#ifndef XCALIBER_MATH_H
#define XCALIBER_MATH_H

#include "xcaliber_common.h"

#include <stdint.h>

#define XC_ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define XC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define XC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define XC_MAX3(a,b,c) XC_MAX(XC_MAX(a,b),c)
#define XC_MIN3(a, b, c) XC_MIN(XC_MIN(a, b), c)
#define XC_PI 3.141592653589793238462643f
#define XC_TWO_PI XC_PI * 2.0f
#define XC_PI_OVER_2 XC_PI / 2.0f

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

typedef struct xc_mat2x2
{
  f32_t values[9];
} xc_mat3x3;

void xc_math_init (void);

f32_t xc_sin (f32_t);

f32_t xc_cos (f32_t);

f32_t xc_sqrt (f32_t);

f32_t xc_rsqrt (f32_t);

xc_vec2i xc_vec2i_sub (xc_vec2i, xc_vec2i);

xc_vec2i xc_vec2i_add (xc_vec2i, xc_vec2i);

int32_t xc_vec2i_cross_product (xc_vec2i, xc_vec2i);

int32_t *xc_interpolate_array (stack_arena *, int32_t, int32_t, int32_t, int32_t, int32_t);

int32_t xc_vec2i_dot_product (xc_vec2i, xc_vec2i);

void xc_barycentric (xc_vec2i, xc_vec2i[3], f32_t *, f32_t *, f32_t *);

#endif
