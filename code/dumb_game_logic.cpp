// this file compiles into libdumb.so, which is needed for hot reloading

#include "dumb_platform.hpp"
#include <SDL2/SDL.h>

extern "C" U0 dumb_update()
{
}

extern "C" U0 dumb_render(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0xff, 0x00, 0xFF);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}
