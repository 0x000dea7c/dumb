#pragma once

#include "hyper_colour.h"

#include <stdbool.h>

typedef enum
{
  HYPER_DRAW_FLAGS_FILLED  = 1 << 0,
  HYPER_DRAW_FLAGS_OUTLINE = 1 << 1,
} hyper_draw_flags;

typedef enum
{
  HYPER_DRAW_TYPE_TRIANGLE,
  HYPER_DRAW_TYPE_LINE,
  HYPER_DRAW_TYPE_QUAD,
  HYPER_DRAW_TYPE_CIRCLE,
} hyper_draw_type;

typedef struct
{
  hyper_draw_flags flags;
  hyper_draw_type type;
  hyper_colour base_colour;
} hyper_draw_command;
