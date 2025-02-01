#include "xcaliber_renderer.h"
#include "xcaliber_math.h"
#include "xcaliber_common.h"
#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include "xcaliber_stack_arena.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

typedef uint8_t pixel_mask;

static inline int32_t
get_simd_width(void)
{
#if defined(__AVX2__)
  return 8;
#elif defined(__SSE4_2__)
  return 4;
#else
#error "engine needs at least SSE4.2"
#endif
}

#if defined(__AVX2__)
#define XC_FILL_PIXELS_SIMD_ALIGNED(dst, colour, count)                 \
  __asm__ volatile(                                                     \
                    /* Load chunk count into rax */                     \
                    "movl %2, %%eax\n\t"                                \
                    /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
                    "vpbroadcastd %0, %%ymm0\n\t"                       \
                    /* Load destination buffer address into rcx */      \
                    "movq %1, %%rcx\n\t"                                \
                    /* Main loop label */                               \
                    "1:\n\t"                                            \
                    /* Store 8 pixels (256 bits) */                     \
                    "vmovdqa %%ymm0, (%%rcx)\n\t"                       \
                    /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
                    "addq $32, %%rcx\n\t"                               \
                    /* Decrement counter */                             \
                    "dec %%eax\n\t"                                     \
                    /* Loop if counter not zero */                      \
                    "jnz 1b\n\t"                                        \
                    : /* No outputs */                                  \
                    : "m"(colour), /* %0: colour to broadcast */        \
                      "r"(dst),    /* %1: destination buffer */         \
                      "r"(count)   /* %2: number of chunks */           \
                    : "ymm0", "eax", "rcx", "memory"                    \
                    )

#define XC_FILL_PIXELS_SIMD_UNALIGNED(dst, colour, count)               \
  __asm__ volatile(                                                     \
                    /* Load chunk count into rax */                     \
                    "movl %2, %%eax\n\t"                                \
                    /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
                    "vpbroadcastd %0, %%ymm0\n\t"                       \
                    /* Load destination buffer address into rcx */      \
                    "movq %1, %%rcx\n\t"                                \
                    /* Main loop label */                               \
                    "1:\n\t"                                            \
                    /* Store 8 pixels (256 bits) */                     \
                    "vmovdqu %%ymm0, (%%rcx)\n\t"                       \
                    /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
                    "addq $32, %%rcx\n\t"                               \
                    /* Decrement counter */                             \
                    "dec %%eax\n\t"                                     \
                    /* Loop if counter not zero */                      \
                    "jnz 1b\n\t"                                        \
                    : /* No outputs */                                  \
                    : "m"(colour), /* %0: colour to broadcast */	\
                      "r"(dst),    /* %1: destination buffer */         \
                      "r"(count)   /* %2: number of chunks */           \
                    : "ymm0", "eax", "rcx", "memory"                    \
                    )

#elif defined(__SSE4_2__)
#define XC_FILL_PIXELS_SIMD_ALIGNED(dst, colour, count, aligned)        \
  __asm__ volatile(                                                     \
                    /* Load chunk count into rax */                     \
                    "movl %2, %%eax\n\t"                                \
                    /* Move 32-bit colour into lowest lane of xmm0 */   \
                    "movd %0, %%xmm0\n\t"                               \
                    /* Broadcast to all 4 lanes */                      \
                    "pshufd $0, %%xmm0, %%xmm0\n\t"                     \
                    /* Load destination buffer address */               \
                    "movq %1, %%rcx\n\t"                                \
                    /* Main loop label */                               \
                    "1:\n\t"                                            \
                    /* Store 4 pixels (128 bits) aligned */             \
                    "movdqa %%xmm0, (%%rcx)\n\t"                        \
                    /* Move to next chunk (16 bytes = 4 pixels × 4 bytes) */ \
                    "addq $16, %%rcx\n\t"                               \
                    /* Decrement counter */                             \
                    "dec %%eax\n\t"                                     \
                    /* Loop if counter not zero */                      \
                    "jnz 1b\n\t"                                        \
                    : /* No outputs */                                  \
                    : "m"(colour), /* %0: colour to broadcast */        \
                      "r"(dst),    /* %1: destination buffer */         \
                      "r"(count)   /* %2: number of chunks */           \
                    : "xmm0", "eax", "rcx", "memory"                    \
                    )

