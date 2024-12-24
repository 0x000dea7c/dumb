#include "dumb_gnulinux_xcb.hpp"
#include "dumb_platform.hpp"
#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

I32 main()
{
    // 0. Initialise xcb window data
    dumb_xcb_window const win_dims = {DEFAULT_WINDOW_WIDTH,
                                      DEFAULT_WINDOW_HEIGHT,
                                      DEFAULT_WINDOW_BORDER,
                                      DEFAULT_WINDOW_X_POS,
                                      DEFAULT_WINDOW_Y_POS};

    // My game module that I will be using to do hot reloading
    game_module game_module = {nullptr, nullptr, nullptr};

    // 1. Open a connection to the X server
    xcb_connection_t* conn = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(conn) != 0)
    {
        (void)fprintf(stderr, "Cannot open connection with X server\n");
        return EXIT_FAILURE;
    }

    // 2. Get the first screen of the connection
    xcb_setup_t const* setup = xcb_get_setup(conn);
    xcb_screen_iterator_t const iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    // 3. Create the window
    xcb_window_t const window = xcb_generate_id(conn);
    U32 const mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK; // XCB_CW_BACK_PIXEL sets the background colour

    U32 values[3] = {
        screen->white_pixel,                               // BG colour
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS // Events I'm interested in
    };

    xcb_create_window(conn,
                      XCB_COPY_FROM_PARENT,
                      window,
                      screen->root,
                      win_dims.x,
                      win_dims.y,
                      win_dims.w,
                      win_dims.h,
                      win_dims.border,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      mask,
                      values);

    // 4. Show the created window bc it's unmapped (hidden) by default
    xcb_map_window(conn, window);

    // 5. Create the graphics context
    xcb_gcontext_t const gctx = xcb_generate_id(conn);
    U32 gctx_values[] = {
        screen->black_pixel,  // foreground colour (black)
        screen->white_pixel,  // background colour (white)
        1,                    // line width
        XCB_LINE_STYLE_SOLID, // line style
        XCB_CAP_STYLE_BUTT,   // cap style
        XCB_JOIN_STYLE_MITER  // join style
    };

    xcb_create_gc(conn,
                  gctx,
                  window,
                  XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_LINE_WIDTH | XCB_GC_LINE_STYLE | XCB_GC_CAP_STYLE |
                      XCB_GC_JOIN_STYLE,
                  gctx_values);

    xcb_key_symbols_t* keysyms = xcb_key_symbols_alloc(conn);
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(keysyms, XK_r); // TODO: grab all keys at once!
    if (keycodes == nullptr)
    {
        (void)fprintf(stderr, "I couldn't grab the keycode for R\n");
        return EXIT_FAILURE;
    }

    xcb_keycode_t const r_keycode = keycodes[0];
    free(keycodes);
    xcb_grab_key(conn, 1, window, XCB_MOD_MASK_ANY, r_keycode, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    if (!load_game_module(&game_module, GAME_MODULE_LIB))
    {
        return EXIT_FAILURE;
    }

    xcb_flush(conn);

    xcb_key_press_event_t* key_press = nullptr;
    xcb_generic_event_t* event = nullptr;

    // This is annoying, but apparently xcb mallocs an event per frame
    while ((event = xcb_wait_for_event(conn)) != nullptr)
    {
        switch (event->response_type & ~0x80)
        {
        case XCB_KEY_PRESS:
            key_press = (xcb_key_press_event_t*)event;
            if (key_press->detail == r_keycode)
            {
                unload_game_module(&game_module);
                if (!load_game_module(&game_module, GAME_MODULE_LIB))
                {
                    goto cleanup;
                }
            }
        default:
            break;
        }
        game_module.update();
        game_module.render(&window, conn, &gctx);
        free(event);
    }

cleanup:
    xcb_key_symbols_free(keysyms);
    xcb_ungrab_key(conn, r_keycode, window, XCB_MOD_MASK_ANY);
    xcb_free_gc(conn, gctx);
    xcb_disconnect(conn);
    return EXIT_SUCCESS;
}

bool load_game_module(game_module* module, char const* path)
{
    module->handle = dlopen(path, RTLD_LAZY);
    if (module->handle == nullptr)
    {
        (void)fprintf(stderr,
                      "%s - %s: I couldn't open my game module for hot reloading: %s\n",
                      __FILE__,
                      __PRETTY_FUNCTION__,
                      dlerror());
        return false;
    }

    *(U0**)(&module->render) = dlsym(module->handle, "game_render");
    *(U0**)(&module->update) = dlsym(module->handle, "game_update");

    if (module->render == nullptr || module->update == nullptr)
    {
        (void)fprintf(stderr,
                      "%s - %s: I couldn't find symbols for my game module: %s\n",
                      __FILE__,
                      __PRETTY_FUNCTION__,
                      dlerror());
        dlclose(module->handle);
        return false;
    }

    return true;
}

U0 unload_game_module(game_module* module)
{
    if (module->handle != nullptr)
    {
        dlclose(module->handle);
        module->handle = nullptr;
        module->render = nullptr;
        module->update = nullptr;
    }
}
