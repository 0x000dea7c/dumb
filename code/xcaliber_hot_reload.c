#include "xcaliber_hot_reload.h"
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h> /* FIXME: abstract this? */
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

bool hot_reload_init(hot_reload_lib_info *lib, char const* path)
{
	assert(strlen(path) != 0);
	lib->path = path;
	lib->handle = NULL;
	lib->last_modified.tv_nsec = 0;
	lib->last_modified.tv_sec = 0;
	lib->render = NULL;
	lib->update = NULL;

	/* try to load the library */
	return hot_reload_update(lib);
}

/* check for changes and update if necessary */
bool hot_reload_update(hot_reload_lib_info *lib)
{
	struct stat attributes;

	if (stat(lib->path, &attributes) != 0) {
		(void)fprintf(stderr, "couldn't get status of lib %s: %s\n", lib->path, strerror(errno));
		return false;
	}

	/* no modification on lib, so it's updated */
	if (attributes.st_mtim.tv_nsec <= lib->last_modified.tv_nsec) {
		return true;
	}

	/* let's try to update */
	if (lib->handle) {
		/* if the lib was opened, close it first */
		dlclose(lib->handle);
		lib->handle = NULL;
		lib->render = NULL;
		lib->update = NULL;
	}

	lib->handle = dlopen(lib->path, RTLD_NOW); /* FIXME: what is RTLD_NOW? */
	if (!lib->handle) {
		(void)fprintf(stderr, "couldn't open lib %s: %s\n", lib->path, dlerror());
		return false;
	}

	/* look for symbols inside this library */
	lib->update = *(update_func*) dlsym(lib->handle, "game_update");
	lib->render = *(render_func*) dlsym(lib->handle, "game_render");

	if (!lib->update || !lib->render) {
		(void)fprintf(stderr, "couldn't find symbols for lib %s: %s\n", lib->path, dlerror());
		return false;
	}

	lib->last_modified = attributes.st_mtim;

	return true;
}

void hot_reload_quit(hot_reload_lib_info *lib)
{
	if (lib->handle) {
		dlclose(lib->handle);
	}
}
