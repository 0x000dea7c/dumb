#include "xcaliber_colour.h"

xc_colour const preset_colours[] =
  {
    [XC_BLACK] = { .r = 0x00, .g = 0x00, .b = 0x00, .a = 0x00 },
    [XC_WHITE] = { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF },
    [XC_RED] = { .r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF },
    [XC_GREEN] = { .r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF },
    [XC_BLUE] = { .r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF },
    [XC_PURPLE] = { .r = 0xA0, .g = 0x20, .b = 0xF0, .a = 0xFF },
    [XC_OLIVE] = { .r = 0x80, .g = 0x80, .b = 0x00, .a = 0xFF }
  };

inline xc_colour
xc_preset_colour (xc_colour_preset preset)
{
  return preset_colours[preset];
}

inline uint32_t
xc_get_colour (xc_colour colour)
{
  return (uint32_t) (colour.r) << 24 | (uint32_t) (colour.g) << 16 | (uint32_t) (colour.b) << 8 | (uint32_t) colour.a;
}