#define XC_FILL_PIXELS_SIMD_UNALIGNED(dst, colour, count)               \
  __asm__ volatile(                                                     \
                    /* Load chunk count into rax */                     \
                    "movl %2, %%eax\n\t"                                \
                    /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
                    "vpbroadcastd %0, %%ymm0\n\t"                       \
                    /* Load destination buffer address into rcx */      \
                    "movq %1, %%rcx\n\t"                                \
                    /* Main loop label */                               \
                    "1:\n\t"                                            \
                    /* Store 8 pixels (256 bits) */                     \
                    "movdqu %%xmm0, (%%rcx)\n\t"                        \
                    /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
                    "addq $32, %%rcx\n\t"                               \
                    /* Decrement counter */                             \
                    "dec %%eax\n\t"                                     \
                    /* Loop if counter not zero */                      \
                    "jnz 1b\n\t"                                        \
                    : /* No outputs */                                  \
                    : "m"(colour), /* %0: colour to broadcast */        \
                      "r"(dst),    /* %1: destination buffer */         \
                      "r"(count)   /* %2: number of chunks */           \
                    : "ymm0", "eax", "rcx", "memory"                    \
                    )

#else
#error "engine needs at least SSE4.2 support"
#endif

struct xcr_context {
  xc_framebuffer *fb;
};

static inline void
put_pixel(xcr_context *ctx, int32_t x, int32_t y, uint32_t colour)
{
#ifdef DEBUG
  if (x < 0 || x > ctx->fb->width || y < 0 || y > ctx->fb->height) {
    (void)fprintf(
                  stderr,
                  "WARNING: OUT OF BOUNDS FRAMEBUFFER UPDATE x: %d, y: %d\n",
                  x, y);
    return;
  }
#endif
  ctx->fb->pixels[y * ctx->fb->width + x] = colour;
}

static inline void
plot_points(xcr_context *ctx, xc_vec2i center, xc_vec2i p, uint32_t c)
{
  /* each point I compute gives me 8 points on the circle (symmetry) */

  /* octant 1 */
  put_pixel(ctx, (center.x + p.x), (center.y + p.y), c);
  /* octant 2 */
  put_pixel(ctx, (center.x + p.y), (center.y + p.x), c);
  /* octant 3 */
  put_pixel(ctx, (center.x - p.y), (center.y + p.x), c);
  /* octant 4 */
  put_pixel(ctx, (center.x - p.x), (center.y + p.y), c);

  /* octant 5 */
  put_pixel(ctx, (center.x - p.x), (center.y - p.y), c);
  /* octant 6 */
  put_pixel(ctx, (center.x - p.y), (center.y - p.x), c);
  /* octant 7 */
  put_pixel(ctx, (center.x + p.y), (center.y - p.x), c);
  /* octant 8 */
  put_pixel(ctx, (center.x + p.x), (center.y - p.y), c);
}

static void
draw_horizontal_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1,
			       uint32_t colour, int32_t dx, int32_t dy,
			       int32_t dy_abs)
{
  if (p1.x < p0.x)
    {
      XC_SWAP(int, p0.x, p1.x);
      XC_SWAP(int, p0.y, p1.y);
      dx = p1.x - p0.x;
      dy = p1.y - p0.y;
      dy_abs = XC_ABS(dy);
    }

  int32_t D = 2 * dy - dx;
  int32_t y = p0.y;
  int32_t y_step = (dy < 0) ? -1 : 1;

  for (int32_t x = p0.x; x <= p1.x; ++x)
    {
      put_pixel(ctx, x, y, colour);

      if (D > 0) {
        y += y_step;
        D += 2 * (dy_abs - dx);
        continue;
      }

      D += 2 * dy_abs;
  }
}

