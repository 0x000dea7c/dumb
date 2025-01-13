#include "xcaliber_renderer.h"
#include "xcaliber_math.h"
#include "xcaliber_common.h"
#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include <stdio.h>

struct xcr_context {
	xc_framebuffer *fb;
};

static uint32_t
xcr_colour_to_uint(xcr_colour c)
{
	return (uint32_t)(c.r) << 24 | (uint32_t)(c.g) << 16 |
	       (uint32_t)(c.b) << 8 | (uint32_t)(c.a);
}

static inline
void plot_points(xcr_context *ctx, xcr_point center, xcr_point p, xcr_colour colour)
{
	uint32_t c = xcr_colour_to_uint(colour);

	/* each point I compute gives me 8 points on the circle (symmetry) */
	ctx->fb->pixels[(center.y + p.y) * ctx->fb->width + (center.x + p.x)] = c; /* octant 1 */
	ctx->fb->pixels[(center.y + p.x) * ctx->fb->width + (center.x + p.y)] = c; /* octant 2 */
	ctx->fb->pixels[(center.y + p.x) * ctx->fb->width + (center.x - p.y)] = c; /* octant 3 */
	ctx->fb->pixels[(center.y + p.y) * ctx->fb->width + (center.x - p.x)] = c; /* octant 4 */
	ctx->fb->pixels[(center.y - p.y) * ctx->fb->width + (center.x - p.x)] = c; /* octant 5 */
	ctx->fb->pixels[(center.y - p.x) * ctx->fb->width + (center.x - p.y)] = c; /* octant 6 */
	ctx->fb->pixels[(center.y - p.x) * ctx->fb->width + (center.x + p.y)] = c; /* octant 7 */
	ctx->fb->pixels[(center.y - p.y) * ctx->fb->width + (center.x + p.x)] = c; /* octant 8 */
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
	/* how far I need to go in each direction */
	int32_t dx = p1.x - p0.x, dy = p1.y - p0.y;
	bool steep = XC_ABS(dy) > XC_ABS(dx);

	/* if the line is steep, step along y instead of x */
	if (steep) {
		/* swap x and y coords */
		XC_SWAP(int, p0.x, p0.y);
		XC_SWAP(int, p1.x, p1.y);
	}

	/* I'm going left, so swap */
	if (p1.x < p0.x) {
		XC_SWAP(int, p0.x, p1.x);
		XC_SWAP(int, p0.y, p1.y);
	}

	dx = p1.x - p0.x;
	dy = p1.y - p0.y;

	/* decision, helps me decide go horizontally or diagonally */
	int32_t D = 2 * XC_ABS(dy) - dx;
	int32_t y = p0.y;
	int32_t y_step = (dy < 0) ? -1 : 1;
	uint32_t colour_fb = xcr_colour_to_uint(colour);

	for (int32_t x = p0.x; x <= p1.x; ++x) {
		/* colourise pixel */
		if (steep) {
			ctx->fb->pixels[x * ctx->fb->width + y] = colour_fb;
		} else {
			ctx->fb->pixels[y * ctx->fb->width + x] = colour_fb;
		}

		/* if D > 0, then I'm too far from the line, so I need to increment y */
		/* D <= 0, then only move horizontally */
		/* to remain in integer land I multiply by 2 */
		if (D > 0) {
			y += y_step;
			D += 2 * (XC_ABS(dy) - dx);
		} else {
			D += 2 * XC_ABS(dy);
		}
	}
}

void
xcr_draw_quad_outline(xcr_context *ctx, xcr_point p0, int32_t width,
		      int32_t height, xcr_colour colour)
{
	xcr_point p1 = {
		.x = p0.x,
		.y = p0.y + height
	};

	xcr_point p2 = {
		.x = p0.x + width,
		.y = p0.y
	};

	xcr_point p3 = {
		.x = p0.x + width,
		.y = p0.y + height
	};

	xcr_draw_line(ctx, p0, p2, colour);
	xcr_draw_line(ctx, p0, p1, colour);
	xcr_draw_line(ctx, p1, p3, colour);
	xcr_draw_line(ctx, p3, p2, colour);
}

void
xcr_draw_triangle_outline(xcr_context *ctx, xcr_point p0, xcr_point p1,
			  xcr_point p2, xcr_colour colour)
{
	xcr_draw_line(ctx, p0, p1, colour);
	xcr_draw_line(ctx, p1, p2, colour);
	xcr_draw_line(ctx, p2, p0, colour);
}

void
xcr_draw_circle_outline(xcr_context *ctx, xcr_point center, int32_t r,
			xcr_colour colour)
{
	xcr_point curr = { .x = 0, .y = r };
	int32_t D = 3 - (2 * r);

	/* draw first set of points */
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
