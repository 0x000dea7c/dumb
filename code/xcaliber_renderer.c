#include "xcaliber_renderer.h"
#include "xcaliber_math.h"
#include "xcaliber_common.h"
#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include "xcaliber_stack_arena.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <immintrin.h>

typedef uint8_t pixel_mask;

static inline int32_t
get_simd_width (void)
{
  return 8;
}

struct xcr_context
{
  xc_framebuffer *fb;
};

static inline void
put_pixel (xcr_context *ctx, int32_t x, int32_t y, uint32_t colour)
{
  if (!(x >= 0 && y >= 0 && x <= ctx->fb->width && y <= ctx->fb->height))
    {
      return;
#ifdef DEBUG
      assert (false && "framebuffer out of bounds!");
#endif
    }
  ctx->fb->pixels[y * ctx->fb->width + x] = colour;
}

static inline void
fill_pixels_aligned_simd (uint32_t *row, uint32_t colour, int32_t chunks, int32_t simd_width)
{
  __m256i colour_i = _mm256_set1_epi32 ((int32_t) colour);

  for (int32_t i = 0; i < chunks; ++i)
    {
      _mm256_store_si256 ((__m256i*) row, colour_i);
      row += simd_width;
    }
}

static inline void
fill_pixels_unaligned_simd (uint32_t *row, uint32_t colour, int32_t chunks, int32_t simd_width)
{
  __m256i colour_i = _mm256_set1_epi32 ((int32_t)colour);

  for (int32_t i = 0; i < chunks; ++i)
    {
      _mm256_storeu_si256 ((__m256i*) row, colour_i);
      row += simd_width;
    }
}

static inline void
plot_points (xcr_context *ctx, xc_vec2i circle_center, xc_vec2i p, uint32_t colour)
{
  /* each point I compute gives me 8 points on the circle (symmetry)
     octant 1 */
  put_pixel(ctx, (circle_center.x + p.x), (circle_center.y + p.y), colour);
  /* octant 2 */
  put_pixel(ctx, (circle_center.x + p.y), (circle_center.y + p.x), colour);
  /* octant 3 */
  put_pixel(ctx, (circle_center.x - p.y), (circle_center.y + p.x), colour);
  /* octant 4 */
  put_pixel(ctx, (circle_center.x - p.x), (circle_center.y + p.y), colour);

  /* octant 5 */
  put_pixel(ctx, (circle_center.x - p.x), (circle_center.y - p.y), colour);
  /* octant 6 */
  put_pixel(ctx, (circle_center.x - p.y), (circle_center.y - p.x), colour);
  /* octant 7 */
  put_pixel(ctx, (circle_center.x + p.y), (circle_center.y - p.x), colour);
  /* octant 8 */
  put_pixel(ctx, (circle_center.x + p.x), (circle_center.y - p.y), colour);
}

static void
draw_horizontal_line_bresenham (xcr_context *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                xc_colour colour, int32_t dx, int32_t dy, int32_t dy_abs)
{
  uint32_t const colour_i = xc_get_colour (colour);

  if (x1 < x0)
    {
      XC_SWAP (int32_t, x0, x1);
      XC_SWAP (int32_t, y0, y1);
      dx = x1 - x0;
      dy = y1 - y0;
      dy_abs = XC_ABS (dy);
    }

  int32_t D = 2 * dy - dx;
  int32_t y = y0;
  int32_t y_step = (dy < 0) ? -1 : 1;

  for (int32_t x = x0; x <= x1; ++x)
    {
      put_pixel (ctx, x, y, colour_i);

      if (D > 0)
        {
          y += y_step;
          D += 2 * (dy_abs - dx);
          continue;
        }

      D += 2 * dy_abs;
  }
}

static void
draw_vertical_line_bresenham (xcr_context *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                              xc_colour colour, int32_t dx, int32_t dy, int32_t dy_abs)
{
  uint32_t const colour_i = xc_get_colour (colour);

  XC_SWAP (int32_t, x0, y0);
  XC_SWAP (int32_t, x1, y1);

  if (x1 < x0)
    {
      XC_SWAP (int32_t, x0, x1);
      XC_SWAP (int32_t, y0, y1);
    }

  dx = x1 - x0;
  dy = y1 - y0;
  dy_abs = XC_ABS (dy);

  int32_t D = 2 * dy - dx;
  int32_t y = y0;
  int32_t y_step = (dy < 0) ? -1 : 1;

  for (int32_t x = x0; x <= x1; ++x)
    {
      /* SPEED: this can be optimised with SIMD and what about that branch? */
      put_pixel (ctx, y, x, colour_i);

      if (D > 0)
        {
          y += y_step;
          D += 2 * (dy_abs - dx);
          continue;
        }

      D += 2 * dy_abs;
    }
}

