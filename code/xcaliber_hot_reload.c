#include "xcaliber_hot_reload.h"
#include <assert.h>
#include <linux/limits.h>	/* NAME_MAX */
#include <stdio.h>
#include <errno.h>
#include <sys/inotify.h>
#include <dlfcn.h>
#include <unistd.h>

/* FIXME: this global here? */
hot_reload_watcher watcher;

union {
	void *obj_ptr;
	update_func func_ptr;
} upd_tmp;

union {
	void *obj_ptr;
	render_func func_ptr;
} rend_tmp;

static bool
hot_reload_watcher_init(char const *path)
{
	watcher.libpath = path;

	watcher.inotify_fd = inotify_init1(IN_NONBLOCK);
	if (watcher.inotify_fd == -1) {
		(void)fprintf(stderr, "couldn't initialise inotify: %s\n", strerror(errno));
		return false;
	}

	watcher.watch_fd = inotify_add_watch(watcher.inotify_fd, watcher.libpath, IN_ALL_EVENTS);
	if (watcher.watch_fd == -1) {
		close(watcher.inotify_fd);
		(void)fprintf(stderr, "couldn't add watch: %s\n", strerror(errno));
		return false;
	}

	return true;
}

static bool
hot_reload_restart_watcher(void)
{
	if (inotify_rm_watch(watcher.inotify_fd, watcher.watch_fd) == -1) {
		(void)fprintf(stderr, "couldn't remove watch: %s\n", strerror(errno));
		return false;
	}

	return hot_reload_watcher_init(watcher.libpath);
}

bool
hot_reload_init(hot_reload_lib_info *lib, char const *path)
{
	assert(strlen(path) != 0);

	lib->path = path;
	lib->handle = NULL;
	lib->render = NULL;
	lib->update = NULL;

	if (!hot_reload_watcher_init(path)) {
		return false;
	}

	/* try to load the library */
	return hot_reload_update(lib);
}

bool
hot_reload_lib_was_modified(void)
{
	char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
	bool modified = false;

	/* non blocking read, neat */
	while (read(watcher.inotify_fd, buf, sizeof(buf)) > 0) {
		struct inotify_event *ev = (struct inotify_event *)buf;
		if (ev->mask & (IN_OPEN | IN_ATTRIB)) {
			hot_reload_restart_watcher();
			modified = true;
			break;
		}
	}

	if (modified) {
		/* don't try to open the new library immediately because it might have not been fully created yet! */
		usleep(50000);
	}

	return modified;
}

bool
hot_reload_update(hot_reload_lib_info *lib)
{
	if (lib->handle) {
		/* if the lib was opened, close it first */
		dlclose(lib->handle);
		lib->handle = NULL;
		lib->render = NULL;
		lib->update = NULL;
	}

	/* RTLD_NOW: find all symbols immediately. I want to fail early */
	lib->handle = dlopen(lib->path, RTLD_NOW);
	if (!lib->handle) {
		(void)fprintf(stderr, "couldn't open lib %s: %s\n", lib->path,
			      dlerror());
		return false;
	}

	/* NOTE: this is to check for errors, read the man page */
	dlerror();

	/* look for symbols inside this library */
	upd_tmp.obj_ptr = dlsym(lib->handle, "game_update");
	char *err = dlerror();
	if (err) {
		(void)fprintf(stderr, "couldn't game_update symbol for lib %s: %s\n", lib->path, err);
		return false;
	}

	dlerror();

	rend_tmp.obj_ptr = dlsym(lib->handle, "game_render");
	err = dlerror();
	if (err) {
		(void)fprintf(stderr, "couldn't game_render symbol for lib %s: %s\n", lib->path, err);
		return false;
	}

	lib->update = upd_tmp.func_ptr;
	lib->render = rend_tmp.func_ptr;

	return true;
}

void
hot_reload_quit(hot_reload_lib_info *lib)
{
	if (lib->handle) {
		dlclose(lib->handle);
	}
	inotify_rm_watch(watcher.inotify_fd, watcher.watch_fd);
	close(watcher.inotify_fd);
	close(watcher.watch_fd);
}
