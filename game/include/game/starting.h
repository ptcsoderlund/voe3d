// The starting frame (0345): one line of text centred on the theme's ground,
// drawn while render builds its mesh pipelines, so the window is never blank
// while a start is slow. The game's run and the editor both use it.
//
//     voe_game_interface *interface = voe_game_interface_new(device, arena);
//     if (!voe_game_starting_prepare(app, voe_game_interface_context(interface),
//                                    scratch, "Starting - preparing shaders..."))
//             ...                     // the window closed or a pipeline failed
//     voe_base_arena_clear(scratch);
//
// IT POLLS THE WINDOW BUT SKIPS voe_app_frame_open, whose pace waits a quarter
// second a frame for a window without focus (ADR-0215): a start behind another
// window would otherwise take that long per pipeline. A headless app has no
// window to poll and never waits, so there the frame is opened as usual for
// the settings' size.
//
// THE COLOURS ARE THE CONTEXT'S THEME IN FORCE: a GROUND panel over the whole
// surface and the line in its normal text, so a light theme reads as well as a
// dark one. The surface is voe_game_interface_surface of the window's size.
//
// Constraints: the frame lays out in `frame_arena` and keeps its records there
// until the caller rewinds it; the prepare loop rewinds it to where it found it
// after each frame. `line` is read at the ui frame's end and must be in the
// font's characters (text/font.h), which hold no em dash and no ellipsis.
#pragma once

#include <app/app.h>

#include <base/arena.h>

#include <ui/layout.h>

#include <stdbool.h>

// One frame: the window polled, its size taken, `line` laid out centred on a
// GROUND panel filling the surface and drawn in one element-only window pass.
// A size with no area draws nothing. False when the window is closing, the ui
// frame was refused or the draw failed.
[[nodiscard]] bool voe_game_starting_frame(voe_app *app, voe_ui_context *ui,
					   voe_base_arena *frame_arena,
					   const char *line);

// A starting frame, then voe_render_device_prepare once, repeated until the
// device answers PREPARED. False when a frame was false or the prepare FAILED.
[[nodiscard]] bool voe_game_starting_prepare(voe_app *app, voe_ui_context *ui,
					     voe_base_arena *frame_arena,
					     const char *line);
