#ifndef XCALIBER_HOT_RELOAD_H
#define XCALIBER_HOT_RELOAD_H

#include <time.h>
#include <stdbool.h>

typedef void (*update_func)(float);
typedef void (*render_func)(void);

typedef struct {
	void *handle; /* handle to the dynamic lib */
	struct timespec last_modified; /* last time the lib was modified */
	char const *path; /* path to the lib */
	update_func update; /* pointer to the update function */
	render_func render; /* pointer to the render function */
} hot_reload_lib_info;

/* initialise hot reloading system */
bool hot_reload_init(hot_reload_lib_info *lib, char const *path);

/* check for changes and update if necessary; returns false if something went wrong */
bool hot_reload_update(hot_reload_lib_info *lib);

/* release resources */
void hot_reload_quit(hot_reload_lib_info *lib);

#endif
