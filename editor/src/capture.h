// `--capture <path>`'s two moments (options.h): the drawn frames counted until
// there are enough to write, and the window target's picture written as a PNG
// through voe_app_capture_png, between frames and never inside one (ADR-0157).
// main.c counts once per drawn and submitted frame and writes once after its
// loop ends; both do nothing without a path.
//
// TWO FRAMES, because a view's target is sized from last frame's rectangle
// (view.h), so the first frame's views are drawn at a size the layout has
// already left behind. The picture written is the second drawn frame.
#pragma once

#include <app/app.h>

// Counts one drawn frame into *frames when path is set. True when that was
// the last frame a capture draws, so the loop stops; always false without one.
bool voe_editor_capture_enough(const char *path, unsigned *frames);

// Writes the window target's picture to path; true without a path. False when
// a step refused, each having said so on stderr.
[[nodiscard]] bool voe_editor_capture_write(voe_app *app, const char *path);