static void
draw_line_bresenham (xcr_context *ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, xc_colour colour)
{
  int32_t const dx = x1 - x0;
  int32_t const dy = y1 - y0;
  int32_t const dy_abs = XC_ABS (dy);
  int32_t const dx_abs = XC_ABS (dx);
  bool    const steep = dy_abs > dx_abs;

  if (steep)
    {
      draw_vertical_line_bresenham (ctx, x0, y0, x1, y1, colour, dx, dy, dy_abs);
      return;
    }

  draw_horizontal_line_bresenham (ctx, x0, y0, x1, y1, colour, dx, dy, dy_abs);
}

/* static void */
/* draw_circle_midpoint (xcr_context *ctx, xc_circle2d circle, xc_colour colour) */
/* { */
/*   uint32_t const colour_i = xc_get_colour (colour); */

/*   /\* start at the top! *\/ */
/*   xc_vec2i current = { .x = 0, .y = circle.radius }; */

/*   /\* decision variable *\/ */
/*   int32_t D = 3 - (2 * circle.radius); */

/*   plot_points (ctx, circle.center, current, colour_i); */

/*   while (current.y > current.x) */
/*     { */
/*       /\* move inward or not *\/ */
/*       if (D > 0) */
/*         { */
/*           --current.y; */
/*           D = D + 4 * (current.x - current.y) + 10; */
/*         } */
/*       else */
/*         { */
/*           D = D + 4 * current.x + 6; */
/*         } */

/*       ++current.x; */
/*       plot_points (ctx, circle.center, current, colour_i); */
/*     } */
/* } */

/* static int32_t * */
/* array_append (stack_arena *arena, int32_t *array0, int32_t *array1, int32_t array0_length, int32_t array1_length) */
/* { */
/*   int32_t const n = array0_length + array1_length; */
/*   int32_t j = 0; */

/*   array0 = stack_arena_resize (arena, */
/*                                array0, */
/*                                (uint64_t) array0_length * sizeof (int32_t), */
/*                                (uint64_t) (n) * sizeof (int32_t)); */
/*   assert (array0); */

/*   for (int32_t i = array0_length; i < n; ++i) */
/*     { */
/*       array0[i] = array1[j++]; */
/*     } */

/*   return array0; */
/* } */

/* static void */
/* draw_triangle_filled (stack_arena *arena, xcr_context *ctx, xc_triangle triangle, xc_colour colour) */
/* { */
/*   uint32_t const colour_i = xc_get_colour (colour); */
/*   int32_t  const simd_width = get_simd_width (); */

/*   /\* sort vertices so that the first vertex is always at the top *\/ */
/*   if (triangle.vertices[1].y < triangle.vertices[0].y) */
/*     { */
/*       XC_SWAP (xc_vec2i, triangle.vertices[0], triangle.vertices[1]); */
/*     } */

/*   if (triangle.vertices[2].y < triangle.vertices[0].y) */
/*     { */
/*       XC_SWAP (xc_vec2i, triangle.vertices[2], triangle.vertices[0]); */
/*     } */

/*   if (triangle.vertices[2].y < triangle.vertices[1].y) */
/*     { */
/*       XC_SWAP (xc_vec2i, triangle.vertices[2], triangle.vertices[1]); */
/*     } */

/*   int32_t const x01_length = triangle.vertices[1].y - triangle.vertices[0].y + 1; */
/*   int32_t const x12_length = triangle.vertices[2].y - triangle.vertices[1].y + 1; */
/*   int32_t const x02_length = triangle.vertices[2].y - triangle.vertices[0].y + 1; */

/*   int32_t *x01 = xc_interpolate_array (arena, */
/*                                        triangle.vertices[0].y, */
/*                                        triangle.vertices[0].x, */
/*                                        triangle.vertices[1].y, */
/*                                        triangle.vertices[1].x, */
/*                                        x01_length); */

