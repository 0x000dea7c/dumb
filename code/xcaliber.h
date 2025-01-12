#ifndef XCALIBER_H
#define XCALIBER_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t *pixels;
	uint32_t pixel_count;
	uint32_t byte_size;
	int32_t width;
	int32_t height;
	int32_t pitch; /* Or stride. Number of bytes between the start of one row of pixels to the next one. */
	uint64_t simd_chunks;
} framebuffer;

typedef struct {
	framebuffer fb;
	SDL_Texture *texture;
	SDL_Renderer *renderer;
	uint64_t last_frame_time;
	float fixed_timestep;
	float physics_accumulator;
	float alpha;
	bool running;
} game_ctx;

typedef struct {
	float target_fps;
	int32_t window_width;
	int32_t window_height;
	bool vsync;
} game_cfg;

void run(void);

#endif
