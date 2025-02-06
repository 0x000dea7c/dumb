#pragma once

#include "hyper.h"
#include "hyper_colour.h"
#include "hyper_draw_command.h"

void hyper_set_background_colour (hyper_renderer_context *, hyper_colour);

void hyper_draw (hyper_renderer_context *, hyper_draw_command *);