/*   int32_t *x12 = xc_interpolate_array (arena, */
/*                                        triangle.vertices[1].y, */
/*                                        triangle.vertices[1].x, */
/*                                        triangle.vertices[2].y, */
/*                                        triangle.vertices[2].x, */
/*                                        x12_length); */

/*   int32_t *x02 = xc_interpolate_array (arena, */
/*                                        triangle.vertices[0].y, */
/*                                        triangle.vertices[0].x, */
/*                                        triangle.vertices[2].y, */
/*                                        triangle.vertices[2].x, */
/*                                        x02_length); */

/*   /\* concatenate short sides *\/ */
/*   int32_t *x012 = array_append (arena, x01, x12, x01_length - 1, x12_length); */

/*   /\* determine which array is the left and which one is the right *\/ */
/*   int32_t const mid = (x01_length + x12_length) >> 1; */
/*   int32_t *x_left, *x_right; */

/*   if (x02[mid] < x012[mid]) */
/*     { */
/*       x_left = x02; */
/*       x_right = x012; */
/*     } */
/*   else */
/*     { */
/*       x_left = x012; */
/*       x_right = x02; */
/*     } */

/*   for (int32_t y = triangle.vertices[0].y; y < triangle.vertices[2].y; ++y) */
/*     { */
/*       int32_t const x_start = x_left[y - triangle.vertices[0].y]; */
/*       int32_t const x_end = x_right[y - triangle.vertices[0].y]; */
/*       int32_t const w = x_end - x_start + 1; */
/*       int32_t const chunks = w / simd_width; */

/*       if (chunks > 0) */
/*         { */
/*           uint32_t *row = &ctx->fb->pixels[y * ctx->fb->width + x_start]; */

/*           fill_pixels_unaligned_simd (row, colour_i, chunks, simd_width); */

/*           for (int32_t x = x_start + (chunks * simd_width); x <= x_end; ++x) */
/*             { */
/*               put_pixel (ctx, x, y, colour_i); */
/*             } */

/*           continue; */
/*         } */

/*       /\* if I can't use SIMD, draw them individually *\/ */
/*       for (int32_t x = x_start; x <= x_end; ++x) */
/*         { */
/*           put_pixel (ctx, x, y, colour_i); */
/*         } */
/*     } */

/*   stack_arena_free_all (arena); */
/* } */

/* static void */
/* draw_coloured_pixels_interpolated_simd (uint32_t *row, int32_t x, int32_t y, xc_shaded_triangle *triangle, int32_t chunks, int32_t simd_width) */
/* { */
/*   /\* most of this comes from that rendering book, whatever the name is, but adapted to SIMD *\/ */

/*   /\* precompute edge vectors - these represent the triangle's sides *\/ */
/*   xc_vec2i const v0 = xc_vec2i_sub (triangle->vertices[1], triangle->vertices[0]); */
/*   xc_vec2i const v1 = xc_vec2i_sub (triangle->vertices[2], triangle->vertices[0]); */

/*   /\* create increment vector [0, 1, 2, 3, 4, 5, 6, 7] for processing 8 pixels at once *\/ */
/*   __m256i increment = _mm256_set_epi32 (7, 6, 5, 4, 3, 2, 1, 0); */

/*   /\* needed for u = 1.0 - v - w *\/ */
/*   __m256 one = _mm256_set1_ps(1.0f); */

/*   /\* convert edge vectors to float vectors for dot products *\/ */
/*   __m256 v0x_f = _mm256_set1_ps ((f32_t) v0.x); */
/*   __m256 v0y_f = _mm256_set1_ps ((f32_t) v0.y); */
/*   __m256 v1x_f = _mm256_set1_ps ((f32_t) v1.x); */
/*   __m256 v1y_f = _mm256_set1_ps ((f32_t) v1.y); */

/*   for (int32_t i = 0; i < chunks; ++i) */
/*     { */
/*       /\* setup x, y coordinates for this 8 pixel chunk *\/ */
/*       __m256i x_base   = _mm256_set1_epi32 (x + i * 8); */
/*       /\* x + 7, x + 6, x + 5 ... *\/ */
/*       __m256i x_coords = _mm256_add_epi32 (x_base, increment); */
/*       /\* broadcast y to all lanes *\/ */
/*       __m256i y_coords = _mm256_set1_epi32 (y); */

/*       /\* compute v2 (point - vertex0) for all 8 pixels *\/ */
/*       __m256i v2x = _mm256_sub_epi32 (x_coords, _mm256_set1_epi32 (triangle->vertices[0].x)); */
/*       __m256i v2y = _mm256_sub_epi32 (y_coords, _mm256_set1_epi32 (triangle->vertices[0].y)); */

