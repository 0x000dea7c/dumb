#include "hyper_math.h"

#include <math.h>

inline f32
hyper_sqrt (f32 n)
{
  return sqrtf (n);
}

inline f32
hyper_rsqrt (f32 n)
{
  f32 const three_halfs = 1.5f;
  f32 const n_half = n * 0.5f;
  f32 rsqrt = 1.0f / hyper_sqrt (n);

  /* newton-raphson */
  rsqrt = rsqrt * (three_halfs - (n_half * rsqrt * rsqrt));

  return rsqrt;
}

inline hyper_vec2i
hyper_vec2i_sub (hyper_vec2i a, hyper_vec2i b)
{
  return (hyper_vec2i) { .x = a.x - b.x, .y = a.y - b.y };
}

inline hyper_vec2i
hyper_vec2_add (hyper_vec2i a, hyper_vec2i b)
{
  return (hyper_vec2i) { .x = a.x + b.x, .y = a.y + b.y };
}

inline i32
hyper_vec2i_cross_product (hyper_vec2i a, hyper_vec2i b)
{
  return a.x * b.y - a.y * b.x;
}

inline i32
hyper_vec2i_dot_product (hyper_vec2i a, hyper_vec2i b)
{
  return a.x * b.x + a.y * b.y;
}

void
hyper_barycentric (hyper_vec2i point, hyper_vec2i triangle[3], f32 *u, f32 *v, f32 *w)
{
  hyper_vec2i const v0 = hyper_vec2i_sub (triangle[1], triangle[0]);
  hyper_vec2i const v1 = hyper_vec2i_sub (triangle[2], triangle[0]);
  hyper_vec2i const v2 = hyper_vec2i_sub (point, triangle[0]);

  int64_t const d00 = hyper_vec2i_dot_product (v0, v0);
  int64_t const d01 = hyper_vec2i_dot_product (v0, v1);
  int64_t const d11 = hyper_vec2i_dot_product (v1, v1);
  int64_t const d20 = hyper_vec2i_dot_product (v2, v0);
  int64_t const d21 = hyper_vec2i_dot_product (v2, v1);
  int64_t const den = d00 * d11 - d01 * d01;

  *u = 1.0f - *v - *w;
  *w = (f32) (d00 * d21 - d01 * d20) / (f32) den;
  *v = (f32) (d11 * d20 - d01 * d21) / (f32) den;

  f32 total = *u + *w + *v;
  *u /= total;
  *w /= total;
  *v /= total;
}

inline f32
hyper_sin (f32 radians)
{
  return sinf (radians);
}

inline f32
hyper_cos (f32 radians)
{
  return cosf (radians);
}
