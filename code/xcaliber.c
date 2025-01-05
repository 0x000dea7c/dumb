#include "xcaliber.h"
#include "xcaliber_common.h"
#include "xcaliber_linear_arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static sdl_window_dimensions win_dims = { .width = 1920, .height = 1080 };
static xcaliber_state state = { .running = true };
static framebuffer fb;
static linear_arena arena;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static unsigned char *game_mem = NULL;

static void
cleanup(void)
{
	if (game_mem) {
		free(game_mem);
	}

	SDL_Quit();
}

static void panic(char const *title, char const *msg) __attribute__((noreturn));

static void
panic(char const *title, char const *msg)
{
	(void)fprintf(stderr, "%s - %s\n", title, msg);

	cleanup();

	exit(EXIT_FAILURE);
}

static void
sdl_initialise(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		panic("SDL_Init", SDL_GetError());
	}

	if (!SDL_CreateWindowAndRenderer("XCaliber", win_dims.width,
					 win_dims.height, 0, &window,
					 &renderer)) {
		panic("SDL_CreateWindowAndRenderer", SDL_GetError());
	}

	SDL_SetRenderVSync(renderer, 1);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
				    SDL_TEXTUREACCESS_STREAMING, win_dims.width,
				    win_dims.height);
	if (!texture) {
		panic("SDL_CreateTexture", SDL_GetError());
	}
}

static void
game_mem_initialise(void)
{
	uint32_t const game_mem_len = MEGABYTES(512);

	game_mem = malloc(game_mem_len);
	if (!game_mem) {
		panic("main game memory", "Couldn't alloc memory for the game");
	}

	linear_arena_init(&arena, game_mem, game_mem_len);
}

static void
framebuffer_initialise(void)
{
	fb.width = (uint32_t)win_dims.width;
	fb.height = (uint32_t)win_dims.height;
	fb.pitch = fb.width * sizeof(uint32_t);
	fb.pixel_count = fb.width * fb.height;
	fb.byte_size = fb.pixel_count * sizeof(uint32_t);

	/* ask the arena to get a chunk of memory */
	fb.pixels = linear_arena_alloc(&arena, fb.byte_size);
	if (!fb.pixels) {
		panic("main linear arena alloc",
		      "Couldn't get a chunk of memory for the framebuffer from the arena");
	}
}

static void
update(void)
{
	uint32_t const red = 0xFF0000FF;
	for (uint32_t i = 0; i < fb.pixel_count; ++i) {
		fb.pixels[i] = red;
	}
	SDL_UpdateTexture(texture, NULL, fb.pixels, (int)fb.pitch);
}

static void
render(void)
{
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void
run(void)
{
	SDL_Event event;

	while (state.running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				state.running = false;
				break;
			}
			if (event.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode const key = event.key.key;
				switch (key) {
				case SDLK_ESCAPE:
					state.running = false;
					break;
				case SDLK_Q:
					printf("Pressed Q!\n");
					break;
				default:
					break;
				}
			}
		}

		update();
		render();
	}
}

int
main(void)
{
	sdl_initialise();
	game_mem_initialise();
	framebuffer_initialise();

	run();

	cleanup();

	return EXIT_SUCCESS;
}
