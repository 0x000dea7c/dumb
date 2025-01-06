#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include <SDL3/SDL.h>
#include <stdint.h>

/* FIXME: I don't want to use SDL here, this should be platform independent code. */
GAME_API void game_update(game_ctx *ctx, __attribute__((unused)) float dt)
{
	uint32_t const red = 0x0000ffFF;
	for (uint32_t i = 0; i < ctx->fb.pixel_count; ++i) {
		ctx->fb.pixels[i] = red;
	}
	SDL_UpdateTexture(ctx->texture, NULL, ctx->fb.pixels, (int)ctx->fb.pitch);
}

GAME_API void game_render(game_ctx *ctx)
{
	SDL_RenderClear(ctx->renderer);
	SDL_RenderTexture(ctx->renderer, ctx->texture, NULL, NULL);
	SDL_RenderPresent(ctx->renderer);
}
