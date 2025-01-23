#include "xcaliber.h"
#include "xcaliber_common.h"
#include "xcaliber_hot_reload.h"
#include "xcaliber_linear_arena.h"
#include "xcaliber_stack_arena.h"
#include "xcaliber_renderer.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define XC_GAME_TOTAL_MEMORY XC_MEGABYTES(256ull)
#define XC_SCRATCH_MEMORY XC_MEGABYTES(32ull)
#define FIXED_TIMESTEP 1.0f / 60.0f
#define GAME_LOGIC_LIB_NAME "libgamelogic.so"

/* SDL needed globals */
static SDL_Window *sdl_window = NULL;
static SDL_Texture *sdl_texture = NULL;
static SDL_Renderer *sdl_renderer = NULL;

/* my needed globals, hehe */
static unsigned char *game_mem = NULL;
static unsigned char *game_scratch_mem = NULL;
static linear_arena *lin_arena = NULL;
static stack_arena *scratch_arena = NULL;
static xc_hot_reload_lib_info game_logic_lib;
static xc_ctx ctx;
static xc_cfg cfg;
static xc_framebuffer fb;

static void
quit(void)
{
	if (sdl_texture) {
		SDL_DestroyTexture(sdl_texture);
	}

	if (sdl_renderer) {
		SDL_DestroyRenderer(sdl_renderer);
	}

	if (sdl_window) {
		SDL_DestroyWindow(sdl_window);
	}

	if (game_mem) {
		free(game_mem);
	}

	if (game_scratch_mem) {
		free(game_scratch_mem);
	}

	if (lin_arena) {
		linear_arena_destroy(lin_arena);
	}

	if (scratch_arena) {
		stack_arena_destroy(scratch_arena);
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

	sdl_window = SDL_CreateWindow("X-Caliber", (int32_t)cfg.width,
				  (int32_t)cfg.height, 0);
	if (!sdl_window) {
		panic("SDL_CreateWindow", SDL_GetError());
	}

	sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
	if (!sdl_renderer) {
		panic("SDL_CreateRenderer", SDL_GetError());
	}

	/* my framebuffer */
	sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_STREAMING,
					(int32_t)cfg.width,
					(int32_t)cfg.height);
	if (!sdl_texture) {
		panic("SDL_CreateTexture", SDL_GetError());
	}
}

static void
cfg_init(void)
{
	cfg.width = 1024;
	cfg.height = 768;
	cfg.target_fps = FIXED_TIMESTEP;
	cfg.vsync = false;
}

static void
mem_init(void)
{
	lin_arena = linear_arena_create();
	assert(lin_arena);

	scratch_arena = stack_arena_create();
	assert(scratch_arena);

	game_mem = malloc(XC_GAME_TOTAL_MEMORY);
	assert(game_mem);

	game_scratch_mem = malloc(XC_SCRATCH_MEMORY);
	assert(game_scratch_mem);

	linear_arena_init(lin_arena, game_mem, XC_GAME_TOTAL_MEMORY);

	stack_arena_init(scratch_arena, game_scratch_mem, XC_SCRATCH_MEMORY);
}

static void
ctx_init(void)
{
	ctx.renderer_ctx = NULL;
	ctx.physics_accumulator = 0.0f;
	ctx.fixed_timestep = FIXED_TIMESTEP;
	ctx.alpha = 0.0f;
	ctx.running = true;
	ctx.last_frame_time = SDL_GetTicks();

	if (cfg.vsync) {
		SDL_SetRenderVSync(sdl_renderer, 1);
	}
}

static void
toggle_vsync(void)
{
	SDL_SetRenderVSync(sdl_renderer, cfg.vsync ? 0 : 1);
	cfg.vsync = !cfg.vsync;
	printf("VSync value changed to: %d\n", cfg.vsync);
}

static void
fb_init(void)
{
	fb.width = cfg.width;
	fb.height = cfg.height;
	fb.pitch = fb.width * (int32_t)sizeof(fb.width);
	fb.pixel_count = (uint32_t)fb.width * (uint32_t)fb.height;
	fb.byte_size = fb.pixel_count * sizeof(uint32_t);
#if defined(__AVX2__)
	fb.simd_chunks = (int32_t)(fb.pixel_count / 8);
#elif defined(__SSE4_2__)
	fb.simd_chunks = (int32_t)(fb.pixel_count / 4);
#else
	#error "engine needs at least SSE4.2 support"
#endif
	fb.pixels = linear_arena_alloc(lin_arena, fb.byte_size);
	if (!fb.pixels) {
		panic("framebuffer init", "Couldn't allocate space for the framebuffer");
	}
}

static void
renderer_init(void)
{
	ctx.renderer_ctx = xcr_create(lin_arena, &fb);
	if (!ctx.renderer_ctx) {
		panic("renderer_init", "Couldn't create renderer");
	}
}

static void
hot_reload_init(void)
{
	if (!xc_hot_reload_init(&game_logic_lib, GAME_LOGIC_LIB_NAME)) {
		panic("hot_reload_init", "couldn't load game's logic shared library!");
	}
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
		if (xc_hot_reload_lib_was_modified()) {
			xc_hot_reload_update(&game_logic_lib);
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
		uint64_t const time_since_fps_update =
			current_time - fps_update_time;
		if (time_since_fps_update > 1000) {
			current_fps = (float)frame_count * 1000.0f /
				      (float)time_since_fps_update;
			(void)snprintf(title, sizeof(title),
				       "X-Caliber FPS: %.2f", current_fps);
			SDL_SetWindowTitle(sdl_window, title);
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
		game_logic_lib.render(&ctx, scratch_arena);

		/* Copy my updated framebuffer to the GPU texture */
		SDL_UpdateTexture(sdl_texture, NULL, fb.pixels, fb.pitch);

		/* go render brr */
		SDL_RenderClear(sdl_renderer);
		SDL_RenderTexture(sdl_renderer, sdl_texture, NULL, NULL);
		SDL_RenderPresent(sdl_renderer);

		++frame_count;

		stack_arena_free_all(scratch_arena);
	}
}

int
main(void)
{
	/* if any of these fails, program panics */
	cfg_init();
	sdl_init();
	mem_init();
	ctx_init();
	fb_init();
	renderer_init();
	hot_reload_init();

	run();

	quit();

	return EXIT_SUCCESS;
}
