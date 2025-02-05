#ifndef XCALIBER_DRAW_COMMAND_H
#define XCALIBER_DRAW_COMMAND_H

#include "xcaliber_geometry.h"
#include "xcaliber_colour.h"

#include <stdbool.h>

typedef enum
{
  DRAW_FLAGS_FILLED  = 1 << 0,
  DRAW_FLAGS_OUTLINE = 1 << 1,
} xc_draw_flags;

typedef enum
{
  DRAW_TYPE_TRIANGLE,
  DRAW_TYPE_LINE,
  DRAW_TYPE_QUAD,
  DRAW_TYPE_CIRCLE,
} xc_draw_type;

typedef struct
{
  xc_draw_flags flags;
  xc_draw_type type;
  xc_colour base_colour;

  union
  {
    struct
    {
      xc_vertex2d vertices[3];

      struct
      {
        xc_colour colours[3];
        bool per_vertex_colouring;
      } colouring;

    } triangle;

    struct
    {
      xc_vertex2d start;
      xc_vertex2d end;
    } line;

    struct
    {
      xc_vertex2d position;
      f32_t width;
      f32_t height;
    } quad;

    struct
    {
      xc_vertex2d center;
      f32_t radius;
    } circle;

  } geometry;

} xc_draw_command;

xc_draw_command make_draw_command_line2d (xc_line2d, xc_colour);

xc_draw_command make_draw_command_quad2d_filled (xc_quad2d, xc_colour);

xc_draw_command make_draw_command_quad2d_outline (xc_quad2d, xc_colour);

xc_draw_command make_draw_command_circle2d_filled (xc_circle2d, xc_colour);

xc_draw_command make_draw_command_circle2d_outline (xc_circle2d, xc_colour);

xc_draw_command make_draw_command_triangle2d_outline (xc_triangle2d, xc_colour);

xc_draw_command make_draw_command_triangle2d_filled (xc_triangle2d, xc_colour);

#endif