static void
draw_vertical_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1,
			     uint32_t colour, int32_t dx, int32_t dy,
			     int32_t dy_abs)
{
  XC_SWAP(int, p0.x, p0.y);
  XC_SWAP(int, p1.x, p1.y);

  if (p1.x < p0.x) {
    XC_SWAP(int, p0.x, p1.x);
    XC_SWAP(int, p0.y, p1.y);
  }

  dx = p1.x - p0.x;
  dy = p1.y - p0.y;
  dy_abs = XC_ABS(dy);

  int32_t D = 2 * dy - dx;
  int32_t y = p0.y;
  int32_t y_step = (dy < 0) ? -1 : 1;

  for (int32_t x = p0.x; x <= p1.x; ++x) {
    put_pixel(ctx, y, x, colour);

    if (D > 0) {
      y += y_step;
      D += 2 * (dy_abs - dx);
    } else {
      D += 2 * dy_abs;
    }
  }
}

static void
draw_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1, uint32_t colour)
{
  int32_t dx = p1.x - p0.x;
  int32_t dy = p1.y - p0.y;
  int32_t dy_abs = XC_ABS(dy);
  int32_t dx_abs = XC_ABS(dx);
  bool steep = dy_abs > dx_abs;

  if (steep) {
    draw_vertical_line_bresenham(ctx, p0, p1, colour, dx, dy, dy_abs);
  } else {
    draw_horizontal_line_bresenham(ctx, p0, p1, colour, dx, dy, dy_abs);
  }
}

static void
draw_circle_midpoint(xcr_context *ctx, xc_vec2i center, int32_t r,
		     uint32_t colour)
{
  /* start at the top! */
  xc_vec2i curr = { .x = 0, .y = r };
  int32_t D = 3 - (2 * r);

  plot_points(ctx, center, curr, colour);

  while (curr.y > curr.x) {
    /* move inward or not */
    if (D > 0) {
      --curr.y;
      D = D + 4 * (curr.x - curr.y) + 10;
    } else {
      D = D + 4 * curr.x + 6;
    }
    ++curr.x;
    plot_points(ctx, center, curr, colour);
  }
}

/* puts everything into arr0 and frees arr1 */
static int32_t *
array_append(stack_arena *a, int32_t *arr0, int32_t *arr1, int32_t arr0_size, int32_t arr1_size)
{
  arr0 = stack_arena_resize(a, arr0, (uint64_t)arr0_size * sizeof(int32_t), (uint64_t)(arr0_size + arr1_size) * sizeof(int32_t));
  assert(arr0);

  int32_t j = 0;
  int32_t const n = arr0_size + arr1_size;

  for (int32_t i = arr0_size; i < n; ++i) {
    arr0[i] = arr1[j++];
  }

  /* don't need arr1 anymore */
  stack_arena_free(a, arr1);

  return arr0;
}

static void
draw_triangle_filled(stack_arena *a, xcr_context *ctx, xcr_triangle T, uint32_t colour)
{
  /* sort vertices so that the first vertex is always at the top */
  if (T.vertices[1].y < T.vertices[0].y) {
    XC_SWAP(xc_vec2i, T.vertices[0], T.vertices[1]);
  }

  if (T.vertices[2].y < T.vertices[0].y) {
    XC_SWAP(xc_vec2i, T.vertices[2], T.vertices[0]);
  }

  if (T.vertices[2].y < T.vertices[1].y) {
    XC_SWAP(xc_vec2i, T.vertices[2], T.vertices[1]);
  }

  int32_t const x01_size = T.vertices[1].y - T.vertices[0].y + 1;
  int32_t const x12_size = T.vertices[2].y - T.vertices[1].y + 1;
  int32_t const x02_size = T.vertices[2].y - T.vertices[0].y + 1;

  int32_t *x01 = xc_interpolate_array(a, T.vertices[0].y, T.vertices[0].x, T.vertices[1].y, T.vertices[1].x, x01_size);

  int32_t *x12 = xc_interpolate_array(a, T.vertices[1].y, T.vertices[1].x, T.vertices[2].y, T.vertices[2].x, x12_size);

  int32_t *x02 = xc_interpolate_array(a, T.vertices[0].y, T.vertices[0].x, T.vertices[2].y, T.vertices[2].x, x02_size);

  /* concatenate short sides */
  int32_t *x012 = array_append(a, x01, x12, x01_size - 1, x12_size);

  /* determine which array is the left and which one is the right */
  int32_t *x_left, *x_right;
  int32_t const mid = (x01_size + x12_size) >> 1;
  int32_t const simd_width = get_simd_width();

  if (x02[mid] < x012[mid]) {
    x_left = x02;
    x_right = x012;
  } else {
    x_left = x012;
    x_right = x02;
  }

  /* draw horizontally */
  for (int32_t y = T.vertices[0].y; y < T.vertices[2].y; ++y) {
    int32_t const x_start = x_left[y - T.vertices[0].y];
    int32_t const x_end = x_right[y - T.vertices[0].y];
    int32_t const w = x_end - x_start + 1;
    int32_t const chunks = w / simd_width;

    if (chunks > 0) {
      uint32_t *row = &ctx->fb->pixels[y * ctx->fb->width + x_start];

      XC_FILL_PIXELS_SIMD_UNALIGNED(row, colour, chunks);

      for (int32_t x = x_start + (chunks * simd_width); x <= x_end; ++x) {
        put_pixel(ctx, x, y, colour);
      }

      continue;
    }

    for (int32_t x = x_start; x <= x_end; ++x) {
      put_pixel(ctx, x, y, colour);
    }
  }
}

