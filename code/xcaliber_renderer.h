#ifndef XCALIBER_RENDERER_H
#define XCALIBER_RENDERER_H

/* XCR stands for (x)-(c)aliber (r)enderer */

#include "xcaliber.h"
#include "xcaliber_colour.h"
#include "xcaliber_linear_arena.h"
#include "xcaliber_math.h"
#include <stdint.h>

typedef struct xcr_context xcr_context;

typedef struct xcr_triangle {
	xc_vec2i vertices[3];
} xcr_triangle;

/* FIXME: naming sucks */
typedef struct xcr_triangle_colours {
	xc_vec2i vertices[3];
	xc_colour colours[3];
} xcr_triangle_colours;

xcr_context *xcr_create(linear_arena *arena, xc_framebuffer *fb);

void xcr_set_bg_colour(xcr_context *ctx, uint32_t colour);

void xcr_draw_line(xcr_context *ctx, xc_vec2i p0, xc_vec2i p1, uint32_t colour);

/* Outline drawing */
void xcr_draw_quad_outline(xcr_context *ctx, xc_vec2i p, int32_t width, int32_t height, uint32_t colour);

void xcr_draw_triangle_outline(xcr_context *ctx, xcr_triangle triangle, uint32_t colour);

void xcr_draw_circle_outline(xcr_context *ctx, xc_vec2i center, int32_t r, uint32_t colour);

/* Filled drawing, only one colour */
void xcr_draw_quad_filled(xcr_context *ctx, xc_vec2i p0, int32_t width, int32_t height, uint32_t colour);

void xcr_draw_triangle_filled(xcr_context *ctx, stack_arena *a, xcr_triangle triangle, uint32_t colour);

void xcr_draw_circle_filled(xcr_context *ctx, xc_vec2i center, int32_t r, uint32_t colour);

/* Shaded drawing */
void xcr_draw_triangle_filled_colours(xcr_context *ctx, xcr_triangle_colours *triangle);

#endif
