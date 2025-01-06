#include "xcaliber.h"
#include "xcaliber_common.h"
#include "xcaliber_hot_reload.h"
#include "xcaliber_linear_arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static sdl_window_dimensions win_dims = { .width = 1920, .height = 1080 };
static xcaliber_state state = { .running = true };
static linear_arena arena;
static SDL_Window *window = NULL;
static unsigned char *game_mem = NULL;
static hot_reload_lib_info game_logic_lib;
static game_ctx ctx;

static void
quit(void)
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

	quit();

	exit(EXIT_FAILURE);
}

static void
sdl_init(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		panic("SDL_Init", SDL_GetError());
	}

	if (!SDL_CreateWindowAndRenderer("XCaliber", win_dims.width,
					 win_dims.height, 0, &window,
					 &ctx.renderer)) {
		panic("SDL_CreateWindowAndRenderer", SDL_GetError());
	}

	SDL_SetRenderVSync(ctx.renderer, 1);

	ctx.texture = SDL_CreateTexture(ctx.renderer, SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_STREAMING,
					win_dims.width, win_dims.height);
	if (!ctx.texture) {
		panic("SDL_CreateTexture", SDL_GetError());
	}
}

static void
game_mem_init(void)
{
	uint32_t const game_mem_len = MEGABYTES(512);

	game_mem = malloc(game_mem_len);
	if (!game_mem) {
		panic("main game memory", "Couldn't alloc memory for the game");
	}

	linear_arena_init(&arena, game_mem, game_mem_len);
}

static void
game_ctx_init(void)
{
	ctx.fb.width = (uint32_t)win_dims.width;
	ctx.fb.height = (uint32_t)win_dims.height;
	ctx.fb.pitch = ctx.fb.width * sizeof(uint32_t);
	ctx.fb.pixel_count = ctx.fb.width * ctx.fb.height;
	ctx.fb.byte_size = ctx.fb.pixel_count * sizeof(uint32_t);

	/* ask the arena to get a chunk of memory */
	ctx.fb.pixels = linear_arena_alloc(&arena, ctx.fb.byte_size);
	if (!ctx.fb.pixels) {
		panic("main linear arena alloc",
		      "Couldn't get a chunk of memory for the framebuffer from the arena");
	}
}

void
run(void)
{
	float const dt = 1.0f / 60.0f;
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
				case SDLK_R:
					hot_reload_update(&game_logic_lib);
					break;
				default:
					break;
				}
			}
		}

		game_logic_lib.update(&ctx, dt);
		game_logic_lib.render(&ctx);
	}
}

int
main(void)
{
	sdl_init();
	game_mem_init();
	game_ctx_init();

	if (!hot_reload_init(&game_logic_lib, "libgamelogic.so")) {
		panic("hot_reload_init",
		      "couldn't load game's logic shared library!");
	}

	run();

	quit();

	return EXIT_SUCCESS;
}
