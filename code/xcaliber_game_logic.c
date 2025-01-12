#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include "xcaliber_common.h"
#include <SDL3/SDL.h>

GAME_API void
game_update(__attribute__((unused)) game_ctx *ctx)
{
	/* Only game physics and logic! */
}

static void
r_draw_line(framebuffer *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
	    uint32_t colour)
{
	/* how far I need to go in each direction */
	int32_t dx = x1 - x0, dy = y1 - y0;
	bool steep = ABS(dy) > ABS(dx);

	/* if the line is steep, step along y instead of x */
	if (steep) {
		/* swap x and y coords */
		SWAP(int, x0, y0);
		SWAP(int, x1, y1);
	}

	/* I'm going left, so swap */
	if (x1 < x0) {
		SWAP(int, x0, x1);
		SWAP(int, y0, y1);
	}

	dx = x1 - x0;
	dy = y1 - y0;

	/* decision, helps me decide go horizontally or diagonally */
	int32_t D = 2 * ABS(dy) - dx;
	int32_t y = y0;
	int32_t y_step = (dy < 0) ? -1 : 1;

	for (int32_t x = x0; x <= x1; ++x) {
		/* colourise pixel */
		if (steep) {
			fb->pixels[x * fb->width + y] = colour;
		} else {
			fb->pixels[y * fb->width + x] = colour;
		}

		/* if D > 0, then I'm too far from the line, so I need to increment y */
		/* D <= 0, then only move horizontally */
		/* to remain in integer land I multiply by 2 */
		if (D > 0) {
			y += y_step;
			D += 2 * (ABS(dy) - dx);
		} else {
			D += 2 * ABS(dy);
		}
	}
}

static void
r_draw_quad_outline(framebuffer *fb, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t colour)
{
	r_draw_line(fb, x, y, x + width, y, colour);
	r_draw_line(fb, x, y, x, y + height, colour);
	r_draw_line(fb, x, y + height, x + width, y + height, colour);
	r_draw_line(fb, x + width, y + height, x + width, y, colour);
}

static void
r_draw_triangle_outline(framebuffer *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t colour)
{
	r_draw_line(fb, x0, y0, x1, y1, colour);
	r_draw_line(fb, x1, y1, x2, y2, colour);
	r_draw_line(fb, x2, y2, x0, y0, colour);
}

static inline
void plot_points(framebuffer *fb, int32_t cx, int32_t cy, int32_t x, int32_t y, uint32_t colour)
{
	/* each point I compute gives me 8 points on the circle (symmetry) */
	fb->pixels[(cy + y) * fb->width + (cx + x)] = colour; /* octant 1 */
	fb->pixels[(cy + x) * fb->width + (cx + y)] = colour; /* octant 2 */
	fb->pixels[(cy + x) * fb->width + (cx - y)] = colour; /* octant 3 */
	fb->pixels[(cy + y) * fb->width + (cx - x)] = colour; /* octant 4 */
	fb->pixels[(cy - y) * fb->width + (cx - x)] = colour; /* octant 5 */
	fb->pixels[(cy - x) * fb->width + (cx - y)] = colour; /* octant 6 */
	fb->pixels[(cy - x) * fb->width + (cx + y)] = colour; /* octant 7 */
	fb->pixels[(cy - y) * fb->width + (cx + x)] = colour; /* octant 8 */
}

static void
r_draw_circle_outline(framebuffer *fb, int32_t x, int32_t y, int32_t r, uint32_t colour)
{
	int32_t curr_x = 0, curr_y = r;
	int32_t D = 3 - (2 * r);

	/* draw first set of points */
	plot_points(fb, x, y, curr_x, curr_y, colour);

	while (curr_y > curr_x) {
		/* move inward or not */
		if (D > 0) {
			--curr_y;
			D = D + 4 * (curr_x - curr_y) + 10;
		} else {
			D = D + 4 * curr_x + 6;
		}
		++curr_x;
		plot_points(fb, x, y, curr_x, curr_y, colour);
	}
}

GAME_API void
game_render(game_ctx *ctx)
{
	uint32_t const
		bg_colour = 0x0020f0ff,
		line_colour = 0x1B1728FF,
		quad_colour = 0xFF0000FF,
		tri_colour = 0xA9898DFF,
		circle_colour = 0xFFFFFFFF;
	uint32_t *pixels = ctx->fb.pixels;

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
		: "m"(bg_colour), /* %0 is the colour */
		  "r"(pixels), /* %1 pixel buffer */
		  "r"(ctx->fb.simd_chunks) /* %2 current chunk count */
		: "ymm0", "rax", "rcx",
		  "memory" /* tells the compiler which registers I'm using */
	);

	/* L */
	r_draw_line(&ctx->fb, 50, 100, 50, 400, line_colour);
	r_draw_line(&ctx->fb, 50, 400, 200, 400, line_colour);

	/* A */
	r_draw_line(&ctx->fb, 320, 100, 220, 400, line_colour);
	r_draw_line(&ctx->fb, 255, 300, 385, 300, line_colour);
	r_draw_line(&ctx->fb, 320, 100, 420, 400, line_colour);

	/* I */
	r_draw_line(&ctx->fb, 440, 100, 440, 400, line_colour);

	/* N */
	r_draw_line(&ctx->fb, 520, 100, 520, 400, line_colour);
	r_draw_line(&ctx->fb, 520, 100, 650, 400, line_colour);
	r_draw_line(&ctx->fb, 650, 100, 650, 400, line_colour);

	r_draw_quad_outline(&ctx->fb, 700, 500, 200, 200, quad_colour);

	r_draw_triangle_outline(&ctx->fb, 600, 550, 500, 450, 500, 650, tri_colour);

	r_draw_circle_outline(&ctx->fb, 250, 550, 100, circle_colour);
}