static void
draw_triangle_filled_colours(xcr_context *ctx, xcr_triangle_colours *T)
{
  /* sort triangles so that v0 is on top and v0 < v1 < v2 */
  if (T->vertices[1].y < T->vertices[0].y) {
    XC_SWAP(xc_vec2i, T->vertices[0], T->vertices[1]);
    XC_SWAP(xc_colour, T->colours[0], T->colours[1]);
  }

  if (T->vertices[2].y < T->vertices[0].y) {
    XC_SWAP(xc_vec2i, T->vertices[2], T->vertices[0]);
    XC_SWAP(xc_colour, T->colours[2], T->colours[0]);
  }

  if (T->vertices[2].y < T->vertices[1].y) {
    XC_SWAP(xc_vec2i, T->vertices[2], T->vertices[1]);
    XC_SWAP(xc_colour, T->colours[2], T->colours[1]);
  }

  /* grab triangle bounding box, doing bounds checking too */
  int32_t const xmin = XC_MAX(XC_MIN3(T->vertices[0].x, T->vertices[1].x, T->vertices[2].x), 0);
  int32_t const ymin = XC_MAX(XC_MIN3(T->vertices[0].y, T->vertices[1].y, T->vertices[2].y), 0);
  int32_t const xmax = XC_MIN(XC_MAX3(T->vertices[0].x, T->vertices[1].x, T->vertices[2].x), ctx->fb->width - 1);
  int32_t const ymax = XC_MIN(XC_MAX3(T->vertices[0].y, T->vertices[1].y, T->vertices[2].y), ctx->fb->height - 1);

  f32_t u, v, w;

  for (int32_t y = ymin; y <= ymax; ++y) {
    for (int32_t x = xmin; x <= xmax; ++x) {
      xc_barycentric((xc_vec2i){ x, y }, T->vertices, &u, &v, &w);

      /* point outside the triangle, skip */
      if (u > 1.0f || v > 1.0f || w > 1.0f || u < 0.0f || v < 0.0f || w < 0.0f)
        {
          continue;
        }

      uint8_t const r = (uint8_t)(T->colours[0].r * u + T->colours[1].r * v + T->colours[2].r * w);
      uint8_t const g = (uint8_t)(T->colours[0].g * u + T->colours[1].g * v + T->colours[2].g * w);
      uint8_t const b = (uint8_t)(T->colours[0].b * u + T->colours[1].b * v + T->colours[2].b * w);
      uint8_t const a = (uint8_t)(T->colours[0].a * u + T->colours[1].a * v + T->colours[2].a * w);

      uint32_t const c = (uint32_t)((r << 24) | (g << 16) | (b << 8) | a);

      put_pixel(ctx, x, y, c);
    }
  }
}

