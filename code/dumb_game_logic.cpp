// this file compiles into libdumb.so, which is needed for hot reloading

#include "dumb_platform.hpp"
#include <xcb/xproto.h>

static U32 mask = XCB_GC_FOREGROUND;
static U32 values[] = {0x000000};
static xcb_rectangle_t rect = {50, 50, 100, 100} // x, y, width, height
;

extern "C" U0 game_update()
{
}

extern "C" U0 game_render(xcb_window_t const* window, xcb_connection_t* conn, xcb_gcontext_t const* gctx)
{
    xcb_change_gc(conn, *gctx, mask, values);
    xcb_poly_fill_rectangle(conn, *window, *gctx, 1, &rect);
    xcb_flush(conn);
}
