#ifndef XCALIBER_HOT_RELOAD_H
#define XCALIBER_HOT_RELOAD_H

#include <stdbool.h>
#include "xcaliber.h"

/* function pointers that act as an interface between the main program and the shared lib */
typedef void (*update_func)(game_ctx *ctx, float dt);
typedef void (*render_func)(game_ctx *ctx);

typedef struct {
	void *handle; /* handle to the dynamic lib */
	char const *path; /* path to the lib */
	update_func update; /* pointer to the update function */
	render_func render; /* pointer to the render function */
} hot_reload_lib_info;

typedef struct {
	int32_t inotify_fd; /* file descriptor for inotify's instance */
	int32_t watch_fd; /* watch descriptor for the shared lib */
	char const *libpath; /* path of the shared lib */
} hot_reload_watcher;

/* initialise hot reloading system */
bool hot_reload_init(hot_reload_lib_info *lib, char const *path);

/* check for changes in the shared library, returns true if there are, false if there isn't */
bool hot_reload_lib_was_modified(void);

/*  update shared library, returns false if something went wrong */
bool hot_reload_update(hot_reload_lib_info *lib);

/* release resources */
void hot_reload_quit(hot_reload_lib_info *lib);

#endif
