#pragma once

/* NOTE: this file will be compiled into a shared library and will be hot
   reloaded! Careful with what I put here, especially state related stuff.
   Also, if I change the name of the functions here, I also need to change
   the constants in hyper_common.h */

/* Symbols are public by default on GNU/Linux systems if I don't specify
   anything at compile time, but it's a good practice to compile shared
   libraries with hidden visiblity and then being explicit about which
   functions should be visible. What do I know anyway */

#include "hyper.h"

#define HYPER_API __attribute__((visibility("default")))

HYPER_API void hyper_game_update (hyper_frame_context *);
HYPER_API void hyper_game_render (hyper_frame_context *);
