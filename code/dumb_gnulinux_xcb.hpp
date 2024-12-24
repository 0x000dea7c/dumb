#pragma once

#include "dumb_platform.hpp"
#include <xcb/xcb.h>

#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080
#define DEFAULT_WINDOW_BORDER 10 // I think this is useless
#define DEFAULT_WINDOW_X_POS 10
#define DEFAULT_WINDOW_Y_POS 10
#define GAME_MODULE_LIB "./libdumb.so"

struct dumb_xcb_window
{
    I32 w;
    I32 h;
    I32 border;
    I16 x;
    I16 y;
};

using game_update_fn = U0 (*)();
using game_render_fn = U0 (*)(xcb_window_t const*, xcb_connection_t*, xcb_gcontext_t const*);

struct game_module
{
    U0* handle;
    game_render_fn render;
    game_update_fn update;
};

[[nodiscard]]
bool load_game_module(game_module* module, char const* path);
U0 unload_game_module(game_module* module);
