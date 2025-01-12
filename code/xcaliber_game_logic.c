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

GAME_API void
game_render(game_ctx *ctx)
{
	uint32_t const colour = 0x0020f0ff;
	uint32_t *pixels = ctx->fb.pixels;

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
		: "m"(colour), /* %0 is the colour */
		  "r"(pixels), /* %1 pixel buffer */
		  "r"(ctx->fb.simd_chunks) /* %2 current chunk count */
		: "ymm0", "rax", "rcx", "memory" /* tells the compiler which registers I'm using */
	);

	/* Copy my framebuffer to the GPU texture */
	SDL_UpdateTexture(ctx->texture, NULL, ctx->fb.pixels, (int)ctx->fb.pitch);

	SDL_RenderClear(ctx->renderer);
	SDL_RenderTexture(ctx->renderer, ctx->texture, NULL, NULL);
	SDL_RenderPresent(ctx->renderer);
}
