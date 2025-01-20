#include "xcaliber_renderer.h"
#include "xcaliber_math.h"
#include "xcaliber_common.h"
#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#if defined(__AVX2__)
#define XC_FILL_PIXELS_SIMD(dst, colour, count) \
    __asm__ volatile( \
        /* Load chunk count into rax */ \
        "movq %2, %%rax\n\t" \
        /* Broadcast 32-bit colour to all 8 lanes of ymm0 */ \
        "vpbroadcastd %0, %%ymm0\n\t" \
        /* Load destination buffer address into rcx */ \
        "movq %1, %%rcx\n\t" \
        /* Main loop label */ \
        "1:\n\t" \
        /* Store 8 pixels (256 bits) aligned */ \
        "vmovdqa %%ymm0, (%%rcx)\n\t" \
        /* Move to next chunk (32 bytes = 8 pixels × 4 bytes) */ \
        "addq $32, %%rcx\n\t" \
        /* Decrement counter */ \
        "decq %%rax\n\t" \
        /* Loop if counter not zero */ \
        "jnz 1b\n\t" \
        : /* No outputs */ \
        : "m"(colour), /* %0: colour to broadcast */ \
          "r"(dst),    /* %1: destination buffer */ \
          "r"(count)   /* %2: number of chunks */ \
        : "ymm0", "rax", "rcx", "memory" \
    )

#elif defined(__SSE4_2__)
#define XC_FILL_PIXELS_SIMD(dst, colour, count) \
    __asm__ volatile( \
        /* Load chunk count into rax */ \
        "movq %2, %%rax\n\t" \
        /* Move 32-bit colour into lowest lane of xmm0 */ \
        "movd %0, %%xmm0\n\t" \
        /* Broadcast to all 4 lanes */ \
        "pshufd $0, %%xmm0, %%xmm0\n\t" \
        /* Load destination buffer address */ \
        "movq %1, %%rcx\n\t" \
        /* Main loop label */ \
        "1:\n\t" \
        /* Store 4 pixels (128 bits) aligned */ \
        "movdqa %%xmm0, (%%rcx)\n\t" \
        /* Move to next chunk (16 bytes = 4 pixels × 4 bytes) */ \
        "addq $16, %%rcx\n\t" \
        /* Decrement counter */ \
        "decq %%rax\n\t" \
        /* Loop if counter not zero */ \
        "jnz 1b\n\t" \
        : /* No outputs */ \
        : "m"(colour), /* %0: colour to broadcast */ \
          "r"(dst),    /* %1: destination buffer */ \
          "r"(count)   /* %2: number of chunks */ \
        : "xmm0", "rax", "rcx", "memory" \
    )

#else
#error "engine needs at least SSE4.2 support"
#endif

struct xcr_context {
	xc_framebuffer *fb;
};

static inline void
xcr_put_pixel(xcr_context *ctx, int32_t x, int32_t y, uint32_t colour)
{
#ifdef DEBUG
	if (x < 0 || x > ctx->fb->width || y < 0 || y > ctx->fb->height) {
		(void)fprintf(
			stderr,
			"WARNING: OUT OF BOUNDS FRAMEBUFFER UPDATE x: %d, y: %d\n",
			x, y);
		(void)fflush(stderr);
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
	xcr_put_pixel(ctx, (center.x + p.x), (center.y + p.y), c);
	/* octant 2 */
	xcr_put_pixel(ctx, (center.x + p.y), (center.y + p.x), c);
	/* octant 3 */
	xcr_put_pixel(ctx, (center.x - p.y), (center.y + p.x), c);
	/* octant 4 */
	xcr_put_pixel(ctx, (center.x - p.x), (center.y + p.y), c);

	/* octant 5 */
	xcr_put_pixel(ctx, (center.x - p.x), (center.y - p.y), c);
	/* octant 6 */
	xcr_put_pixel(ctx, (center.x - p.y), (center.y - p.x), c);
	/* octant 7 */
	xcr_put_pixel(ctx, (center.x + p.y), (center.y - p.x), c);
	/* octant 8 */
	xcr_put_pixel(ctx, (center.x + p.x), (center.y - p.y), c);
}

static inline void
draw_horizontal_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1,
			       uint32_t colour, int32_t dx, int32_t dy,
			       int32_t dy_abs)
{
	if (p1.x < p0.x) {
		XC_SWAP(int, p0.x, p1.x);
		XC_SWAP(int, p0.y, p1.y);
		dx = p1.x - p0.x;
		dy = p1.y - p0.y;
		dy_abs = XC_ABS(dy);
	}

	int32_t D = 2 * dy - dx;
	int32_t y = p0.y;
	int32_t y_step = (dy < 0) ? -1 : 1;

	for (int32_t x = p0.x; x <= p1.x; ++x) {
		xcr_put_pixel(ctx, x, y, colour);

		if (D > 0) {
			y += y_step;
			D += 2 * (dy_abs - dx);
		} else {
			D += 2 * dy_abs;
		}
	}
}

