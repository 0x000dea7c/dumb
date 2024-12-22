#include "dumb_gnulinux_x11.h"
#include "dumb_platform.h"
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>

I32 main() {
  game_module game_module{nullptr, nullptr, nullptr};
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

  if (!load_game_module(game_module, "./libdumb.so")) {
    goto cleanup;
  }

  while (true) {
    XNextEvent(display, &event);

    if (event.type == KeyPress) {
      auto x_key = XLookupKeysym(&event.xkey, 0);
      if (x_key == XK_r) {
        unload_game_module(game_module);
        if (!load_game_module(game_module, "./libdumb.so")) {
          break;
        }
      }
    }

    game_module.update();
    game_module.render(display, window, graphics_ctx);
  }

  // NOTE: this is not really necessary in any modern OS, but leaving it for
  // completeness
cleanup:
  XFreeGC(display, graphics_ctx);
  XDestroyWindow(display, window);
  XCloseDisplay(display);
  unload_game_module(game_module);

  return EXIT_SUCCESS;
}

bool load_game_module(game_module &module, char const *path) {
  module.handle = dlopen(path, RTLD_LAZY);

  if (module.handle == nullptr) {
    (void)fprintf(
        stderr,
        "%s - %s: I couldn't open my game module for hot reloading: %s\n",
        __FILE__, __PRETTY_FUNCTION__, dlerror());
    return false;
  }

  *(U0 **)(&module.render) = dlsym(module.handle, "game_render");
  *(U0 **)(&module.update) = dlsym(module.handle, "game_update");

  if (module.render == nullptr || module.update == nullptr) {
    (void)fprintf(stderr,
                  "%s - %s: I couldn't find symbols for my game module: %s\n",
                  __FILE__, __PRETTY_FUNCTION__, dlerror());
    dlclose(module.handle);
    return false;
  }

  return true;
}

U0 unload_game_module(game_module &module) {
  if (module.handle != nullptr) {
    dlclose(module.handle);
    module.handle = nullptr;
    module.render = nullptr;
    module.update = nullptr;
  }
}
