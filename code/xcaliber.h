#ifndef XCALIBER_H
#define XCALIBER_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	int32_t width;
	int32_t height;
} sdl_window_dimensions;

typedef struct {
	bool running;
} xcaliber_state;

typedef struct {
	uint32_t *pixels;
	uint32_t pixel_count;
	uint32_t byte_size;
	uint32_t width;
	uint32_t height;
	uint32_t pitch; /* Or stride. Number of bytes between the start of one row of pixels to the next one. */
} framebuffer;

void run(void);

#endif
