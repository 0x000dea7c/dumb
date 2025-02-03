#ifndef XCALIBER_GEOMETRY_H
#define XCALIBER_GEOMETRY_H

#include "xcaliber_math.h"
#include "xcaliber_colour.h"

typedef struct xc_triangle
{
  xc_vec2i vertices[3];
} xc_triangle;

typedef struct xc_shaded_triangle
{
  xc_vec2i vertices[3];
  xc_colour colours[3];
} xc_shaded_triangle;

typedef struct xc_line
{
  xc_vec2i start;
  xc_vec2i end;
} xc_line;

typedef struct xc_quad
{
  /* bottom left */
  xc_vec2i position;
  int32_t width;
  int32_t height;
} xc_quad;

typedef struct xc_circle
{
  xc_vec2i center;
  int32_t radius;
} xc_circle;

#endif
