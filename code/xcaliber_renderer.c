#include "xcaliber_renderer.h"
#include "xcaliber_math.h"
#include "xcaliber_common.h"
#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include <stdio.h>
#include <assert.h>

struct xcr_context {
	xc_framebuffer *fb;
};

static inline uint32_t
xcr_colour_to_uint(xcr_colour c)
{
	return (uint32_t)(c.r) << 24 | (uint32_t)(c.g) << 16 |
	       (uint32_t)(c.b) << 8 | (uint32_t)(c.a);
}

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
plot_points(xcr_context *ctx, xcr_point center, xcr_point p, xcr_colour colour)
{
	uint32_t c = xcr_colour_to_uint(colour);

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
draw_horizontal_line_bresenham(xcr_context *ctx, xcr_point p0, xcr_point p1,
			       xcr_colour colour, int32_t dx, int32_t dy,
			       int32_t dy_abs)
{
	uint32_t const colour_fb = xcr_colour_to_uint(colour);

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
		xcr_put_pixel(ctx, x, y, colour_fb);

		if (D > 0) {
			y += y_step;
			D += 2 * (dy_abs - dx);
		} else {
			D += 2 * dy_abs;
		}
	}
}

static inline void
draw_vertical_line_bresenham(xcr_context *ctx, xcr_point p0, xcr_point p1,
			     xcr_colour colour, int32_t dx, int32_t dy,
			     int32_t dy_abs)
{
	uint32_t const colour_fb = xcr_colour_to_uint(colour);

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
		xcr_put_pixel(ctx, y, x, colour_fb);

		if (D > 0) {
			y += y_step;
			D += 2 * (dy_abs - dx);
		} else {
			D += 2 * dy_abs;
		}
	}
}

