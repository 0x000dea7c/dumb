#pragma once

#include "dumb_platform.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
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
  long events;
};
