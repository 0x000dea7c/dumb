#pragma once

#include <stdbool.h>

#include "hyper.h"

typedef void (*update_function_ptr)(hyper_frame_context *);
typedef void (*render_function_ptr)(hyper_frame_context *);

typedef struct
{
  void *handle;
  char const *path;
  update_function_ptr update;
  render_function_ptr render;
} hyper_hot_reload_library_data;

typedef struct
{
  int32_t inotify_fd; /* file descriptor for inotify's instance */
  int32_t watch_fd; /* watch descriptor for the shared library */
  char const *path;
} hyper_hot_reload_watcher;

bool hyper_hot_reload_init (hyper_hot_reload_library_data *, char const *);

bool hyper_hot_reload_library_was_updated (void);

bool hyper_hot_reload_load (hyper_hot_reload_library_data *);

void hyper_hot_reload_quit (hyper_hot_reload_library_data *);