/*       /\* convert v2 to floats for dot products *\/ */
/*       __m256 v2x_f = _mm256_cvtepi32_ps (v2x); */
/*       __m256 v2y_f = _mm256_cvtepi32_ps (v2y); */

/*       /\* compute all dot products needed for barycentric coordinates *\/ */

/*       /\* d00 = dot(v0, v0) *\/ */
/*       __m256 d00 = _mm256_add_ps (_mm256_mul_ps (v0x_f, v0x_f), _mm256_mul_ps (v0y_f, v0y_f)); */

/*       /\* d01 = dot(v0, v1) *\/ */
/*       __m256 d01 = _mm256_add_ps (_mm256_mul_ps (v0x_f, v1x_f), _mm256_mul_ps (v0y_f, v1y_f)); */

/*       /\* d11 = dot(v1, v1) *\/ */
/*       __m256 d11 = _mm256_add_ps (_mm256_mul_ps (v1x_f, v1x_f), _mm256_mul_ps (v1y_f, v1y_f)); */

/*       /\* d20 = dot(v2, v0) *\/ */
/*       __m256 d20 = _mm256_add_ps (_mm256_mul_ps (v2x_f, v0x_f), _mm256_mul_ps (v2y_f, v0y_f)); */

/*       /\* d21 = dot(v2, v1) *\/ */
/*       __m256 d21 = _mm256_add_ps (_mm256_mul_ps (v2x_f, v1x_f), _mm256_mul_ps (v2y_f, v1y_f)); */

/*       /\* compute denominator = d00 * d11 - d01 * d01, it's guaranteed not to be zero (degenerate triangle) *\/ */
/*       __m256 denom = _mm256_sub_ps (_mm256_mul_ps (d00, d11), _mm256_mul_ps (d01, d01)); */

/*       /\* compute barycentric weights *\/ */

/*       /\* w = (d00 * d21 - d01 * d20) / denom *\/ */
/*       __m256 w = _mm256_div_ps (_mm256_sub_ps(_mm256_mul_ps(d00, d21), _mm256_mul_ps(d01, d20)), denom); */

/*       /\* v = (d11 * d20 - d01 * d21) / denom *\/ */
/*       __m256 v = _mm256_div_ps (_mm256_sub_ps (_mm256_mul_ps(d11, d20), _mm256_mul_ps(d01, d21)), denom); */

/*       /\* u = 1 - v - w *\/ */
/*       __m256 u = _mm256_sub_ps (one, _mm256_add_ps (v, w)); */

/*       /\* colour interpolation *\/ */
/*       __m256 v0r = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[0].r)); */
/*       __m256 v1r = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[1].r)); */
/*       __m256 v2r = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[2].r)); */

/*       __m256 v0g = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[0].g)); */
/*       __m256 v1g = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[1].g)); */
/*       __m256 v2g = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[2].g)); */

/*       __m256 v0b = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[0].b)); */
/*       __m256 v1b = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[1].b)); */
/*       __m256 v2b = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[2].b)); */

/*       __m256 v0a = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[0].a)); */
/*       __m256 v1a = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[1].a)); */
/*       __m256 v2a = _mm256_cvtepi32_ps (_mm256_set1_epi32 (triangle->colours[2].a)); */

/*       /\* red = (u * v0r) + (v * v1r) +  (w * v2r) *\/ */
/*       __m256 red = _mm256_add_ps (_mm256_mul_ps (u, v0r), _mm256_add_ps (_mm256_mul_ps(v, v1r), */
/*                                                                          _mm256_mul_ps(w, v2r))); */

/*       /\* likewise for green, blue, alpha *\/ */
/*       __m256 green = _mm256_add_ps (_mm256_mul_ps(u, v0g), _mm256_add_ps(_mm256_mul_ps(v, v1g), */
/*                                                                          _mm256_mul_ps(w, v2g))); */

/*       __m256 blue = _mm256_add_ps (_mm256_mul_ps (u, v0b), _mm256_add_ps (_mm256_mul_ps(v, v1b), */
/*                                                                           _mm256_mul_ps(w, v2b))); */

/*       __m256 alpha = _mm256_add_ps (_mm256_mul_ps(u, v0a), _mm256_add_ps (_mm256_mul_ps(v, v1a), */
/*                                                                           _mm256_mul_ps(w, v2a))); */

