#pragma once

#include "dumb_platform.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

struct x_win_dims {
  I32 width;
  I32 height;
};

struct x_win_pos {
  I32 x;
  I32 y;
};

struct x_input_events {
  I64 events;
};

using game_render_fn = U0 (*)(Display *, Window, GC);
using game_update_fn = U0 (*)();

struct game_module {
  U0 *handle;
  game_render_fn render;
  game_update_fn update;
};

bool load_game_module(game_module& module, char const* path);
U0 unload_game_module(game_module& module);
