#include "xcaliber.h"
#include "xcaliber_linear_arena.h"
#include <SDL3/SDL_render.h>
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

static void panic_and_abort(char const *title, char const *msg)
	__attribute__((noreturn));

static void
panic_and_abort(char const *title, char const *msg)
{
	(void)fprintf(stderr, "%s - %s\n", title, msg);
	SDL_Quit();
	exit(EXIT_FAILURE);
}

void
put_pixel(framebuffer *f, size_t c, size_t r,
	  unsigned int colour) /* TODO: make colour a struct */
{
	/* formula to transform from 2D to the framebuffer: framebuffer's width * row(y) + col (x) */
	/* https://youtu.be/bQBY9BM9g_Y?t=2463 */
	f->pixels[f->width * r + c] = colour;
}

static void
update(void)
{
	/* TODO: remember to use fixed time step size, not variable */
	memset(fb.pixels, 0x00, fb.size);
}

static void
render(void)
{
	SDL_UpdateTexture(texture, NULL, fb.pixels, (int)fb.pitch);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void
run(void)
{
	SDL_Event event;

	while (state.running) {
		/* Handle keyboard and mouse input */
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				state.running = false;
				break;
			}
		}

		update();
		render();
	}
}

int
main(void)
{
	/* FIXME: wrap around func? SDL initialisation */
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		panic_and_abort("SDL_Init", SDL_GetError());
	}

	if (!SDL_CreateWindowAndRenderer("XCaliber", win_dims.width,
					 win_dims.height, 0, &window,
					 &renderer)) {
		panic_and_abort("SDL_CreateWindowAndRenderer", SDL_GetError());
	}

	SDL_SetRenderVSync(renderer, 1);

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
				    SDL_TEXTUREACCESS_STREAMING, win_dims.width,
				    win_dims.height);
	if (!texture) {
		panic_and_abort("SDL_CreateTexture", SDL_GetError());
	}

	/* initialisation of game memory */
	fb.width = (size_t)win_dims.width;
	fb.height = (size_t)win_dims.height;
	fb.pitch = fb.width * sizeof(uint32_t); /* FIXME: use fixed types */
	fb.size = fb.width * fb.height * sizeof(fb.width);
	unsigned char *buf = malloc(fb.size);
	linear_arena_init(&arena, buf, fb.size);

	fb.pixels = linear_arena_alloc(&arena, fb.size);
	assert(fb.pixels);

	/* run the app */
	run();

	/* cleanup */
	free(buf);
	SDL_Quit();

	return EXIT_SUCCESS;
}
