#ifndef XCALIBER_GEOMETRY_H
#define XCALIBER_GEOMETRY_H

#include "xcaliber_math.h"
#include "xcaliber_colour.h"

/* NOTE: not adding colours or texture coordinates here because adding them per vertex
 * would be too much of an overhead. I think the common case is to draw triangles all
 * with the same colour. Anyway, if I want to specify colours per vertex I still can.
 * Just specify it on the draw command. */
typedef struct xc_vertex2d
{
  xc_vec2f position;
} xc_vertex2d;

typedef struct xc_triangle2d
{
  xc_vec2f vertices[3];
} xc_triangle2d;

typedef struct xc_line2d
{
  xc_vec2f start;
  xc_vec2f end;
} xc_line2d;

typedef struct xc_quad2d
{
  /* bottom left */
  xc_vec2f position;
  f32_t width;
  f32_t height;
} xc_quad2d;

typedef struct xc_circle2d
{
  xc_vec2f center;
  f32_t radius;
} xc_circle2d;

#endif
