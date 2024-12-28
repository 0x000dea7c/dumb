#include "xcaliber.h"
#include "xcaliber_keycodes.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* TEMP */
static struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} colour;

static struct sdl_window_dimensions win_dims = { .width = 1920,
						 .height = 1080 };
static struct xcaliber_state state = { .running = true };
static struct sdl_window win;

static int
key_to_engine(int key)
{
	switch (key) {
	case SDLK_q:
		return XCALIBER_KEY_Q;
	case SDLK_w:
		return XCALIBER_KEY_W;
	case SDLK_e:
		return XCALIBER_KEY_E;
	case SDLK_r:
		return XCALIBER_KEY_R;
	case SDLK_t:
		return XCALIBER_KEY_T;
	case SDLK_y:
		return XCALIBER_KEY_Y;
	case SDLK_u:
		return XCALIBER_KEY_U;
	case SDLK_i:
		return XCALIBER_KEY_I;
	case SDLK_o:
		return XCALIBER_KEY_O;
	case SDLK_p:
		return XCALIBER_KEY_P;
	case SDLK_a:
		return XCALIBER_KEY_A;
	case SDLK_s:
		return XCALIBER_KEY_S;
	case SDLK_d:
		return XCALIBER_KEY_D;
	case SDLK_f:
		return XCALIBER_KEY_F;
	case SDLK_g:
		return XCALIBER_KEY_G;
	case SDLK_h:
		return XCALIBER_KEY_H;
	case SDLK_j:
		return XCALIBER_KEY_J;
	case SDLK_k:
		return XCALIBER_KEY_K;
	case SDLK_l:
		return XCALIBER_KEY_L;
	case SDLK_z:
		return XCALIBER_KEY_Z;
	case SDLK_x:
		return XCALIBER_KEY_X;
	case SDLK_c:
		return XCALIBER_KEY_C;
	case SDLK_v:
		return XCALIBER_KEY_V;
	case SDLK_b:
		return XCALIBER_KEY_B;
	case SDLK_n:
		return XCALIBER_KEY_N;
	case SDLK_m:
		return XCALIBER_KEY_M;
	case SDLK_1:
		return XCALIBER_KEY_1;
	case SDLK_2:
		return XCALIBER_KEY_2;
	case SDLK_3:
		return XCALIBER_KEY_3;
	case SDLK_4:
		return XCALIBER_KEY_4;
	case SDLK_5:
		return XCALIBER_KEY_5;
	case SDLK_6:
		return XCALIBER_KEY_6;
	case SDLK_7:
		return XCALIBER_KEY_7;
	case SDLK_8:
		return XCALIBER_KEY_8;
	case SDLK_9:
		return XCALIBER_KEY_9;
	case SDLK_ESCAPE:
		return XCALIBER_KEY_ESC;
	default:
		return XCALIBER_KEY_UNHANDLED;
	}
}

static void
handle_key_press(int key)
{
	switch (key) {
	case XCALIBER_KEY_ESC:
		state.running = false;
		break;
	case XCALIBER_KEY_R:
		colour.r = (colour.r + 1u) % 256u;
		break;
	case XCALIBER_KEY_G:
		colour.g = (colour.g + 1u) % 256u;
		break;
	case XCALIBER_KEY_B:
		colour.b = (colour.b + 1u) % 256u;
		break;
	case XCALIBER_KEY_A:
		colour.a = (colour.a + 1u) % 256u;
		break;
	default:
		break;
	}
}

void
run(void)
{
	while (state.running) {
		SDL_Event event;

		/* Handle keyboard and mouse input */
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				state.running = false;
				break;
			}
			if (event.type == SDL_KEYDOWN ||
			    event.type == SDL_KEYUP) {
				int key = key_to_engine(event.key.keysym.sym);
				bool pressed = event.type == SDL_KEYDOWN;

				if (pressed) {
					handle_key_press(key);
				}
			}
		}

		/* TODO: update. Don't need delta time because I'm using a fixed time step. */

		/* Render */
		SDL_SetRenderDrawColor(win.renderer, colour.r, colour.g,
				       colour.b, colour.a);
		SDL_RenderClear(win.renderer);
		SDL_RenderPresent(win.renderer);
	}
}

int
main(void)
{
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;

	colour.r = 0xFF;
	colour.b = 0x00;
	colour.b = 0x00;
	colour.a = 0xFF;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		(void)fprintf(stderr, "Couldn't initialise SDL: %s\n",
			      SDL_GetError());
		return EXIT_FAILURE;
	}

	window = SDL_CreateWindow("XCaliber", SDL_WINDOWPOS_CENTERED,
			 SDL_WINDOWPOS_CENTERED, win_dims.width,
			 win_dims.height, SDL_WINDOW_SHOWN);

	if (!window) {
		(void)fprintf(stderr, "Couldn't create window: %s\n",
			      SDL_GetError());
		goto cleanup;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	if (!renderer) {
		(void)fprintf(stderr, "Couldn't create renderer: %s\n",
			      SDL_GetError());
		goto cleanup;
	}

	win.window = window;
	win.renderer = renderer;

	run();

cleanup:
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
