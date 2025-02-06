#pragma once

#include "hyper_common.h"

typedef struct
{
  u8 r;
  u8 g;
  u8 b;
  u8 a;
} hyper_colour;

typedef enum
{
  HYPER_BLACK = 0,
  HYPER_WHITE,
  HYPER_RED,
  HYPER_GREEN,
  HYPER_BLUE,
  HYPER_PURPLE,
  HYPER_OLIVE,

  HYPER_COLOUR_COUNT
} hyper_colour_preset;

hyper_colour hyper_get_colour_from_preset (hyper_colour_preset);

u32 hyper_get_colour_uint (hyper_colour);
