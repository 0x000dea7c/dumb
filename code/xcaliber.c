#include "xcaliber.h"
#include "xcaliber_common.h"
#include "xcaliber_hot_reload.h"
#include "xcaliber_linear_arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define FIXED_TIMESTEP 1.0f / 60.0f
#define GAME_LOGIC_LIB_NAME "libgamelogic.so"

static linear_arena arena;
static SDL_Window *window = NULL;
static unsigned char *game_mem = NULL;
static hot_reload_lib_info game_logic_lib;
static game_ctx ctx;
static game_cfg cfg;

static void
quit(void)
{
	if (ctx.texture) {
		SDL_DestroyTexture(ctx.texture);
	}

	if (ctx.renderer) {
		SDL_DestroyRenderer(ctx.renderer);
	}

	if (window) {
		SDL_DestroyWindow(window);
	}

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

	window = SDL_CreateWindow("X-Caliber", (int32_t)cfg.window_width,
				  (int32_t)cfg.window_height, 0);
	if (!window) {
		panic("SDL_CreateWindow", SDL_GetError());
	}

	ctx.renderer = SDL_CreateRenderer(window, NULL);
	if (!ctx.renderer) {
		panic("SDL_CreateRenderer", SDL_GetError());
	}

	/* my framebuffer */
	ctx.texture = SDL_CreateTexture(ctx.renderer, SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_STREAMING,
					(int32_t)cfg.window_width, (int32_t)cfg.window_height);
	if (!ctx.texture) {
		panic("SDL_CreateTexture", SDL_GetError());
	}
}

static void
game_cfg_init(void)
{
	cfg.window_width = 1024;
	cfg.window_height = 768;
	cfg.target_fps = FIXED_TIMESTEP;
	cfg.vsync = false;
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
	ctx.fb.width = cfg.window_width;
	ctx.fb.height = cfg.window_height;
	ctx.fb.pitch = ctx.fb.width * sizeof(uint32_t);
	ctx.fb.pixel_count = ctx.fb.width * ctx.fb.height;
	ctx.fb.byte_size = ctx.fb.pixel_count * sizeof(uint32_t);
	ctx.fb.simd_chunks = ctx.fb.pixel_count / 8; /* assumming AVX2 processor */

	/* ask the arena to get a chunk of memory */
	ctx.fb.pixels = linear_arena_alloc(&arena, ctx.fb.byte_size);
	if (!ctx.fb.pixels) {
		panic("main linear arena alloc",
		      "Couldn't get a chunk of memory for the framebuffer from the arena");
	}

	ctx.physics_accumulator = 0.0f;
	ctx.fixed_timestep = FIXED_TIMESTEP;
	ctx.alpha = 0.0f;
	ctx.running = true;
	ctx.last_frame_time = SDL_GetTicks();

	if (cfg.vsync) {
		SDL_SetRenderVSync(ctx.renderer, 1);
	}
}

static void
toggle_vsync(void)
{
	SDL_SetRenderVSync(ctx.renderer, cfg.vsync ? 0 : 1);
	cfg.vsync = !cfg.vsync;
	printf("VSync value changed to: %d\n", cfg.vsync);
}

void
run(void)
{
	uint64_t frame_count = 0, last_time = SDL_GetTicks();
	uint64_t fps_update_time = last_time;
	float current_fps = 0.0f;
	SDL_Event event;
	char title[32];

	while (ctx.running) {
		/* check if the lib was modified, if so, reload it. This is non blocking! */
		if (hot_reload_lib_was_modified()) {
			hot_reload_update(&game_logic_lib);
		}

		uint64_t const current_time = SDL_GetTicks();
		float frame_time = (float)(current_time - last_time) / 1000.0f;
		last_time = current_time;

		/* Cap max frame rate, avoid spiral of death, that is to say, constantly trying to catch up
		 if I miss a deadline */
		if (frame_time > 0.25f) {
			frame_time = 0.25f;
		}

		/* FPS display every second */
		uint64_t const time_since_fps_update = current_time - fps_update_time;
		if (time_since_fps_update > 1000) {
			current_fps = (float)frame_count * 1000.0f / (float)time_since_fps_update;
			(void)snprintf(title, sizeof(title), "X-Caliber FPS: %.2f", current_fps);
			SDL_SetWindowTitle(window, title);
			frame_count = 0;
			fps_update_time = current_time;
		}

		ctx.physics_accumulator += frame_time;

		/* process input */
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				ctx.running = false;
				break;
			}
			if (event.type == SDL_EVENT_KEY_DOWN) {
				SDL_Keycode const key = event.key.key;
				switch (key) {
				case SDLK_ESCAPE:
					ctx.running = false;
					break;
				case SDLK_Q:
					printf("Pressed Q!\n");
					break;
				case SDLK_F1:
					toggle_vsync();
					break;
				default:
					break;
				}
			}
		}

		/* fixed timestep physics and logic updates */
		while (ctx.physics_accumulator >= ctx.fixed_timestep) {
			game_logic_lib.update(&ctx);
			ctx.physics_accumulator -= ctx.fixed_timestep;
		}

		/* render as fast as possible with interpolation */
		ctx.alpha = ctx.physics_accumulator / ctx.fixed_timestep;
		game_logic_lib.render(&ctx);

		/* Copy my updated framebuffer to the GPU texture */
		SDL_UpdateTexture(ctx.texture, NULL, ctx.fb.pixels, (int)ctx.fb.pitch);
		SDL_RenderClear(ctx.renderer);
		SDL_RenderTexture(ctx.renderer, ctx.texture, NULL, NULL);
		SDL_RenderPresent(ctx.renderer);

		++frame_count;
	}
}

int
main(void)
{
	game_cfg_init();
	sdl_init();
	game_mem_init();
	game_ctx_init();

	if (!hot_reload_init(&game_logic_lib, GAME_LOGIC_LIB_NAME)) {
		panic("hot_reload_init",
		      "couldn't load game's logic shared library!");
	}

	run();

	quit();

	return EXIT_SUCCESS;
}
