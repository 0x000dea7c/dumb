#include "hyper_renderer.h"
#include "hyper_math.h"
#include "hyper_common.h"
#include "hyper.h"
#include "hyper_stack_arena.h"

#include <assert.h>
#include <immintrin.h>

static inline i32
get_simd_width (void)
{
  /* AVX2 */
  return 8;
}

static inline void
colourise_pixel (hyper_framebuffer *framebuffer, i32 x, i32 y, u32 colour)
{
  if (!(x >= 0 && y >= 0 && x <= framebuffer->width && y <= framebuffer->height))
    {
#ifdef DEBUG
      assert (false && "framebuffer out of bounds!");
#endif
      return;
    }
  framebuffer->pixels[y * framebuffer->width + x] = colour;
}

static inline void
colourise_pixels_aligned_simd (u32 *row, u32 colour, i32 chunks, i32 simd_width)
{
  __m256i const colour_i = _mm256_set1_epi32 ((i32) colour);

  for (i32 i = 0; i < chunks; ++i)
    {
      _mm256_store_si256 ((__m256i*) row, colour_i);
      row += simd_width;
    }
}

static inline void
colourise_pixels_unaligned_simd (u32 *row, u32 colour, i32 chunks, i32 simd_width)
{
  __m256i const colour_i = _mm256_set1_epi32 ((i32) colour);

  for (i32 i = 0; i < chunks; ++i)
    {
      _mm256_storeu_si256 ((__m256i*) row, colour_i);
      row += simd_width;
    }
}

static void
circle_plot_points (hyper_framebuffer *framebuffer, i32 center_x, i32 center_y, hyper_vec2i p, u32 colour)
{
  /* each point I compute gives me 8 points on the circle (symmetry) */

  /* octant 1 */
  colourise_pixel (framebuffer, (center_x + p.x), (center_y + p.y), colour);
  /* octant 2 */
  colourise_pixel (framebuffer, (center_x + p.y), (center_y + p.x), colour);
  /* octant 3 */
  colourise_pixel (framebuffer, (center_x - p.y), (center_y + p.x), colour);
  /* octant 4 */
  colourise_pixel (framebuffer, (center_x - p.x), (center_y + p.y), colour);
  /* octant 5 */
  colourise_pixel (framebuffer, (center_x - p.x), (center_y - p.y), colour);
  /* octant 6 */
  colourise_pixel (framebuffer, (center_x - p.y), (center_y - p.x), colour);
  /* octant 7 */
  colourise_pixel (framebuffer, (center_x + p.y), (center_y - p.x), colour);
  /* octant 8 */
  colourise_pixel (framebuffer, (center_x + p.x), (center_y - p.y), colour);
}

