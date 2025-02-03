#ifndef XCALIBER_COLOUR_H
#define XCALIBER_COLOUR_H

#include <stdint.h>

typedef struct xc_colour
{
  uint8_t r, g, b, a;
} xc_colour;

typedef enum xc_colour_preset
{
  XC_BLACK = 0,
  XC_WHITE,
  XC_RED,
  XC_GREEN,
  XC_BLUE,
  XC_PURPLE,
  XC_OLIVE,

  XC_COUNT
} xc_colour_preset;

uint32_t xc_preset_colour (xc_colour_preset);

uint32_t xc_get_colour (xc_colour);

#endif
