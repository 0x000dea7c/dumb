#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct sdl_window_dimensions {
	int width;
	int height;
} sdl_window_dimensions;

typedef struct sdl_window {
	SDL_Window *window;
	SDL_Renderer *renderer;
} sdl_window;

typedef struct xcaliber_state {
	bool running;
} xcaliber_state;

void run(void);