static void
draw_coloured_pixels_interpolated_simd(uint32_t *row, int32_t x_start, int32_t y, xcr_triangle_colours *T, int32_t chunks)
{
  /* PRECONDITION: don't use degenerate triangles (at least one side length or angle is 0) for God's sake */
  static f32_t const one = 1.0f;

  __asm__ volatile(
                   /* outer loop, for each chunk */
                   "mov %[chunks], %%ecx\n\t"
                   "1:\n\t"

                   /* broadcast pixels, x component */
                   "vmovd %[x], %%xmm0\n\t"
                   "vpbroadcastd %%xmm0, %%ymm0\n\t"
                   /* all lanes of ymm4 to 1's, this can be improved */
                   "vpcmpeqd %%ymm4, %%ymm4, %%ymm4\n\t"
                   "vpsrld $31, %%ymm4, %%ymm4\n\t"
                   "vpsllvd %%ymm4, %%ymm4, %%ymm4\n\t"
                   /* add to x */
                   "vpaddd %%ymm4, %%ymm0, %%ymm0\n\t"

                   /* broadcast y coordinate */
                   "vmovd %[y], %%xmm1\n\t"
                   "vpbroadcastd %%xmm1, %%ymm1\n\t"

                   /* compute v2 for every lane = (x, y) - T[0] */
                   "vpsubd %[v0x], %%ymm0, %%ymm2\n\t"
                   "vpsubd %[v0y], %%ymm1, %%ymm3\n\t"

                   /* for dpps!! */
                   /* 1111: keep the result in all 4 positions of each lane */
                   /* 0001: multiply and add just the x and y components */

                   /* compute d00: dot product (v0, v0) */
                   "vbroadcastss %[v0x], %%ymm4\n\t"
                   "vbroadcastss %[v0y], %%ymm5\n\t"
                   "dpps $0b11110001, %%ymm4, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm9\n\t"

                   /* compute d01: dot product (v0, v1) */
                   "vbroadcastss %[v1x], %%ymm4\n\t"
                   "vbroadcastss %[v1y], %%ymm5\n\t"
                   "dpps $0b11110001, %%ymm4, %%ymm5, %%ymm5\n\t"
                   "vmovaps %%ymm5, %%ymm10\n\t"

                   /* compute d11: dot product (v1, v1) */
                   "dpps $0b11110001, %%ymm4, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm11\n\t"

                   /* compute d20: dot product (v2, v0) */
                   "vbroadcastss %[v0x], %%ymm4\n\t"
                   "vbroadcastss %[v0y], %%ymm5\n\t"
                   "dpps $0b11110001, %%ymm2, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm12\n\t"

                   /* compute d21: dot product (v2, v1) */
                   "vbroadcastss %[v1x], %%ymm4\n\t"
                   "vbroadcastss %[v1y], %%ymm5\n\t"
                   "dpps $0b11110001, %%ymm2, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm13\n\t"

                   /* compute denominator (d00 * d11 - d01 * d01) */
                   /* first, (d00 * d11) */
                   "vmovaps %%ymm9, %%ymm4\n\t"
                   "vmulps %%ymm11, %%ymm4, %%ymm4\n\t"
                   /* second, (d01 * d01) */
                   "vmovaps %%ymm10, %%ymm5\n\t"
                   "vmulps %%ymm10, %%ymm5, %%ymm5\n\t"
                   /* now, subtract */
                   "vsubps %%ymm5, %%ymm4, %%ymm4\n\t"

                   /* store denominator in ymm14 */
                   "vmovaps %%ymm4, %%ymm14\n\t"

                   /* compute weights */
                   /* w = (d00 * d21 - d01 * d20) / denominator */
                   /* (d00 * d21) */
                   "vmovaps %%ymm9, %%ymm4\n\t"
                   "vmulps %%ymm13, %%ymm4, %%ymm4\n\t"
                   /* (d01 * d20) */
                   "vmovaps %%ymm10, %%ymm5\n\t"
                   "vmulps %%ymm12, %%ymm5, %%ymm5\n\t"
                   /* subract them */
                   "vsubps %%ymm5, %%ymm4, %%ymm4\n\t"
                   /* divide and store in ymm7 (w) */
                   "vdivps %%ymm14, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm7\n\t"

                   /* v = (d11 * d20 - d01 * d21) / denominator */
                   /* (d11 * d20) */
                   "vmovaps %%ymm11, %%ymm4\n\t"
                   "vmulps %%ymm12, %%ymm4, %%ymm4\n\t"
                   /* d01 * d21 */
                   "vmovaps %%ymm10, %%ymm5\n\t"
                   "vmulps %%ymm13, %%ymm5, %%ymm5\n\t"
                   /* subtract them */
                   "vsubps %%ymm5, %%ymm4, %%ymm4\n\t"
                   /* divide and store in ymm8 (v) */
                   "vdivps %%ymm14, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm8\n\t"

                   /* u = 1.0 - v - w, ymm6 (u) */
                   "vbroadcastss %[one], %%ymm4\n\t"
                   "vmovaps %%ymm7, %%ymm5\n\t"
                   "vaddps %%ymm8, %%ymm5, %%ymm5\n\t"
                   "vsubps %%ymm5, %%ymm4, %%ymm6\n\t"

                   /* interpolate red, green, blue and alpha - ymm6 (u), ymm7 (w), ymm8 (v) */
                   /* at this point, ymm9+ registers are free */

                   /* red, v0 * u, v1 * v and v2 * w. interpolated red goes into ymm9 */
                   "vbroadcastss %[v0_r], %%ymm4\n\t"
                   "vcvtdq2ps %%ymm4, %%ymm4\n\t"
                   "vmulps %%ymm6, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v1_r], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm8, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v2_r], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm7, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm9\n\t"

                   /* green, goes into ymm10 */
                   "vbroadcastss %[v0_g], %%ymm4\n\t"
                   "vcvtdq2ps %%ymm4, %%ymm4\n\t"
                   "vmulps %%ymm6, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v1_g], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm8, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v2_g], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm7, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm10\n\t"

                   /* blue, goes into ymm11 */
                   "vbroadcastss %[v0_b], %%ymm4\n\t"
                   "vcvtdq2ps %%ymm4, %%ymm4\n\t"
                   "vmulps %%ymm6, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v1_b], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm8, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v2_b], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm7, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm11\n\t"

                   /* alpha, goes into ymm12 */
                   "vbroadcastss %[v0_a], %%ymm4\n\t"
                   "vcvtdq2ps %%ymm4, %%ymm4\n\t"
                   "vmulps %%ymm6, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v1_a], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm8, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vbroadcastss %[v2_a], %%ymm5\n\t"
                   "vcvtdq2ps %%ymm5, %%ymm5\n\t"
                   "vmulps %%ymm7, %%ymm5, %%ymm5\n\t"
                   "vaddps %%ymm5, %%ymm4, %%ymm4\n\t"
                   "vmovaps %%ymm4, %%ymm12\n\t"

                   /* convert interpolated colours back to integers */
                   "vcvtps2dq %%ymm9, %%ymm9\n\t"
                   "vcvtps2dq %%ymm10, %%ymm10\n\t"
                   "vcvtps2dq %%ymm11, %%ymm11\n\t"
                   "vcvtps2dq %%ymm12, %%ymm12\n\t"

                   /* pack 32 bit integrs to 16 bit, then 16 to 8 */
                   "vpackssdw %%ymm9, %%ymm9, %%ymm9\n\t"
                   "vpackssdw %%ymm10, %%ymm10, %%ymm10\n\t"
                   "vpackssdw %%ymm11, %%ymm11, %%ymm11\n\t"
                   "vpackssdw %%ymm12, %%ymm12, %%ymm12\n\t"

                   "vpackuswb %%ymm9, %%ymm9, %%ymm9\n\t"
                   "vpackuswb %%ymm10, %%ymm10, %%ymm10\n\t"
                   "vpackuswb %%ymm11, %%ymm11, %%ymm11\n\t"
                   "vpackuswb %%ymm12, %%ymm12, %%ymm12\n\t"

                   /* here I have: ymm9 (eight 8bit red values) ymm10 (eight 8 bit green values...) */
                   /* it's shifting time */
                   "vpslld $24, %%ymm9, %%ymm9\n\t"
                   "vpslld $16, %%ymm10, %%ymm10\n\t"
                   "vpslld $8, %%ymm11, %%ymm11\n\t"

                   /* now OR the result */
                   "vpor %%ymm10, %%ymm9, %%ymm4\n\t"
                   "vpor %%ymm11, %%ymm12, %%ymm5\n\t"
                   "vpor %%ymm5, %%ymm4, %%ymm4\n\t"

                   /* store 8 pixels at once and advance by 8 */
                   "vmovdqu %%ymm4, (%[row])\n\t"
                   "add $32, %[row]\n\t"

                   /* loop conditions */
                   "dec %%ecx\n\t"
                   "jnz 1b\n\t"

                   : [row] "+r" (row)
                   : [chunks] "r" (chunks),
                     [v0_r] "m"(T->colours[0].r),
                     [v0_g] "m"(T->colours[0].g),
                     [v0_b] "m"(T->colours[0].b),
                     [v0_a] "m"(T->colours[0].a),
                     [v1_r] "m"(T->colours[1].r),
                     [v1_g] "m"(T->colours[1].g),
                     [v1_b] "m"(T->colours[1].b),
                     [v1_a] "m"(T->colours[1].a),
                     [v2_r] "m"(T->colours[2].r),
                     [v2_g] "m"(T->colours[2].g),
                     [v2_b] "m"(T->colours[2].b),
                     [v2_a] "m"(T->colours[2].a),
                     [v0x]  "m"(T->vertices[0].x),
                     [v0y]  "m"(T->vertices[0].y),
                     [v1x]  "m"(T->vertices[1].x),
                     [v1y]  "m"(T->vertices[1].y),
                     [one]  "m"(one),
                     [y]    "r"(y),
                     [x]    "r"(x_start)
                   : "ecx", "ymm0", "ymm1", "ymm2", "ymm3",
                     "ymm4", "ymm5", "ymm6", "ymm7", "ymm8",
                     "ymm9", "ymm10", "ymm11", "ymm12", "ymm13"
                   );
}