/*       /\* convert back to integers *\/ */
/*       __m256i red_i   = _mm256_cvtps_epi32 (red); */
/*       __m256i green_i = _mm256_cvtps_epi32 (green); */
/*       __m256i blue_i  = _mm256_cvtps_epi32 (blue); */
/*       __m256i alpha_i = _mm256_cvtps_epi32 (alpha); */

/*       /\* pack colors into final RGBA format */
/*          need to shift components into position and combine them *\/ */
/*       __m256i red_shifted   = _mm256_slli_epi32 (red_i, 24); */
/*       __m256i green_shifted = _mm256_slli_epi32 (green_i, 16); */
/*       __m256i blue_shifted  = _mm256_slli_epi32 (blue_i, 8); */

/*       /\* combine all components (OR them) *\/ */
/*       __m256i pixels = _mm256_or_si256 (_mm256_or_si256 (red_shifted, green_shifted), */
/*                                         _mm256_or_si256 (blue_shifted, alpha_i)); */

/*       _mm256_storeu_si256 ((__m256i*) row, pixels); */

/*       row += simd_width; */
/*     } */
/* } */

/* static void */
/* draw_shaded_triangle_filled_simd (xcr_context *ctx, stack_arena *arena, xc_shaded_triangle *triangle) */
/* { */
/*   f32_t u, v, w; */
/*   int32_t const simd_width = get_simd_width (); */

/*   if (triangle->vertices[1].y < triangle->vertices[0].y) */
/*     { */
/*       XC_SWAP (xc_vec2i, triangle->vertices[0], triangle->vertices[1]); */
/*       XC_SWAP (xc_colour, triangle->colours[0], triangle->colours[1]); */
/*     } */

/*   if (triangle->vertices[2].y < triangle->vertices[0].y) */
/*     { */
/*       XC_SWAP (xc_vec2i, triangle->vertices[2], triangle->vertices[0]); */
/*       XC_SWAP (xc_colour, triangle->colours[2], triangle->colours[0]); */
/*     } */

/*   if (triangle->vertices[2].y < triangle->vertices[1].y) */
/*     { */
/*       XC_SWAP (xc_vec2i, triangle->vertices[2], triangle->vertices[1]); */
/*       XC_SWAP (xc_colour, triangle->colours[2], triangle->colours[1]); */
/*     } */

/*   int32_t const x01_length = triangle->vertices[1].y - triangle->vertices[0].y + 1; */
/*   int32_t const x12_length = triangle->vertices[2].y - triangle->vertices[1].y + 1; */
/*   int32_t const x02_length = triangle->vertices[2].y - triangle->vertices[0].y + 1; */

/*   int32_t *x01  = xc_interpolate_array (arena, */
/*                                         triangle->vertices[0].y, */
/*                                         triangle->vertices[0].x, */
/*                                         triangle->vertices[1].y, */
/*                                         triangle->vertices[1].x, */
/*                                         x01_length); */

/*   int32_t *x12  = xc_interpolate_array (arena, */
/*                                         triangle->vertices[1].y, */
/*                                         triangle->vertices[1].x, */
/*                                         triangle->vertices[2].y, */
/*                                         triangle->vertices[2].x, */
/*                                         x12_length); */

/*   int32_t *x02  = xc_interpolate_array (arena, */
/*                                         triangle->vertices[0].y, */
/*                                         triangle->vertices[0].x, */
/*                                         triangle->vertices[2].y, */
/*                                         triangle->vertices[2].x, */
/*                                         x02_length); */

/*   int32_t *x012 = array_append (arena, x01, x12, x01_length - 1, x12_length); */
/*   int32_t const mid = (x01_length + x12_length) >> 1; */
/*   int32_t *x_left, *x_right; */

/*   if (x02[mid] < x012[mid]) */
/*     { */
/*       x_left = x02; */
/*       x_right = x012; */
/*     } */
/*   else */
/*     { */
/*       x_left = x012; */
/*       x_right = x02; */
/*     } */

/*   for (int32_t y = triangle->vertices[0].y; y < triangle->vertices[2].y; ++y) */
/*     { */
/*       int32_t const x_start = x_left[y - triangle->vertices[0].y]; */
/*       int32_t const x_end = x_right[y - triangle->vertices[0].y]; */
/*       int32_t const width = x_end - x_start + 1; */
/*       int32_t const chunks = width / simd_width; */

