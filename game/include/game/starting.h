// The starting frame (0345): one line of text on the theme's ground, over a
// splash picture when there is one, drawn while a worker does a start's slow
// work, so the window is never blank or frozen (0362). Game and editor use it.
//
//     static bool work(void *context, voe_game_progress *progress)
//     {       struct start *s = context;
//             return voe_game_starting_shaders(s->device, s->cache, s->scratch,
//                                              progress); }
//     if (!voe_game_starting_wait(app, ui, frame_arena, splash, work, &start))
//             ...             // the window closed, or the work answered false
//
// THE WAIT'S TWO THREADS (0370 point 2). One thrd_t runs the work; the worker
// owns everything its context names until the wait returns. The main thread
// touches only the app's window, the ui context and the splash: it polls,
// draws element-only frames with the progress line and, while the window is
// not visible, draws none and waits on the window 50 ms at a time. A close
// sets the progress's stop and joins the worker, which stops before its next
// step, so quitting waits out at most one step, never a half GPU call. The
// shaders work loads the pipeline cache before its first step and saves it
// once PREPARED; a stopped or failed one writes nothing (0370 point 6).
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
// until the caller rewinds it; the wait rewinds it to where it found it after
// each frame. `line` is read at the ui frame's end and must be in the
// font's characters (text/font.h), which hold no em dash and no ellipsis. The
// splash is the caller's and its texture must outlive the frame.
#pragma once

#include <app/app.h>
#include <app/picture.h>

#include <base/arena.h>

#include <game/progress.h>

#include <render/device.h>

#include <ui/layout.h>

#include <stdbool.h>

// A start's slow work, run on the wait's worker with its context. It reports
// into progress and returns false once voe_game_progress_stopped says so.
typedef bool voe_game_starting_work(void *context, voe_game_progress *progress);

// One frame: the window polled, its size taken, `line` laid out plain or on
// `splash` (NULL for plain) and drawn in one element-only window pass.
// A size with no area draws nothing. False when the window is closing, the ui
// frame was refused or the draw failed.
[[nodiscard]] bool voe_game_starting_frame(voe_app *app, voe_ui_context *ui,
					   voe_base_arena *frame_arena,
					   const voe_app_picture *splash,
					   const char *line);

// `work` run with `context` on one worker while starting frames show its
// progress line, "Starting" until it sets one. The work's answer once it has
// ended; false, the worker stopped and joined, when a frame was false.
[[nodiscard]] bool voe_game_starting_wait(voe_app *app, voe_ui_context *ui,
					  voe_base_arena *frame_arena,
					  const voe_app_picture *splash,
					  voe_game_starting_work *work,
					  void *context);

// The shaders step, for a work: the cache at cache_path loaded (none when
// NULL), voe_render_device_prepare a step at a time as "Preparing shaders"
// done/steps, and the cache saved once PREPARED. False before the next step
// once stopped, or on FAILED. Scratch is the worker's and kept empty.
[[nodiscard]] bool voe_game_starting_shaders(voe_render_device *device,
					     const char *cache_path,
					     voe_base_arena *scratch,
					     voe_game_progress *progress);
