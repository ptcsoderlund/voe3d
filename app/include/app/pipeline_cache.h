// The device's pipeline cache kept between runs, so a second start builds its
// pipelines faster (decision 0370 point 6). A program finds the path, loads it
// before the first prepare step, and saves once prepare answered PREPARED.
//
//     const char *path = voe_app_pipeline_cache_path(arena,
//                                                    "pipelines_editor.cache");
//     voe_app_pipeline_cache_load(device, path, scratch);
//     while (voe_render_device_prepare(device) == VOE_RENDER_PREPARING)
//             ;
//     ... PREPARED: voe_app_pipeline_cache_save(device, path, scratch);
//
// THE SETTINGS FOLDER, NOT BESIDE THE PROGRAM: it exists on every machine the
// engine runs on and is never tracked or shipped, where a shipped game's own
// folder may be read-only. Editor and game keep separate names, so two engine
// builds do not rewrite one file in turn.
//
// NO FAILURE HERE FAILS A START. A cache only saves time: no settings folder,
// no file, a file that will not read, bytes render refuses, a folder that will
// not be made or a write that fails are each at most one stderr line, and the
// pipelines are built from nothing. A NULL path is nothing to load or save.
//
// Constraints: load and save use scratch and keep nothing in it. The file is
// written through platform/file.h's atomic write, so a crash leaves the old one.
// Load only before the device's first prepare step (render asserts it); save
// only after PREPARED, on the thread that prepared or after it is joined.
#pragma once

#include <base/arena.h>
#include <render/device.h>

// `<settings>/voe3d/<name>`, pushed into arena; NULL with no settings folder.
const char *voe_app_pipeline_cache_path(voe_base_arena *arena,
					const char *name);

// The file at path handed to the device's cache. A missing file is silent; one
// render refuses is a stderr line saying the pipelines are built again.
void voe_app_pipeline_cache_load(voe_render_device *device, const char *path,
				 voe_base_arena *scratch);

// The device's cache written to path, its parent and that one's parent made
// first when they are not there. A failure is one stderr line.
void voe_app_pipeline_cache_save(voe_render_device *device, const char *path,
				 voe_base_arena *scratch);