/*       if (chunks > 0) */
/*         { */
/*           uint32_t *row = &ctx->fb->pixels[y * ctx->fb->width + x_start]; */

/*           draw_coloured_pixels_interpolated_simd (row, x_start, y, triangle, chunks, simd_width); */

/*           for (int32_t x = x_start + (chunks * simd_width); x <= x_end; ++x) */
/*             { */
/*               xc_barycentric ((xc_vec2i){ x, y }, triangle->vertices, &u, &v, &w); */

/*               uint8_t  const r = (uint8_t) (triangle->colours[0].r * u + triangle->colours[1].r * v + triangle->colours[2].r * w); */
/*               uint8_t  const g = (uint8_t) (triangle->colours[0].g * u + triangle->colours[1].g * v + triangle->colours[2].g * w); */
/*               uint8_t  const b = (uint8_t) (triangle->colours[0].b * u + triangle->colours[1].b * v + triangle->colours[2].b * w); */
/*               uint8_t  const a = (uint8_t) (triangle->colours[0].a * u + triangle->colours[1].a * v + triangle->colours[2].a * w); */
/*               uint32_t const colour = (uint32_t)((r << 24) | (g << 16) | (b << 8) | a); */

/*               put_pixel (ctx, x, y, colour); */
/*             } */

/*           continue; */
/*         } */

/*       /\* very small groups of pixels that cannot be processed using SIMD *\/ */
/*       for (int32_t x = x_start; x <= x_end; ++x) */
/*         { */
/*           xc_barycentric ((xc_vec2i){ x, y }, triangle->vertices, &u, &v, &w); */

/*           uint8_t  const r = (uint8_t)(triangle->colours[0].r * u + triangle->colours[1].r * v + triangle->colours[2].r * w); */
/*           uint8_t  const g = (uint8_t)(triangle->colours[0].g * u + triangle->colours[1].g * v + triangle->colours[2].g * w); */
/*           uint8_t  const b = (uint8_t)(triangle->colours[0].b * u + triangle->colours[1].b * v + triangle->colours[2].b * w); */
/*           uint8_t  const a = (uint8_t)(triangle->colours[0].a * u + triangle->colours[1].a * v + triangle->colours[2].a * w); */
/*           uint32_t const colour = (uint32_t)((r << 24) | (g << 16) | (b << 8) | a); */

/*           put_pixel (ctx, x, y, colour); */
/*         } */
/*     } */

/*   stack_arena_free_all (arena); */
/* } */

xcr_context *
xcr_create (linear_arena *arena, xc_framebuffer *fb)
{
  xcr_context *ctx = linear_arena_alloc (arena, sizeof (xcr_context));
  if (!ctx)
    {
      (void) fprintf (stderr, "Couldn't allocate space for renderer\n");
      return NULL;
    }

  ctx->fb = fb;

  return ctx;
}

void
xcr_set_background_colour (xcr_context *ctx, xc_colour colour)
{
  uint32_t colour_i = xc_get_colour (colour);
  uint32_t *row = ctx->fb->pixels;
  fill_pixels_aligned_simd (row, colour_i, ctx->fb->simd_chunks, get_simd_width ());
}

void
xcr_draw (xcr_context *ctx, stack_arena *arena, xc_draw_command *command)
{
  /* TODO: before rasterisation, perform conversion from floating point to integers */

  /* TODO: this is a switch for now, but maybe I can use a function pointer instead? */
  switch (command->type)
    {
    case DRAW_TYPE_TRIANGLE:
      break;
    case DRAW_TYPE_LINE: {
      /* FIXME: prototype */
      int32_t x0 = (int32_t) (command->geometry.line.start.position.x);
      int32_t y0 = (int32_t) (command->geometry.line.start.position.y);
      int32_t x1 = (int32_t) (command->geometry.line.end.position.x);
      int32_t y1 = (int32_t) (command->geometry.line.end.position.y);
      draw_line_bresenham (ctx, x0, y0, x1, y1, command->base_colour);
    } break;
    case DRAW_TYPE_QUAD:
      break;
    case DRAW_TYPE_CIRCLE:
      break;
    default:
      break;
    }
}

