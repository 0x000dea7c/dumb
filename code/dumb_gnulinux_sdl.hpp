#pragma once

#include "dumb_platform.hpp"
#include <SDL2/SDL.h>

struct dumb_window
{
    SDL_Window* window;
    I32 width;
    I32 height;
    I32 flags;
    I32 x;
    I32 y;
    B32 fullscreen;
};

struct dumb_state
{
    B32 running;
};

struct dumb_renderer
{
    SDL_Renderer* renderer;
};

using update_fn = U0 (*)();
using render_fn = U0 (*)(SDL_Renderer*);

struct dumb_module
{
    U0* handle;
    render_fn render;
    update_fn update;
};

[[nodiscard]]
bool init_sdl();

U0 free_sdl();

U0 run(dumb_state& state);

[[nodiscard]]
bool load_game_module(dumb_module& module, char const* path);

U0 unload_game_module(dumb_module& module);
