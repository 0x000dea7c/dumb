#include "xcaliber_game_logic.h"

#include "xcaliber.h"
#include <SDL3/SDL.h>
#include <stdint.h>

/* FIXME: this isn't good, but I'm trying things around now */
extern SDL_Texture *texture;
extern SDL_Renderer *renderer;
extern framebuffer fb;

/* FIXME: I don't want to use SDL here, this should be platform independent code. */
GAME_API void game_update(__attribute__((unused)) float dt)
{
	uint32_t const red = 0xFF0000FF;
	for (uint32_t i = 0; i < fb.pixel_count; ++i) {
		fb.pixels[i] = red;
	}
	SDL_UpdateTexture(texture, NULL, fb.pixels, (int)fb.pitch);
}

GAME_API void game_render(void)
{
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}
