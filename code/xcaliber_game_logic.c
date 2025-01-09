#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include <SDL3/SDL.h>
#include <stdio.h>

/* FIXME: I don't want to use SDL here, this should be platform independent code. */
GAME_API void game_update(__attribute__((unused)) game_ctx *ctx)
{
	/* Only game physics and logic! */
}

GAME_API void game_render(game_ctx *ctx)
{
	/* This only works for black and white! */
	uint32_t const colour = 0x00000000;
	memset(ctx->fb.pixels, colour, ctx->fb.byte_size);

	/* Copy my framebuffer to the GPU texture */
	SDL_UpdateTexture(ctx->texture, NULL, ctx->fb.pixels, (int)ctx->fb.pitch);

	SDL_RenderClear(ctx->renderer);
	SDL_RenderTexture(ctx->renderer, ctx->texture, NULL, NULL);
	SDL_RenderPresent(ctx->renderer);
}
