// The three calls of app/include/app/pipeline_cache.h: the path joined under
// the settings folder, the file read and seeded, and the bytes written with
// their two folders made first. The reasoning is in the header.
#include <app/pipeline_cache.h>

#include <base/assert.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <stdint.h>
#include <stdio.h>

#define CACHE_FOLDER "voe3d"

const char *voe_app_pipeline_cache_path(voe_base_arena *arena, const char *name)
{
	const char *settings;
	const char *path;

	VOE_BASE_ASSERT(arena != NULL && name != NULL,
			"a cache path needs an arena and a name");
	settings = voe_platform_folder_settings(arena);
	if (settings == NULL)
		return NULL;
	path = voe_platform_path_join(
		arena, voe_platform_path_join(arena, settings, CACHE_FOLDER),
		name);
	VOE_BASE_ASSERT(path != NULL, "a join always answers");
	return path;
}

void voe_app_pipeline_cache_load(voe_render_device *device, const char *path,
				 voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark;
	const uint8_t *bytes;
	size_t size = 0;

	VOE_BASE_ASSERT(device != NULL && scratch != NULL,
			"a cache load needs a device and scratch");
	if (path == NULL || !voe_platform_file_exists(path))
		return;
	mark = voe_base_arena_mark(scratch);
	bytes = voe_platform_file_read(path, scratch, &size, NULL);
	if (bytes == NULL)
		fprintf(stderr, "app: %s did not read; pipelines are built again\n",
			path);
	else if (!voe_render_device_cache_seed(device, bytes, size))
		fprintf(stderr, "app: %s is not this build's; pipelines are built again\n",
			path);
	voe_base_arena_rewind(scratch, mark);
}

// path already there, or made one level. False only when neither is true.
static bool ensure_folder(const char *path, voe_base_arena *scratch)
{
	voe_platform_folder_listing listing;

	if (voe_platform_folder_list(path, scratch, &listing, NULL))
		return true;
	return voe_platform_folder_create(path, NULL);
}

// The file's parent and that one's parent, made as needed. A path with no
// parent is written where it is named.
static bool ensure_folders(const char *path, voe_base_arena *scratch)
{
	const char *parent = voe_platform_path_parent(scratch, path);
	const char *grandparent;

	if (parent == NULL)
		return true;
	grandparent = voe_platform_path_parent(scratch, parent);
	if (grandparent != NULL && !ensure_folder(grandparent, scratch))
		return false;
	return ensure_folder(parent, scratch);
}

void voe_app_pipeline_cache_save(voe_render_device *device, const char *path,
				 voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark;
	const void *bytes;
	size_t size = 0;

	VOE_BASE_ASSERT(device != NULL && scratch != NULL,
			"a cache save needs a device and scratch");
	if (path == NULL)
		return;
	mark = voe_base_arena_mark(scratch);
	bytes = voe_render_device_cache_bytes(device, scratch, &size);
	if (bytes == NULL)
		fprintf(stderr, "app: the driver handed out no pipeline cache\n");
	else if (!ensure_folders(path, scratch) ||
		 !voe_platform_file_write(path, bytes, size, NULL))
		fprintf(stderr, "app: the pipeline cache was not written to %s\n",
			path);
	voe_base_arena_rewind(scratch, mark);
}
