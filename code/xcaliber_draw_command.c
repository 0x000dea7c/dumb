#include "xcaliber_draw_command.h"

xc_draw_command
make_draw_command_line2d (xc_line2d line, xc_colour colour)
{
  xc_draw_command command = {
    .flags = DRAW_FLAGS_OUTLINE,
    .type = DRAW_TYPE_LINE,
    .base_colour = colour,
    .geometry = {
      .line = {
        .start = {
          .position = line.start
        },
        .end = {
          .position = line.end
        }
      }
    }
  };

  return command;
}