static void
draw_horizontal_line_bresenham (hyper_framebuffer *framebuffer, i32 x0, i32 y0, i32 x1, i32 y1,
                                hyper_colour colour, i32 dx, i32 dy, i32 dy_abs)
{
  u32 const colour_uint = hyper_get_colour_uint (colour);

  if (x1 < x0)
    {
      HYPER_SWAP (i32, x0, x1);
      HYPER_SWAP (i32, y0, y1);
      dx = x1 - x0;
      dy = y1 - y0;
      dy_abs = HYPER_ABS (dy);
    }

  i32 D = 2 * dy - dx;
  i32 y = y0;
  i32 y_step = (dy < 0) ? -1 : 1;

  /* TODO: a good idea is to remove the branch and simdify this weasel */
  for (i32 x = x0; x <= x1; ++x)
    {
      colourise_pixel (framebuffer, x, y, colour_uint);

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
draw_vertical_line_bresenham (hyper_framebuffer *framebuffer, i32 x0, i32 y0, i32 x1, i32 y1,
                              hyper_colour colour, i32 dx, i32 dy, i32 dy_abs)
{
  u32 const colour_uint = hyper_get_colour_uint (colour);

  HYPER_SWAP (i32, x0, y0);
  HYPER_SWAP (i32, x1, y1);

  if (x1 < x0)
    {
      HYPER_SWAP (i32, x0, x1);
      HYPER_SWAP (i32, y0, y1);
    }

  dx = x1 - x0;
  dy = y1 - y0;
  dy_abs = HYPER_ABS (dy);

  i32 D = 2 * dy - dx;
  i32 y = y0;
  i32 y_step = (dy < 0) ? -1 : 1;

  /* TODO: a good idea is to remove the branch and simdify this weasel */
  for (i32 x = x0; x <= x1; ++x)
    {
      colourise_pixel (framebuffer, y, x, colour_uint);

      if (D > 0)
        {
          y += y_step;
          D += 2 * (dy_abs - dx);
          continue;
        }

      D += 2 * dy_abs;
    }
}

/* TODO: this number of arguments is pissing me off */
static void
draw_line_bresenham (hyper_framebuffer *framebuffer, i32 x0, i32 y0, i32 x1, i32 y1, hyper_colour colour)
{
  i32  const dx = x1 - x0;
  i32  const dy = y1 - y0;
  i32  const dy_abs = HYPER_ABS (dy);
  i32  const dx_abs = HYPER_ABS (dx);
  bool const steep = dy_abs > dx_abs;

  if (steep)
    {
      draw_vertical_line_bresenham (framebuffer, x0, y0, x1, y1, colour, dx, dy, dy_abs);
      return;
    }

  draw_horizontal_line_bresenham (framebuffer, x0, y0, x1, y1, colour, dx, dy, dy_abs);
}

static void
draw_circle_midpoint (hyper_framebuffer *framebuffer, i32 center_x, i32 center_y, i32 radius, hyper_colour colour)
{
  u32 const colour_uint = hyper_get_colour_uint (colour);

  /* start at the top */
  hyper_vec2i current = { .x = 0, .y = radius };

  /* decision variable */
  i32 D = 3 - (2 * radius);

  circle_plot_points (framebuffer, center_x, center_y, current, colour_uint);

  /* TODO: this branching seems off */
  while (current.y > current.x)
    {
      /* move inward or not */
      if (D > 0)
        {
          --current.y;
          D = D + 4 * (current.x - current.y) + 10;
        }
      else
        {
          D = D + 4 * current.x + 6;
        }

      ++current.x;

      circle_plot_points (framebuffer, center_x, center_y, current, colour_uint);
    }
}

i32 *
hyper_interpolate_array (hyper_stack_arena *arena, i32 y0, i32 x0, i32 y1, i32 x1, i32 points)
{
  if (y0 > y1)
    {
      HYPER_SWAP (i32, y0, y1);
      HYPER_SWAP (i32, x0, x1);
    }

  i32 const dx = x1 - x0;
  i32 const dy = y1 - y0;
  i32 *x_values = hyper_stack_arena_alloc (arena, (u32) points * sizeof (i32));

  if (dy == 0)
    {
      for (i32 i = 0; i < points; ++i)
        {
          x_values[i] = (x0 + dx * i) / points;
        }
    }
  else
    {
      for (i32 i = 0; i < points; ++i)
        {
          x_values[i] = x0 + ((dx * i) / dy);
        }
    }

  return x_values;
}

void
hyper_set_background_colour (hyper_renderer_context *context, hyper_colour colour)
{
  u32 const colour_uint = hyper_get_colour_uint (colour);
  colourise_pixels_aligned_simd (context->framebuffer->pixels, colour_uint, context->framebuffer->simd_chunks, get_simd_width ());
}

/* static i32 * */
/* array_append (stack_arena *arena, i32 *array0, i32 *array1, i32 array0_length, i32 array1_length) */
/* { */
/*   i32 const n = array0_length + array1_length; */
/*   i32 j = 0; */

/*   array0 = stack_arena_resize (arena, */
/*                                array0, */
/*                                (uint64_t) array0_length * sizeof (i32), */
/*                                (uint64_t) (n) * sizeof (i32)); */
/*   assert (array0); */

/*   for (i32 i = array0_length; i < n; ++i) */
/*     { */
/*       array0[i] = array1[j++]; */
/*     } */

/*   return array0; */
/* } */

/* static void */
/* draw_triangle_filled (stack_arena *arena, xcr_context *ctx, xc_triangle triangle, xc_colour colour) */
/* { */
/*   u32 const colour_i = xc_get_colour (colour); */
/*   i32  const simd_width = get_simd_width (); */

/*   /\* sort vertices so that the first vertex is always at the top *\/ */
/*   if (triangle.vertices[1].y < triangle.vertices[0].y) */
/*     { */
/*       XC_SWAP (hyper_vec2i, triangle.vertices[0], triangle.vertices[1]); */
/*     } */

/*   if (triangle.vertices[2].y < triangle.vertices[0].y) */
/*     { */
/*       XC_SWAP (hyper_vec2i, triangle.vertices[2], triangle.vertices[0]); */
/*     } */

/*   if (triangle.vertices[2].y < triangle.vertices[1].y) */
/*     { */
/*       XC_SWAP (hyper_vec2i, triangle.vertices[2], triangle.vertices[1]); */
/*     } */

/*   i32 const x01_length = triangle.vertices[1].y - triangle.vertices[0].y + 1; */
/*   i32 const x12_length = triangle.vertices[2].y - triangle.vertices[1].y + 1; */
/*   i32 const x02_length = triangle.vertices[2].y - triangle.vertices[0].y + 1; */

/*   i32 *x01 = xc_interpolate_array (arena, */
/*                                        triangle.vertices[0].y, */
/*                                        triangle.vertices[0].x, */
/*                                        triangle.vertices[1].y, */
/*                                        triangle.vertices[1].x, */
/*                                        x01_length); */

/*   i32 *x12 = xc_interpolate_array (arena, */
/*                                        triangle.vertices[1].y, */
/*                                        triangle.vertices[1].x, */
/*                                        triangle.vertices[2].y, */
/*                                        triangle.vertices[2].x, */
/*                                        x12_length); */

/*   i32 *x02 = xc_interpolate_array (arena, */
/*                                        triangle.vertices[0].y, */
/*                                        triangle.vertices[0].x, */
/*                                        triangle.vertices[2].y, */
/*                                        triangle.vertices[2].x, */
/*                                        x02_length); */

/*   /\* concatenate short sides *\/ */
/*   i32 *x012 = array_append (arena, x01, x12, x01_length - 1, x12_length); */

/*   /\* determine which array is the left and which one is the right *\/ */
/*   i32 const mid = (x01_length + x12_length) >> 1; */
/*   i32 *x_left, *x_right; */

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

/*   for (i32 y = triangle.vertices[0].y; y < triangle.vertices[2].y; ++y) */
/*     { */
/*       i32 const x_start = x_left[y - triangle.vertices[0].y]; */
/*       i32 const x_end = x_right[y - triangle.vertices[0].y]; */
/*       i32 const w = x_end - x_start + 1; */
/*       i32 const chunks = w / simd_width; */

/*       if (chunks > 0) */
/*         { */
/*           u32 *row = &ctx->fb->pixels[y * ctx->fb->width + x_start]; */

/*           fill_pixels_unaligned_simd (row, colour_i, chunks, simd_width); */

/*           for (i32 x = x_start + (chunks * simd_width); x <= x_end; ++x) */
/*             { */
/*               put_pixel (ctx, x, y, colour_i); */
/*             } */

/*           continue; */
/*         } */

/*       /\* if I can't use SIMD, draw them individually *\/ */
/*       for (i32 x = x_start; x <= x_end; ++x) */
/*         { */
/*           put_pixel (ctx, x, y, colour_i); */
/*         } */
/*     } */

/*   stack_arena_free_all (arena); */
/* } */

/* static void */
/* draw_coloured_pixels_interpolated_simd (u32 *row, i32 x, i32 y, xc_shaded_triangle *triangle, i32 chunks, i32 simd_width) */
/* { */
/*   /\* most of this comes from that rendering book, whatever the name is, but adapted to SIMD *\/ */

/*   /\* precompute edge vectors - these represent the triangle's sides *\/ */
/*   hyper_vec2i const v0 = hyper_vec2i_sub (triangle->vertices[1], triangle->vertices[0]); */
/*   hyper_vec2i const v1 = hyper_vec2i_sub (triangle->vertices[2], triangle->vertices[0]); */

/*   /\* create increment vector [0, 1, 2, 3, 4, 5, 6, 7] for processing 8 pixels at once *\/ */
/*   __m256i increment = _mm256_set_epi32 (7, 6, 5, 4, 3, 2, 1, 0); */

/*   /\* needed for u = 1.0 - v - w *\/ */
/*   __m256 one = _mm256_set1_ps(1.0f); */

/*   /\* convert edge vectors to float vectors for dot products *\/ */
/*   __m256 v0x_f = _mm256_set1_ps ((f32) v0.x); */
/*   __m256 v0y_f = _mm256_set1_ps ((f32) v0.y); */
/*   __m256 v1x_f = _mm256_set1_ps ((f32) v1.x); */
/*   __m256 v1y_f = _mm256_set1_ps ((f32) v1.y); */

/*   for (i32 i = 0; i < chunks; ++i) */
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
/*   f32 u, v, w; */
/*   i32 const simd_width = get_simd_width (); */

/*   if (triangle->vertices[1].y < triangle->vertices[0].y) */
/*     { */
/*       XC_SWAP (hyper_vec2i, triangle->vertices[0], triangle->vertices[1]); */
/*       XC_SWAP (xc_colour, triangle->colours[0], triangle->colours[1]); */
/*     } */

/*   if (triangle->vertices[2].y < triangle->vertices[0].y) */
/*     { */
/*       XC_SWAP (hyper_vec2i, triangle->vertices[2], triangle->vertices[0]); */
/*       XC_SWAP (xc_colour, triangle->colours[2], triangle->colours[0]); */
/*     } */

/*   if (triangle->vertices[2].y < triangle->vertices[1].y) */
/*     { */
/*       XC_SWAP (hyper_vec2i, triangle->vertices[2], triangle->vertices[1]); */
/*       XC_SWAP (xc_colour, triangle->colours[2], triangle->colours[1]); */
/*     } */

/*   i32 const x01_length = triangle->vertices[1].y - triangle->vertices[0].y + 1; */
/*   i32 const x12_length = triangle->vertices[2].y - triangle->vertices[1].y + 1; */
/*   i32 const x02_length = triangle->vertices[2].y - triangle->vertices[0].y + 1; */

/*   i32 *x01  = xc_interpolate_array (arena, */
/*                                         triangle->vertices[0].y, */
/*                                         triangle->vertices[0].x, */
/*                                         triangle->vertices[1].y, */
/*                                         triangle->vertices[1].x, */
/*                                         x01_length); */

/*   i32 *x12  = xc_interpolate_array (arena, */
/*                                         triangle->vertices[1].y, */
/*                                         triangle->vertices[1].x, */
/*                                         triangle->vertices[2].y, */
/*                                         triangle->vertices[2].x, */
/*                                         x12_length); */

/*   i32 *x02  = xc_interpolate_array (arena, */
/*                                         triangle->vertices[0].y, */
/*                                         triangle->vertices[0].x, */
/*                                         triangle->vertices[2].y, */
/*                                         triangle->vertices[2].x, */
/*                                         x02_length); */

/*   i32 *x012 = array_append (arena, x01, x12, x01_length - 1, x12_length); */
/*   i32 const mid = (x01_length + x12_length) >> 1; */
/*   i32 *x_left, *x_right; */

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

/*   for (i32 y = triangle->vertices[0].y; y < triangle->vertices[2].y; ++y) */
/*     { */
/*       i32 const x_start = x_left[y - triangle->vertices[0].y]; */
/*       i32 const x_end = x_right[y - triangle->vertices[0].y]; */
/*       i32 const width = x_end - x_start + 1; */
/*       i32 const chunks = width / simd_width; */

/*       if (chunks > 0) */
/*         { */
/*           u32 *row = &ctx->fb->pixels[y * ctx->fb->width + x_start]; */

/*           draw_coloured_pixels_interpolated_simd (row, x_start, y, triangle, chunks, simd_width); */

/*           for (i32 x = x_start + (chunks * simd_width); x <= x_end; ++x) */
/*             { */
/*               xc_barycentric ((hyper_vec2i){ x, y }, triangle->vertices, &u, &v, &w); */

/*               uint8_t  const r = (uint8_t) (triangle->colours[0].r * u + triangle->colours[1].r * v + triangle->colours[2].r * w); */
/*               uint8_t  const g = (uint8_t) (triangle->colours[0].g * u + triangle->colours[1].g * v + triangle->colours[2].g * w); */
/*               uint8_t  const b = (uint8_t) (triangle->colours[0].b * u + triangle->colours[1].b * v + triangle->colours[2].b * w); */
/*               uint8_t  const a = (uint8_t) (triangle->colours[0].a * u + triangle->colours[1].a * v + triangle->colours[2].a * w); */
/*               u32 const colour = (u32)((r << 24) | (g << 16) | (b << 8) | a); */

/*               put_pixel (ctx, x, y, colour); */
/*             } */

/*           continue; */
/*         } */

/*       /\* very small groups of pixels that cannot be processed using SIMD *\/ */
/*       for (i32 x = x_start; x <= x_end; ++x) */
/*         { */
/*           xc_barycentric ((hyper_vec2i){ x, y }, triangle->vertices, &u, &v, &w); */

/*           uint8_t  const r = (uint8_t)(triangle->colours[0].r * u + triangle->colours[1].r * v + triangle->colours[2].r * w); */
/*           uint8_t  const g = (uint8_t)(triangle->colours[0].g * u + triangle->colours[1].g * v + triangle->colours[2].g * w); */
/*           uint8_t  const b = (uint8_t)(triangle->colours[0].b * u + triangle->colours[1].b * v + triangle->colours[2].b * w); */
/*           uint8_t  const a = (uint8_t)(triangle->colours[0].a * u + triangle->colours[1].a * v + triangle->colours[2].a * w); */
/*           u32 const colour = (u32)((r << 24) | (g << 16) | (b << 8) | a); */

/*           put_pixel (ctx, x, y, colour); */
/*         } */
/*     } */

/*   stack_arena_free_all (arena); */
/* } */
