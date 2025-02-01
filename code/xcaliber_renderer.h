#ifndef XCALIBER_RENDERER_H
#define XCALIBER_RENDERER_H

/* XCR stands for (x)-(c)aliber (r)enderer */

#include "xcaliber.h"
#include "xcaliber_colour.h"
#include "xcaliber_math.h"

#include <stdint.h>

typedef struct xcr_context xcr_context;
typedef struct linear_arena linear_arena;

typedef struct xcr_triangle
{
  xc_vec2i vertices[3];
} xcr_triangle;

typedef struct xcr_shaded_triangle
{
  xc_vec2i vertices[3];
  xc_colour colours[3];
} xcr_shaded_triangle;

xcr_context *xcr_create (linear_arena *, xc_framebuffer *);

void xcr_set_background_colour (xcr_context *, uint32_t);

void xcr_draw_line (xcr_context *, xc_vec2i, xc_vec2i, uint32_t);

void xcr_draw_quad_outline (xcr_context *, xc_vec2i, int32_t, int32_t, uint32_t);

void xcr_draw_triangle_outline (xcr_context *, xcr_triangle, uint32_t);

void xcr_draw_circle_outline (xcr_context *, xc_vec2i, int32_t, uint32_t);

void xcr_draw_quad_filled (xcr_context *, xc_vec2i, int32_t, int32_t, uint32_t);

void xcr_draw_triangle_filled (xcr_context *, stack_arena *, xcr_triangle, uint32_t);

void xcr_draw_circle_filled (xcr_context *, xc_vec2i, int32_t, uint32_t);

void xcr_draw_shaded_triangle_filled (xcr_context *, stack_arena *, xcr_shaded_triangle *);

#endif
