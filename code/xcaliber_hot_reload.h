#ifndef XCALIBER_HOT_RELOAD_H
#define XCALIBER_HOT_RELOAD_H

#include "xcaliber_stack_arena.h"
#include "xcaliber.h"

#include <stdbool.h>

/* function pointers that act as an interface between the main program and the shared lib */
typedef void (*update_func)(xc_context *);
typedef void (*render_func)(xc_context *, stack_arena *);

typedef struct xc_hot_reload_lib_info
{
  void *handle; /* handle to the dynamic lib */
  char const *path; /* path to the lib */
  update_func update; /* pointer to the update function */
  render_func render; /* pointer to the render function */
} xc_hot_reload_lib_info;

typedef struct xc_hot_reload_watcher
{
  int32_t inotify_fd; /* file descriptor for inotify's instance */
  int32_t watch_fd; /* watch descriptor for the shared lib */
  char const *libpath; /* path of the shared lib */
} xc_hot_reload_watcher;

/* initialise hot reloading system */
bool xc_hot_reload_init (xc_hot_reload_lib_info *, char const *);

/* check for changes in the shared library, returns true if there are, false if there isn't */
bool xc_hot_reload_lib_was_modified (void);

/*  update shared library, returns false if something went wrong */
bool xc_hot_reload_update (xc_hot_reload_lib_info *);

/* release resources */
void xc_hot_reload_quit (xc_hot_reload_lib_info *);

#endif
