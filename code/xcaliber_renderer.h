#ifndef XCALIBER_RENDERER_H
#define XCALIBER_RENDERER_H

/* XCR stands for (x)-(c)aliber (r)enderer */

#include "xcaliber.h"
#include "xcaliber_linear_arena.h"

typedef struct xcr_context xcr_context;

typedef struct xcr_point {
	int32_t x;
	int32_t y;
} xcr_point;

typedef struct xcr_colour {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} xcr_colour;

xcr_context *xcr_create(linear_arena *arena, xc_framebuffer *fb);

void xcr_set_bg_colour(xcr_context *ctx, xcr_colour colour);

void xcr_draw_line(xcr_context *ctx, xcr_point p0, xcr_point p1,
		   xcr_colour colour);

/* Outline drawing */
void xcr_draw_quad_outline(xcr_context *ctx, xcr_point p, int32_t width,
			   int32_t height, xcr_colour colour);

void xcr_draw_triangle_outline(xcr_context *ctx, xcr_point p0, xcr_point p1,
			       xcr_point p2, xcr_colour colour);

void xcr_draw_circle_outline(xcr_context *ctx, xcr_point center, int32_t r,
			     xcr_colour colour);

/* Filled drawing */
void xcr_draw_quad_filled(xcr_context *ctx, xcr_point p0, int32_t width, int32_t height,
			  xcr_colour colour);

#endif