static void
draw_triangle_filled_colours_simd(xcr_context *ctx, stack_arena *arena, xcr_triangle_colours *T)
{
  if (T->vertices[1].y < T->vertices[0].y)
    {
      XC_SWAP(xc_vec2i, T->vertices[0], T->vertices[1]);
      XC_SWAP(xc_colour, T->colours[0], T->colours[1]);
    }

  if (T->vertices[2].y < T->vertices[0].y)
    {
      XC_SWAP(xc_vec2i, T->vertices[2], T->vertices[0]);
      XC_SWAP(xc_colour, T->colours[2], T->colours[0]);
    }

  if (T->vertices[2].y < T->vertices[1].y)
    {
      XC_SWAP(xc_vec2i, T->vertices[2], T->vertices[1]);
      XC_SWAP(xc_colour, T->colours[2], T->colours[1]);
    }

  int32_t const simd_width = get_simd_width();
  int32_t const x01_size = T->vertices[1].y - T->vertices[0].y + 1;
  int32_t const x12_size = T->vertices[2].y - T->vertices[1].y + 1;
  int32_t const x02_size = T->vertices[2].y - T->vertices[0].y + 1;

  int32_t *x01  = xc_interpolate_array(arena, T->vertices[0].y, T->vertices[0].x, T->vertices[1].y, T->vertices[1].x, x01_size);
  int32_t *x12  = xc_interpolate_array(arena, T->vertices[1].y, T->vertices[1].x, T->vertices[2].y, T->vertices[2].x, x12_size);
  int32_t *x02  = xc_interpolate_array(arena, T->vertices[0].y, T->vertices[0].x, T->vertices[2].y, T->vertices[2].x, x02_size);
  int32_t *x012 = array_append(arena, x01, x12, x01_size - 1, x12_size);

  int32_t const mid = (x01_size + x12_size) >> 1;
  int32_t *x_left, *x_right;

  if (x02[mid] < x012[mid])
    {
      x_left = x02;
      x_right = x012;
    }
  else
    {
      x_left = x012;
      x_right = x02;
    }

  for (int32_t y = T->vertices[0].y; y <= T->vertices[2].y; ++y)
    {
      int32_t const x_start = x_left[y - T->vertices[0].y];
      int32_t const x_end   = x_right[y - T->vertices[0].y];
      int32_t const width   = x_end - x_start + 1;
      int32_t const chunks  = width / simd_width;

      if (chunks > 0)
        {
          /* compute barycentric weights and interpolated colours in chunks */
          uint32_t *row = &ctx->fb->pixels[y * ctx->fb->width + x_start];

          draw_coloured_pixels_interpolated_simd(row, x_start, y, T, chunks);

          continue;
        }

      /* compute barycentric weights and interpolated colours individually */

    }
}