/* void */
/* xcr_draw_quad_outline (xcr_context *ctx, xc_quad quad, xc_colour colour) */
/* { */
/*   xc_vec2i const p1 = { .x = quad.position.x,              .y = quad.position.y + quad.height }; */
/*   xc_vec2i const p2 = { .x = quad.position.x + quad.width, .y = quad.position.y }; */
/*   xc_vec2i const p3 = { .x = quad.position.x + quad.width, .y = quad.position.y + quad.height }; */

/*   xc_line const line02 = { quad.position, p2 }; */
/*   xc_line const line01 = { quad.position, p1 }; */
/*   xc_line const line13 = { p1, p3 }; */
/*   xc_line const line32 = { p3, p2 }; */

/*   xcr_draw_line (ctx, line02, colour); */
/*   xcr_draw_line (ctx, line01, colour); */
/*   xcr_draw_line (ctx, line13, colour); */
/*   xcr_draw_line (ctx, line32, colour); */
/* } */

/* void */
/* xcr_draw_triangle_outline (xcr_context *ctx, xc_triangle triangle, xc_colour colour) */
/* { */
/*   xc_line const line01 = { triangle.vertices[0], triangle.vertices[1] }; */
/*   xc_line const line12 = { triangle.vertices[1], triangle.vertices[2] }; */
/*   xc_line const line20 = { triangle.vertices[2], triangle.vertices[1] }; */

/*   xcr_draw_line (ctx, line01, colour); */
/*   xcr_draw_line (ctx, line12, colour); */
/*   xcr_draw_line (ctx, line20, colour); */
/* } */

/* void */
/* xcr_draw_circle_outline (xcr_context *ctx, xc_circle circle, xc_colour colour) */
/* { */
/*   draw_circle_midpoint (ctx, circle, colour); */
/* } */

/* void */
/* xcr_draw_quad_filled (xcr_context *ctx, xc_quad quad, xc_colour colour) */
/* { */
/*   uint32_t const colour_i = xc_get_colour (colour); */
/*   int32_t  const x_start = XC_MAX (quad.position.x, 0); */
/*   int32_t  const y_start = XC_MAX (quad.position.y, 0); */

/*   /\* bounds checking *\/ */
/*   int32_t const y_max = XC_MIN (quad.position.y + quad.height, ctx->fb->height - 1); */
/*   int32_t const x_max = XC_MIN (quad.position.x + quad.width,  ctx->fb->width - 1); */

/*   for (int32_t y = y_start; y <= y_max; ++y) */
/*     { */
/*       /\* SPEED: this can be optimised with SIMD *\/ */
/*       for (int32_t x = x_start; x <= x_max; ++x) */
/*         { */
/*           put_pixel (ctx, x, y, colour_i); */
/*         } */
/*     } */
/* } */

/* void */
/* xcr_draw_triangle_filled (xcr_context *ctx, stack_arena *arena, xc_triangle triangle, xc_colour colour) */
/* { */
/*   draw_triangle_filled (arena, ctx, triangle, colour); */
/* } */

/* void */
/* xcr_draw_circle_filled (xcr_context *ctx, xc_circle circle, xc_colour colour) */
/* { */
/*   uint32_t const colour_i = xc_get_colour (colour); */
/*   int32_t  const radius_squared = circle.radius * circle.radius; */
/*   int32_t  const y_start = XC_MAX (circle.center.y - circle.radius, 0); */
/*   int32_t  const y_end = XC_MIN (circle.center.y + circle.radius, ctx->fb->height - 1); */

/*   for (int32_t y = y_start; y <= y_end; ++y) */
/*     { */
/*       int32_t dy = y - circle.center.y; */
/*       int32_t width_squared = radius_squared - (dy * dy); */

/*       if (width_squared < 0) */
/*         { */
/*           continue; */
/*         } */

/*       int32_t width = (int32_t) xc_sqrt ((f32_t) width_squared); */
/*       int32_t x_start = XC_MAX (circle.center.x - width, 0); */
/*       int32_t x_end = XC_MIN (circle.center.x + width, ctx->fb->width - 1); */

/*       for (int32_t x = x_start; x <= x_end; ++x) */
/*         { */
/*           put_pixel (ctx, x, y, colour_i); */
/*         } */
/*     } */
/* } */

/* void */
/* xcr_draw_shaded_triangle_filled (xcr_context *ctx, stack_arena *arena, xc_shaded_triangle *triangle) */
/* { */
/*   draw_shaded_triangle_filled_simd (ctx, arena, triangle); */
/* } */
