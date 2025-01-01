#pragma once

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct sdl_window_dimensions {
	int width;
	int height;
} sdl_window_dimensions;

typedef struct xcaliber_state {
	bool running;
} xcaliber_state;

typedef struct {
	unsigned int *pixels;
	size_t size;
	size_t width;
	size_t height;
	size_t pitch; /* or stride. Number of bytes between the start of one row of pixels to the next one. */
} framebuffer;

void run(void);