xcr_context *
xcr_create(linear_arena *arena, xc_framebuffer *fb)
{
  xcr_context *ctx = linear_arena_alloc(arena, sizeof(xcr_context));
  if (!ctx) {
    (void)fprintf(stderr, "Couldn't allocate space for renderer\n");
    return NULL;
  }

  ctx->fb = fb;

  return ctx;
}

void
xcr_set_bg_colour(xcr_context *ctx, uint32_t colour)
{
  uint32_t *pixels = ctx->fb->pixels;
  XC_FILL_PIXELS_SIMD_ALIGNED(pixels, colour, ctx->fb->simd_chunks);
}

void
xcr_draw_line(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1, uint32_t colour)
{
  draw_line_bresenham(ctx, p0, p1, colour);
}

void
xcr_draw_quad_outline(xcr_context *ctx, xc_vec2i p0, int32_t width,
		      int32_t height, uint32_t colour)
{
  xc_vec2i p1 = { .x = p0.x, .y = p0.y + height };

  xc_vec2i p2 = { .x = p0.x + width, .y = p0.y };

  xc_vec2i p3 = { .x = p0.x + width, .y = p0.y + height };

  xcr_draw_line(ctx, p0, p2, colour);
  xcr_draw_line(ctx, p0, p1, colour);
  xcr_draw_line(ctx, p1, p3, colour);
  xcr_draw_line(ctx, p3, p2, colour);
}

