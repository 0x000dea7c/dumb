#ifndef XCALIBER_RENDERER_H
#define XCALIBER_RENDERER_H

/* XCR stands for (x)-(c)aliber (r)enderer */

#include "xcaliber.h"
#include "xcaliber_colour.h"
#include "xcaliber_geometry.h"
#include "xcaliber_draw_command.h"

typedef struct xcr_context  xcr_context;
typedef struct linear_arena linear_arena;

xcr_context *xcr_create (linear_arena *, xc_framebuffer *);

void xcr_set_background_colour (xcr_context *, xc_colour);

void xcr_draw (xcr_context *, stack_arena *, xc_draw_command *);

/* void xcr_draw_quad_outline (xcr_context *, xc_quad2d, xc_colour); */

/* void xcr_draw_triangle_outline (xcr_context *, xc_triangle2d, xc_colour); */

/* void xcr_draw_circle_outline (xcr_context *, xc_circle2d, xc_colour); */

/* void xcr_draw_quad_filled (xcr_context *, xc_quad2d, xc_colour); */

/* void xcr_draw_triangle_filled (xcr_context *, stack_arena *, xc_triangle2d, xc_colour); */

/* void xcr_draw_circle_filled (xcr_context *, xc_circle2d, xc_colour); */

/* void xcr_draw_shaded_triangle_filled (xcr_context *, stack_arena *, xc_shaded_triangle *); */

#endif
