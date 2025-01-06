#ifndef XCALIBER_GAME_LOGIC_H
#define XCALIBER_GAME_LOGIC_H

/* NOTE: this file will be compiled into a shared library and will be hot
   reloaded! Careful with what I put here, especially state related stuff. */

/* Symbols are public by default on GNU/Linux systems if I don't specify
   anything at compile time, but it's a good practice to compile shared
   libraries with hidden visiblity and then being explicit about which
   functions should be visible or not. */

#define GAME_API __attribute__((visibility("default")))

GAME_API void game_update(float dt);
GAME_API void game_render(void);

#endif
