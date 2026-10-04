// The starting frame (0345): one line of text on the theme's ground, over a
// splash picture when there is one, drawn while render builds its mesh pipelines, so the window is never blank
// while a start is slow. The game's run and the editor both use it.
//
//     voe_game_interface *interface = voe_game_interface_new(device, arena);
//     if (!voe_game_starting_prepare(app, voe_game_interface_context(interface),
//                                    scratch, NULL,
//                                    "Starting - preparing shaders..."))
//             ...                     // the window closed or a pipeline failed
//     voe_base_arena_clear(scratch);
//
// IT POLLS THE WINDOW BUT SKIPS voe_app_frame_open, whose pace waits a quarter
// second a frame for a window without focus (ADR-0215): a start behind another
// window would otherwise take that long per pipeline. A headless app has no
// window to poll and never waits, so there the frame is opened as usual for
// the settings' size.
//
// TWO LAYOUTS, BY `splash` (0356). NULL is the plain screen: a GROUND panel of
// the context's theme over the whole surface, the line centred in its normal
// text. A splash is its top-left texel stretched over the whole surface (the
// image's edge colour), the picture anchored centred at the largest size that
// fits without changing its aspect, and a GROUND panel holding the line in
// normal text, anchored at the bottom middle with its lower edge 8 mm above
// the surface's. The surface is voe_game_interface_surface of the window's
// size, read every frame, so a resize keeps the splash whole and centred.
//
// Constraints: the frame lays out in `frame_arena` and keeps its records there
// until the caller rewinds it; the prepare loop rewinds it to where it found it
// after each frame. `line` is read at the ui frame's end and must be in the
// font's characters (text/font.h), which hold no em dash and no ellipsis. The
// splash is the caller's and its texture must outlive the frame.
#pragma once

#include <app/app.h>
#include <app/picture.h>

#include <base/arena.h>

#include <ui/layout.h>

#include <stdbool.h>

// One frame: the window polled, its size taken, `line` laid out plain or on
// `splash` (NULL for plain) and drawn in one element-only window pass.
// A size with no area draws nothing. False when the window is closing, the ui
// frame was refused or the draw failed.
[[nodiscard]] bool voe_game_starting_frame(voe_app *app, voe_ui_context *ui,
					   voe_base_arena *frame_arena,
					   const voe_app_picture *splash,
					   const char *line);

// A starting frame, then voe_render_device_prepare once, repeated until the
// device answers PREPARED. False when a frame was false or the prepare FAILED.
[[nodiscard]] bool voe_game_starting_prepare(voe_app *app, voe_ui_context *ui,
					     voe_base_arena *frame_arena,
					     const voe_app_picture *splash,
					     const char *line);
