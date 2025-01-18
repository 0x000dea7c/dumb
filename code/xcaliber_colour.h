#ifndef XCALIBER_COLOUR_H
#define XCALIBER_COLOUR_H

#include <stdint.h>

typedef struct xc_colour {
	uint8_t r, g, b, a;
} xc_colour;

/* I have these presets here to choose from */
typedef enum xc_colour_preset {
	XC_BLACK = 0,
	XC_WHITE,
	XC_RED,
	XC_GREEN,
	XC_BLUE,
	XC_PURPLE,

	XC_COUNT
} xc_colour_preset;

/* But if I'm feeling fancy, I can create some at compile time */
#define XC_RGBA(r, g, b, a) \
	((xc_colour){ .r = (r), .g = (g), .b = (b), .a = (a) })

#define XC_RGB(r, g, b) XC_RGBA(r, g, b), 0xFF)

uint32_t xc_preset_colour(xc_colour_preset preset);

#endif
