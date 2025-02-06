#include "hyper_colour.h"

hyper_colour const preset_colours[] =
  {
    [HYPER_BLACK]  = { .r = 0x00, .g = 0x00, .b = 0x00, .a = 0x00 },
    [HYPER_WHITE]  = { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF },
    [HYPER_RED]    = { .r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF },
    [HYPER_GREEN]  = { .r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF },
    [HYPER_BLUE]   = { .r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF },
    [HYPER_PURPLE] = { .r = 0xA0, .g = 0x20, .b = 0xF0, .a = 0xFF },
    [HYPER_OLIVE]  = { .r = 0x80, .g = 0x80, .b = 0x00, .a = 0xFF }
  };

inline hyper_colour
hyper_get_colour_from_preset (hyper_colour_preset preset)
{
  return preset_colours[preset];
}

inline u32
hyper_get_colour_uint (hyper_colour colour)
{
  return (u32) (colour.r) << 24 | (u32) (colour.g) << 16 | (u32) (colour.b) << 8 | (u32) colour.a;
}
