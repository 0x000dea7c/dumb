#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include <SDL3/SDL.h>
#include <stdio.h>

/* FIXME: I don't want to use SDL here, this should be platform independent code. */
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
	/* decision, helps me decide go horizontally or diagonally */
	int32_t D = 2 * dy - dx;
	int32_t y = y0;

	/* Walk along the X axis */
	for (int32_t x = x0; x <= x1; ++x) {
		/* colourise pixel */
		fb->pixels[y * (int32_t)fb->width + x] = colour;

		/* if D > 0, then I'm too far from the line, so I need to increment y */
		/* D <= 0, then only move horizontally */
		/* to remain in integer land I multiply by 2 */
		if (D > 0) {
			++y;
			D += 2 * (dy - dx);
		} else {
			D += 2 * dy;
		}
	}
}

GAME_API void
game_render(game_ctx *ctx)
{
	uint32_t const bg_colour = 0x0020f0ff, line_colour = 0x1B1728FF;
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

	r_draw_line(&ctx->fb, 10, 10, 400, 400, line_colour);
}
