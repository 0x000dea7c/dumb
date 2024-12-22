#include "dumb_gnulinux_x11.h"
#include "dumb_platform.h"
#include <cstdio>
#include <cstdlib>

I32 main() {
  x_win_dims const x_dims{1920, 1080};
  x_win_pos const x_win_pos{10, 10};
  x_input_events const x_input_ev{ExposureMask | KeyPressMask};
  XEvent event;

  Display *display = XOpenDisplay(nullptr);
  if (display == nullptr) {
    (void)fprintf(stderr, "%s - %s: I couldn't open an X display.\n", __FILE__,
                  __PRETTY_FUNCTION__);
    return EXIT_FAILURE;
  }

  I32 screen = DefaultScreen(display);

  Window window =
      XCreateSimpleWindow(display, RootWindow(display, screen), x_win_pos.x,
                          x_win_pos.y, x_dims.width, x_dims.height,
                          1, // border width, not using it
                          BlackPixel(display, screen),  // border black
                          WhitePixel(display, screen)); // background white

  XSelectInput(display, window,
               x_input_ev.events); // select input events I'm interested in

  XMapWindow(display, window); // make the window visible

  GC graphics_ctx = XCreateGC(display, window, 0, nullptr);

  XSetForeground(display, graphics_ctx, WhitePixel(display, screen));
  XSetBackground(display, graphics_ctx, BlackPixel(display, screen));

  while (true) {
    XNextEvent(display, &event);

    if (event.type == Expose) {
      XClearWindow(display, window);
      XSetForeground(display, graphics_ctx, 0xFF0000);
      XFillRectangle(display, window, graphics_ctx, 100, 100, 200, 200);
    } else if (event.type == KeyPress) {
      break;
    }
  }

  // NOTE: this is not really necessary in any modern OS, but leaving it for completeness
  XFreeGC(display, graphics_ctx);
  XDestroyWindow(display, window);
  XCloseDisplay(display);

  return EXIT_SUCCESS;
}
