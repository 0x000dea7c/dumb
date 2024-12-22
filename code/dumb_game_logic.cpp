// this file compiles into libdumb.so, which is needed for hot reloading

#include "dumb_gnulinux_x11.h"

extern "C" U0 game_render(Display *display, Window window, GC gc) {
  XClearWindow(display, window);
  XSetForeground(display, gc, 0x000000);
  XFillRectangle(display, window, gc, 100, 100, 200, 200);
}

extern "C" U0 game_update() {

}
