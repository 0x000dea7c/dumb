#include "dumb_gnulinux_sdl.hpp"
#include "dumb_keycodes.hpp"
#include "dumb_platform.hpp"
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

#define GAME_MODULE_LIB "./libdumb_game_logic.so"

global dumb_window window;
global dumb_state state;
global dumb_renderer renderer;
global dumb_module module;

internal U0 free_window()
{
    SDL_DestroyWindow(window.window);
}

internal U0 free_renderer()
{
    SDL_DestroyRenderer(renderer.renderer);
}

internal key_code SDL_key_to_dumb(I32 key)
{
    switch (key)
    {
    case SDLK_q:
        return key_code::DUMB_KEY_Q;
    case SDLK_w:
        return key_code::DUMB_KEY_W;
    case SDLK_e:
        return key_code::DUMB_KEY_E;
    case SDLK_r:
        return key_code::DUMB_KEY_R;
    case SDLK_t:
        return key_code::DUMB_KEY_T;
    case SDLK_y:
        return key_code::DUMB_KEY_Y;
    case SDLK_u:
        return key_code::DUMB_KEY_U;
    case SDLK_i:
        return key_code::DUMB_KEY_I;
    case SDLK_o:
        return key_code::DUMB_KEY_O;
    case SDLK_p:
        return key_code::DUMB_KEY_P;
    case SDLK_a:
        return key_code::DUMB_KEY_A;
    case SDLK_s:
        return key_code::DUMB_KEY_S;
    case SDLK_d:
        return key_code::DUMB_KEY_D;
    case SDLK_f:
        return key_code::DUMB_KEY_F;
    case SDLK_g:
        return key_code::DUMB_KEY_G;
    case SDLK_h:
        return key_code::DUMB_KEY_H;
    case SDLK_j:
        return key_code::DUMB_KEY_J;
    case SDLK_k:
        return key_code::DUMB_KEY_K;
    case SDLK_l:
        return key_code::DUMB_KEY_L;
    case SDLK_z:
        return key_code::DUMB_KEY_Z;
    case SDLK_x:
        return key_code::DUMB_KEY_X;
    case SDLK_c:
        return key_code::DUMB_KEY_C;
    case SDLK_v:
        return key_code::DUMB_KEY_V;
    case SDLK_b:
        return key_code::DUMB_KEY_B;
    case SDLK_n:
        return key_code::DUMB_KEY_N;
    case SDLK_m:
        return key_code::DUMB_KEY_M;
    case SDLK_1:
        return key_code::DUMB_KEY_1;
    case SDLK_2:
        return key_code::DUMB_KEY_2;
    case SDLK_3:
        return key_code::DUMB_KEY_3;
    case SDLK_4:
        return key_code::DUMB_KEY_4;
    case SDLK_5:
        return key_code::DUMB_KEY_5;
    case SDLK_6:
        return key_code::DUMB_KEY_6;
    case SDLK_7:
        return key_code::DUMB_KEY_7;
    case SDLK_8:
        return key_code::DUMB_KEY_8;
    case SDLK_9:
        return key_code::DUMB_KEY_9;
    case SDLK_ESCAPE:
        return key_code::DUMB_KEY_ESC;
    default:
        return key_code::DUMB_KEY_9; // TEMP
    }
}

bool init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        (void)fprintf(stderr, "%s: I couldn't initialise SDL2: %s\n", __PRETTY_FUNCTION__, SDL_GetError());
        return false;
    }

    return true;
}

U0 free_sdl()
{
    SDL_Quit();
}

U0 run(dumb_state& state)
{
    while (state.running != 0)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event) != 0)
        {
            bool user_forces_shutdown = (event.type == SDL_QUIT);
            if (user_forces_shutdown)
            {
                state.running = 0;
                break;
            }

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                key_code key_code = SDL_key_to_dumb(event.key.keysym.sym);
                bool pressed = (event.type == SDL_KEYDOWN);
                if (pressed && key_code == DUMB_KEY_ESC)
                {
                    state.running = 0;
                    break;
                }
                else if (pressed && key_code == DUMB_KEY_R)
                {
                    unload_game_module(module);
                    if (!load_game_module(module, GAME_MODULE_LIB))
                    {
                        break;
                    }
                }
            }
        }

        module.update();
        module.render(renderer.renderer);
    }
}

bool load_game_module(dumb_module& module, char const* path)
{
    module.handle = dlopen(path, RTLD_LAZY);
    if (module.handle == nullptr)
    {
        (void)fprintf(stderr,
                      "%s - %s: I couldn't open my game module for hot reloading: %s\n",
                      __FILE__,
                      __PRETTY_FUNCTION__,
                      dlerror());
        return false;
    }

    *(U0**)(&module.render) = dlsym(module.handle, "dumb_render");
    *(U0**)(&module.update) = dlsym(module.handle, "dumb_update");

    if (module.render == nullptr || module.update == nullptr)
    {
        (void)fprintf(stderr,
                      "%s - %s: I couldn't find symbols for my game module: %s\n",
                      __FILE__,
                      __PRETTY_FUNCTION__,
                      dlerror());
        dlclose(module.handle);
        return false;
    }

    return true;
}

U0 unload_game_module(dumb_module& module)
{
    if (module.handle != nullptr)
    {
        dlclose(module.handle);
        module.handle = nullptr;
        module.render = nullptr;
        module.update = nullptr;
    }
}

I32 main()
{
    if (!init_sdl())
    {
        return EXIT_FAILURE;
    }

    window.flags = 0;
    window.fullscreen = 0;
    window.width = 1920;
    window.height = 1080;
    window.x = SDL_WINDOWPOS_CENTERED;
    window.y = SDL_WINDOWPOS_CENTERED;
    window.window = SDL_CreateWindow("dumb", window.x, window.y, window.width, window.height, window.flags);
    if (window.window == nullptr)
    {
        (void)fprintf(stderr, "%s: I couldn't create SDL2 window: %s\n", __PRETTY_FUNCTION__, SDL_GetError());
        goto cleanup;
    }

    renderer.renderer = SDL_CreateRenderer(window.window, -1, 0);
    if (renderer.renderer == nullptr)
    {
        (void)fprintf(stderr, "%s: I couldn't create SDL2 renderer: %s\n", __PRETTY_FUNCTION__, SDL_GetError());
        goto cleanup;
    }

    if (!load_game_module(module, GAME_MODULE_LIB))
    {
        return EXIT_FAILURE;
    }

    state.running = 1;

    run(state);

cleanup:
    free_renderer();
    free_window();
    free_sdl();
    unload_game_module(module);

    return EXIT_SUCCESS;
}
