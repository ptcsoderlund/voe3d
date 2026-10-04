// The editor's splash: the engine's own splashscreen.png read into a texture,
// shown by game/starting.h behind the starting line.
//
//     voe_app_picture splash;
//     bool held = voe_editor_splash_read(device, scratch, &splash);
//     ... voe_game_starting_prepare(app, ui, scratch, held ? &splash : NULL, line)
//     if (held)
//             voe_render_texture_destroy(device, splash.texture);
//
// THE EDITOR ALWAYS SHOWS THE ENGINE'S SPLASH, never a project's (0346): a
// project's own Assets/splashscreen.png is its game's, not the editor's.
//
// READ FROM THE ENGINE SOURCE AT RUN TIME (0356): the file is
// `<VOE_TOOLCHAIN_ENGINE>/game/src/splashscreen.png`, the engine folder this
// build names in its generated toolchain.h, so a missing or renamed copy shows
// the plain screen without a rebuild. A false is that plain screen, with one
// stderr line naming the path; it never stops the editor.
//
// Constraints: the file and its pixels go through `scratch`, which the caller
// rewinds. The picture is the caller's, kept for the start and every scene
// load, and its texture is given back before the device closes. The upload
// waits for the GPU to go idle, so it is a startup operation.
#pragma once

#include <app/picture.h>

#include <base/arena.h>

#include <render/device.h>

#include <stdbool.h>

// Reads the engine's splash into out. False, out untouched, with one stderr
// line naming the path, when it cannot be read.
[[nodiscard]] bool voe_editor_splash_read(voe_render_device *device,
					  voe_base_arena *scratch,
					  voe_app_picture *out);
