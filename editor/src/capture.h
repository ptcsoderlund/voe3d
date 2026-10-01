// `--capture <path>`'s two moments (options.h): the drawn frames counted until
// there are enough to write, and the pictures written as PNGs through
// voe_app_capture_png, between frames and never inside one (ADR-0157).
// main.c counts once per drawn and submitted frame and writes once after its
// loop ends; every call does nothing without a path.
//
// TWO FRAMES BY DEFAULT, because a view's target is sized from last frame's
// rectangle (view.h), so the first frame's views are drawn at a size the
// layout has already left behind. The picture written is the last drawn frame.
// `--frames <n>` draws more, which is what lets the bounce grid fill: it enters
// 4096 of its 32³ probes a frame (ADR-0308), so two frames show a quarter of it
// and eight or more show all of it.
//
// THE VIEW PICTURE IS THE SCENE VIEW'S OWN TARGET AT ITS OWN SIZE, not a crop
// of the window: the window's layout follows remembered settings, but a view's
// target is exactly the picture its camera drew, so a measurement can project
// world points into it with that camera.
#pragma once

#include <app/app.h>

// Counts one drawn frame into *frames when path is set. True when that was
// frame `reach`, the last a capture draws, so the loop stops; always false
// without a path.
bool voe_editor_capture_enough(const char *path, unsigned *frames,
			       unsigned reach);

// Writes the window target's picture to path; true without a path. False when
// a step refused, each having said so on stderr.
[[nodiscard]] bool voe_editor_capture_write(voe_app *app, const char *path);

// Writes `target`'s picture to path, as voe_editor_capture_write does the
// window's; true without a path.
[[nodiscard]] bool voe_editor_capture_write_view(voe_app *app,
						 voe_render_target target,
						 const char *path);
