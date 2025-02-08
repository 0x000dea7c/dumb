#include "hyper_game_logic.h"

#include "hyper_renderer.h"

HYPER_API void
hyper_game_update (hyper_frame_context *context)
{
}

HYPER_API void
hyper_game_render (hyper_frame_context *context)
{
  hyper_draw (context->renderer_context);
}
