#include "hyper_game_logic.h"

#include "hyper_renderer.h"

HYPER_API void
hyper_game_update (hyper_frame_context *context)
{

}

HYPER_API void
hyper_game_render (hyper_frame_context *context)
{
  hyper_set_background_colour (context->renderer_context, hyper_get_colour_from_preset (HYPER_BLACK));
}