static inline void
draw_line_bresenham(xcr_context *ctx, xcr_point p0, xcr_point p1,
		    xcr_colour colour)
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
draw_circle_midpoint(xcr_context *ctx, xcr_point center, int32_t r,
		     xcr_colour colour)
{
	/* start at the top! */
	xcr_point curr = { .x = 0, .y = r };
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

static inline bool
point_inside_triangle(xcr_point P, xcr_triangle T)
{
	/* AB and P */
	int32_t edge0 = (T.p1.x - T.p0.x) * (P.y - T.p0.y) -
			(T.p1.y - T.p0.y) * (P.x - T.p0.x);
	/* BC and P */
	int32_t edge1 = (T.p2.x - T.p1.x) * (P.y - T.p1.y) -
			(T.p2.y - T.p1.y) * (P.x - T.p1.x);
	/* CA and P */
	int32_t edge2 = (T.p0.x - T.p2.x) * (P.y - T.p2.y) -
			(T.p0.y - T.p2.y) * (P.x - T.p2.x);

	return (edge0 >= 0 && edge1 >= 0 && edge2 >= 0) ||
	       (edge0 <= 0 && edge1 <= 0 && edge2 <= 0);
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
xcr_set_bg_colour(xcr_context *ctx, xcr_colour colour)
{
	uint32_t c = xcr_colour_to_uint(colour);
	uint32_t *pixels = ctx->fb->pixels;

	/* Draw background as blue */
	/* AT&T src, dst */
	__asm__ volatile(
		/* Load chunk count into rax */
		"movq %2, %%rax\n\t"

		/* Broadcast colour to all 8 chunks of ymm0 */
		"vpbroadcastd %0, %%ymm0\n\t"

		/* Load pixel buffer into rcx */
		"movq %1, %%rcx\n\t"

		/* Loop label */
		"1:\n\t"

		/* Store 8 pixels (256 bits, 4 bytes per pixel) aligned */
		"vmovdqa %%ymm0, (%%rcx)\n\t"

		/* Go to the next chunk (add 32) */
		"addq $32, %%rcx\n\t"

		/* Decrement counter (chunk count) */
		"decq %%rax\n\t"

		/* Jump back if not zero */
		"jnz 1b\n\t"

		/* Outputs */
		:
		: "m"(c), /* %0 is the colour */
		  "r"(pixels), /* %1 pixel buffer */
		  "r"(ctx->fb->simd_chunks) /* %2 current chunk count */
		: "ymm0", "rax", "rcx",
		  "memory" /* tells the compiler which registers I'm using */
	);
}

void
xcr_draw_line(xcr_context *ctx, xcr_point p0, xcr_point p1, xcr_colour colour)
{
	draw_line_bresenham(ctx, p0, p1, colour);
}

void
xcr_draw_quad_outline(xcr_context *ctx, xcr_point p0, int32_t width,
		      int32_t height, xcr_colour colour)
{
	xcr_point p1 = { .x = p0.x, .y = p0.y + height };

	xcr_point p2 = { .x = p0.x + width, .y = p0.y };

	xcr_point p3 = { .x = p0.x + width, .y = p0.y + height };

	xcr_draw_line(ctx, p0, p2, colour);
	xcr_draw_line(ctx, p0, p1, colour);
	xcr_draw_line(ctx, p1, p3, colour);
	xcr_draw_line(ctx, p3, p2, colour);
}

void
xcr_draw_triangle_outline(xcr_context *ctx, xcr_triangle triangle,
			  xcr_colour colour)
{
	xcr_draw_line(ctx, triangle.p0, triangle.p1, colour);
	xcr_draw_line(ctx, triangle.p1, triangle.p2, colour);
	xcr_draw_line(ctx, triangle.p2, triangle.p0, colour);
}

void
xcr_draw_circle_outline(xcr_context *ctx, xcr_point center, int32_t r,
			xcr_colour colour)
{
	draw_circle_midpoint(ctx, center, r, colour);
}

void
xcr_draw_quad_filled(xcr_context *ctx, xcr_point p0, int32_t width,
		     int32_t height, xcr_colour colour)
{
	uint32_t const c = xcr_colour_to_uint(colour);

	/* Just in case I pass some weird shit as arguments... Like intentionally
	   drawing at the edge of the screen */
	int32_t const xstart = XC_MAX(p0.x, 0);
	int32_t const ystart = XC_MAX(p0.y, 0);

	/* Careful not to go outside boundaries */
	int32_t const ymax = XC_MIN(p0.y + height, ctx->fb->height);
	int32_t const xmax = XC_MIN(p0.x + width, ctx->fb->width);

	for (int32_t y = ystart; y < ymax; ++y) {
		for (int32_t x = xstart; x < xmax; ++x) {
			xcr_put_pixel(ctx, x, y, c);
		}
	}
}

void
xcr_draw_triangle_filled(xcr_context *ctx, xcr_triangle triangle,
			 xcr_colour colour)

{
	uint32_t const c = xcr_colour_to_uint(colour);

	/* Find bounding box */
	int32_t xmin =
		XC_MIN(triangle.p0.x, XC_MIN(triangle.p1.x, triangle.p2.x));
	int32_t ymin =
		XC_MIN(triangle.p0.y, XC_MIN(triangle.p1.y, triangle.p2.y));
	int32_t xmax =
		XC_MAX(triangle.p0.x, XC_MAX(triangle.p1.x, triangle.p2.x));
	int32_t ymax =
		XC_MAX(triangle.p0.y, XC_MAX(triangle.p1.y, triangle.p2.y));

	/* Bounds checking */
	xmin = XC_MAX(xmin, 0);
	ymin = XC_MAX(ymin, 0);
	xmax = XC_MIN(xmax, ctx->fb->width - 1);
	ymax = XC_MIN(ymax, ctx->fb->height - 1);

	/* Scan each line and fill if I'm inside the triangle */
	for (int32_t y = ymin; y <= ymax; ++y) {
		for (int32_t x = xmin; x <= xmax; ++x) {
			if (point_inside_triangle((xcr_point){ .x = x, .y = y },
						  triangle)) {
				xcr_put_pixel(ctx, x, y, c);
			}
		}
	}
}

void
xcr_draw_circle_filled(xcr_context *ctx, xcr_point center, int32_t r,
		       xcr_colour colour)
{
	/* x² + y² = r² */
	uint32_t const c = xcr_colour_to_uint(colour);
	int32_t const r_sq = r * r;
	int32_t ystart = XC_MAX(center.y - r, 0);
	int32_t yend = XC_MIN(center.y + r, ctx->fb->height - 1);

	for (int32_t y = ystart; y <= yend; ++y) {
		int32_t dy = y - center.y;
		int32_t width_sq = r_sq - (dy * dy);
		if (width_sq >= 0) {
			int32_t width = (int32_t)xcr_sqrt((float)width_sq);
			int32_t xstart = XC_MAX(center.x - width, 0);
			int32_t xend =
				XC_MIN(center.x + width, ctx->fb->width - 1);

			for (int32_t x = xstart; x <= xend; ++x) {
				xcr_put_pixel(ctx, x, y, c);
			}
		}
	}
}