void
xcr_draw_triangle_outline(xcr_context *ctx, xcr_triangle T, uint32_t colour)
{
  xcr_draw_line(ctx, T.vertices[0], T.vertices[1], colour);
  xcr_draw_line(ctx, T.vertices[1], T.vertices[2], colour);
  xcr_draw_line(ctx, T.vertices[2], T.vertices[0], colour);
}

void
xcr_draw_circle_outline(xcr_context *ctx, xc_vec2i center, int32_t radius, uint32_t colour)
{
  draw_circle_midpoint(ctx, center, radius, colour);
}

void
xcr_draw_quad_filled(xcr_context *ctx, xc_vec2i p0, int32_t width, int32_t height, uint32_t colour)
{
  /* Just in case I pass some weird shit as arguments... Like intentionally
     drawing at the edge of the screen */
  int32_t const xstart = XC_MAX(p0.x, 0);
  int32_t const ystart = XC_MAX(p0.y, 0);

  /* Careful not to go outside boundaries */
  int32_t const ymax = XC_MIN(p0.y + height, ctx->fb->height);
  int32_t const xmax = XC_MIN(p0.x + width, ctx->fb->width);

  for (int32_t y = ystart; y < ymax; ++y) {
    for (int32_t x = xstart; x < xmax; ++x) {
      put_pixel(ctx, x, y, colour);
    }
  }
}

void
xcr_draw_triangle_filled(xcr_context *ctx, stack_arena *arena, xcr_triangle triangle, uint32_t colour)
{
  draw_triangle_filled(arena, ctx, triangle, colour);
}

void
xcr_draw_circle_filled(xcr_context *ctx, xc_vec2i center, int32_t radius, uint32_t colour)
{
  /* x² + y² = r² */
  int32_t const radius_squared = radius * radius;
  int32_t const ystart = XC_MAX(center.y - radius, 0);
  int32_t const yend = XC_MIN(center.y + radius, ctx->fb->height - 1);

  for (int32_t y = ystart; y <= yend; ++y) {
    int32_t dy = y - center.y;
    int32_t width_squared = radius_squared - (dy * dy);

    if (width_squared >= 0) {
      int32_t width = (int32_t)xc_sqrt((f32_t)width_squared);
      int32_t xstart = XC_MAX(center.x - width, 0);
      int32_t xend = XC_MIN(center.x + width, ctx->fb->width - 1);

      for (int32_t x = xstart; x <= xend; ++x) {
        put_pixel(ctx, x, y, colour);
      }
    }
  }
}

void
xcr_draw_triangle_filled_colours(xcr_context *ctx, stack_arena *arena, xcr_triangle_colours *T)
{
  // draw_triangle_filled_colours(ctx, T);
  draw_triangle_filled_colours_simd(ctx, arena, T);
}
