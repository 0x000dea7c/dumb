#include "xcaliber_colour.h"

xc_colour const preset_colours[] = {
	[XC_BLACK] = { .r = 0x00, .g = 0x00, .b = 0x00, .a = 0x00 },
	[XC_WHITE] = { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF },
	[XC_RED] = { .r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF },
	[XC_GREEN] = { .r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF },
	[XC_BLUE] = { .r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF },
	[XC_PURPLE] = { .r = 0xA0, .g = 0x20, .b = 0xF0, .a = 0xFF },
};

uint32_t
xc_preset_colour(xc_colour_preset preset)
{
	xc_colour c = preset_colours[preset];
	return (uint32_t)(c.r) << 24 | (uint32_t)(c.g) << 16 |
	       (uint32_t)(c.b) << 8 | (uint32_t)(c.a);
}
