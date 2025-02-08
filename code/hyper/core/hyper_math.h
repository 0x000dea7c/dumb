#pragma once

#include "hyper_common.h"

#define HYPER_ABS(a) ((a) >= 0 ? (a) : (-(a)))
#define HYPER_MIN(a, b) ((a) < (b) ? (a) : (b))
#define HYPER_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HYPER_MAX3(a,b,c) HYPER_MAX(HYPER_MAX(a,b),c)
#define HYPER_MIN3(a, b, c) HYPER_MIN(HYPER_MIN(a, b), c)
#define HYPER_PI 3.1415926535f
#define HYPER_TWO_PI HYPER_PI * 2.0f
#define HYPER_PI_OVER_2 HYPER_PI / 2.0f

typedef struct
{
  f32 x;
  f32 y;
} hyper_vec2f;

typedef struct
{
  f32 x;
  f32 y;
  f32 z;
} hyper_vec3f;

typedef struct
{
  i32 x;
  i32 y;
  i32 z;
} hyper_vec3i;

typedef struct
{
  f32 x;
  f32 y;
  f32 z;
  f32 w;
} hyper_vec4f;

typedef struct
{
  i32 x;
  i32 y;
} hyper_vec2i;

typedef struct
{
  f32 values[9];
} hyper_mat3x3;

typedef struct
{
  f32 values[16];
} hyper_mat4x4;

/* math functions */

f32 hyper_sin (f32);

f32 hyper_cos (f32);

f32 hyper_sqrt (f32);

f32 hyper_rsqrt (f32);

/* vector operations */

hyper_vec2i hyper_vec2i_sub (hyper_vec2i, hyper_vec2i);

hyper_vec2i hyper_vec2i_add (hyper_vec2i, hyper_vec2i);

/* products */

i32 hyper_vec2i_cross_product (hyper_vec2i, hyper_vec2i);

i32 hyper_vec2i_dot_product (hyper_vec2i, hyper_vec2i);

hyper_vec4f hyper_mat4x4_vec4f_mul (hyper_mat4x4 *, hyper_vec4f *);

/* misc */

void hyper_barycentric (hyper_vec2i, hyper_vec2i[3], f32 *, f32 *, f32 *);
