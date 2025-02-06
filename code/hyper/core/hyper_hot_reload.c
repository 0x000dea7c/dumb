#include "hyper_hot_reload.h"

#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/inotify.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

static hyper_hot_reload_watcher watcher;

/* this hack is needed for calling dylsym */
union
{
  void *obj_ptr;
  update_function_ptr func_ptr;
} upd_tmp;

union
{
  void *obj_ptr;
  render_function_ptr func_ptr;
} rend_tmp;

static bool
watcher_init (char const *path)
{
  watcher.path = path;

  watcher.inotify_fd = inotify_init1 (IN_NONBLOCK);
  if (watcher.inotify_fd == -1)
    {
      (void) fprintf (stderr, "couldn't initialise inotify: %s\n", strerror (errno));
      return false;
    }

  watcher.watch_fd = inotify_add_watch (watcher.inotify_fd, watcher.path, IN_ALL_EVENTS);
  if (watcher.watch_fd == -1)
    {
      close (watcher.inotify_fd);
      (void) fprintf (stderr, "couldn't add watch: %s\n", strerror (errno));
      return false;
    }

  return true;
}

static bool
restart_watcher (void)
{
  if (inotify_rm_watch (watcher.inotify_fd, watcher.watch_fd) == -1)
    {
      (void) fprintf (stderr, "couldn't remove watch: %s\n", strerror (errno));
      return false;
    }

  return watcher_init (watcher.path);
}

bool
hyper_hot_reload_init (hyper_hot_reload_library_data *library, char const *path)
{
  library->path = path;
  library->handle = NULL;
  library->render = NULL;
  library->update = NULL;

  if (!watcher_init (path))
    {
      return false;
    }

  return hyper_hot_reload_load (library);
}

bool
hyper_hot_reload_library_was_updated (void)
{
  char buffer[sizeof (struct inotify_event) + NAME_MAX + 1];
  bool modified = false;

  /* non blocking read, neat */
  while (read (watcher.inotify_fd, buffer, sizeof (buffer)) > 0)
    {
      struct inotify_event *event = (struct inotify_event *) buffer;
      if (event->mask & (IN_OPEN | IN_ATTRIB))
        {
          restart_watcher ();
          modified = true;
          break;
        }
    }

  if (modified)
    {
      /* XXX: don't try to open the new library immediately because it might have not been fully created yet! */
      usleep (50000);
    }

  return modified;
}

bool
hyper_hot_reload_load (hyper_hot_reload_library_data *library)
{
  if (library->handle)
    {
      dlclose (library->handle);
      library->handle = NULL;
      library->render = NULL;
      library->update = NULL;
    }

  /* RTLD_NOW: find all symbols immediately. */
  library->handle = dlopen (library->path, RTLD_NOW);
  if (!library->handle)
    {
      (void) fprintf (stderr, "couldn't open lib %s: %s\n", library->path, dlerror ());
      return false;
    }

  /* NOTE: this is to check for errors, read the man page */
  dlerror ();

  /* look for symbols inside this library */
  upd_tmp.obj_ptr = dlsym (library->handle, HYPER_GAME_UPDATE_FUNCTION_NAME);
  char *err = dlerror ();
  if (err)
    {
      (void) fprintf (stderr, "couldn't game_update symbol for lib %s: %s\n", library->path, err);
      return false;
    }

  dlerror ();

  rend_tmp.obj_ptr = dlsym (library->handle, HYPER_GAME_RENDER_FUNCTION_NAME);
  err = dlerror ();
  if (err)
    {
      (void) fprintf (stderr, "couldn't game_render symbol for lib %s: %s\n", library->path, err);
      return false;
    }

  library->update = upd_tmp.func_ptr;
  library->render = rend_tmp.func_ptr;

  return true;
}

void
hyper_hot_reload_quit (hyper_hot_reload_library_data *library)
{
  if (library->handle)
    {
      dlclose (library->handle);
    }

  inotify_rm_watch (watcher.inotify_fd, watcher.watch_fd);
  close (watcher.inotify_fd);
  close (watcher.watch_fd);
}