static inline void
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
		xcr_put_pixel(ctx, y, x, colour);

		if (D > 0) {
			y += y_step;
			D += 2 * (dy_abs - dx);
		} else {
			D += 2 * dy_abs;
		}
	}
}

static inline void
draw_line_bresenham(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1, uint32_t colour)
{
	int32_t dx = p1.x - p0.x;
	int32_t dy = p1.y - p0.y;
	int32_t dy_abs = XC_ABS(dy);
	int32_t dx_abs = XC_ABS(dx);
	bool steep = dy_abs > dx_abs;

	if (steep) {
		draw_vertical_line_bresenham(ctx, p0, p1, colour, dx, dy,
					     dy_abs);
	} else {
		draw_horizontal_line_bresenham(ctx, p0, p1, colour, dx, dy,
					       dy_abs);
	}
}

static inline void
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

static inline int32_t
xc_edge_function(xc_vec2i a, xc_vec2i b, xc_vec2i p)
{
	xc_vec2i edge = xc_vec2i_sub(b, a);
	xc_vec2i to_p = xc_vec2i_sub(p, a);
	return xc_vec2i_cross_product(edge, to_p);
}

static inline bool
point_inside_triangle(xc_vec2i p, xc_vec2i a, xc_vec2i b, xc_vec2i c)
{
	int32_t edge0 = xc_edge_function(a, b, p);
	int32_t edge1 = xc_edge_function(b, c, p);
	int32_t edge2 = xc_edge_function(c, a, p);
	return (edge0 > 0 && edge1 > 0 && edge2 > 0) ||
	       (edge0 < 0 && edge1 < 0 && edge2 < 0);
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
	XC_FILL_PIXELS_SIMD(pixels, colour, ctx->fb->simd_chunks);
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
	xcr_draw_line(ctx, T.p0, T.p1, colour);
	xcr_draw_line(ctx, T.p1, T.p2, colour);
	xcr_draw_line(ctx, T.p2, T.p0, colour);
}

void
xcr_draw_circle_outline(xcr_context *ctx, xc_vec2i center, int32_t r,
			uint32_t colour)
{
	draw_circle_midpoint(ctx, center, r, colour);
}

void
xcr_draw_quad_filled(xcr_context *ctx, xc_vec2i p0, int32_t width,
		     int32_t height, uint32_t colour)
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
			xcr_put_pixel(ctx, x, y, colour);
		}
	}
}

void
xcr_draw_triangle_filled(xcr_context *ctx, xcr_triangle T, uint32_t colour)

{
	/* Find bounding box */
	int32_t xmin = XC_MIN((int32_t)T.p0.x,
			      XC_MIN((int32_t)T.p1.x, (int32_t)T.p2.x));
	int32_t ymin = XC_MIN((int32_t)T.p0.y,
			      XC_MIN((int32_t)T.p1.y, (int32_t)T.p2.y));
	int32_t xmax = XC_MAX((int32_t)T.p0.x,
			      XC_MAX((int32_t)T.p1.x, (int32_t)T.p2.x));
	int32_t ymax = XC_MAX((int32_t)T.p0.y,
			      XC_MAX((int32_t)T.p1.y, (int32_t)T.p2.y));

	/* Bounds checking */
	xmin = XC_MAX(xmin, 0);
	ymin = XC_MAX(ymin, 0);
	xmax = XC_MIN(xmax, ctx->fb->width - 1);
	ymax = XC_MIN(ymax, ctx->fb->height - 1);

	/* Scan each line and fill if I'm inside the triangle */
	for (int32_t y = ymin; y <= ymax; ++y) {
		for (int32_t x = xmin; x <= xmax; ++x) {
			if (point_inside_triangle((xc_vec2i){ .x = x, .y = y },
						  T.p0, T.p1, T.p2)) {
				xcr_put_pixel(ctx, x, y, colour);
			}
		}
	}
}

void
xcr_draw_circle_filled(xcr_context *ctx, xc_vec2i center, int32_t r,
		       uint32_t colour)
{
	/* x² + y² = r² */
	int32_t const r_sq = r * r;
	int32_t ystart = XC_MAX(center.y - r, 0);
	int32_t yend = XC_MIN(center.y + r, ctx->fb->height - 1);

	for (int32_t y = ystart; y <= yend; ++y) {
		int32_t dy = y - center.y;
		int32_t width_sq = r_sq - (dy * dy);
		if (width_sq >= 0) {
			int32_t width = (int32_t)xc_sqrt((float)width_sq);
			int32_t xstart = XC_MAX(center.x - width, 0);
			int32_t xend =
				XC_MIN(center.x + width, ctx->fb->width - 1);

			for (int32_t x = xstart; x <= xend; ++x) {
				xcr_put_pixel(ctx, x, y, colour);
			}
		}
	}
}
