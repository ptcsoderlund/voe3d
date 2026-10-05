// The editor's panels as the person left them: started from the default dock
// tree with settings.h's file read over it, and remembered back into that file
// when a drag of a border ends (resize.h says when).
//
//     voe_editor_panels_start(&roots[0], &bar);          // once, before the loop
//     if (!voe_editor_panels_remember(&roots[0], &bar))   // a drag ended
//             ...the notice from base/report.h's kept error...
//
// ONE FILE FOR THE PERSON, NOT THE PROJECT (ADR-0220, ADR-0226). The Scene
// list's, the Inspector's and the Assets panel's lengths, the views' share and
// the top bar's height are how a person likes their editor, so every project
// opens with them as they were last left. Which panels are open joins them
// here (0363 point 5), started and remembered by the same two calls.
//
// Constraints: both calls are synchronous file reads or writes through
// settings.h; remember is made once when a drag ends, never while one goes on.
// A capture reads the file too, as it reads the themes.
#pragma once

#include "dock.h"
#include "topbar.h"

// Sets `root->tree` to the default tree, its lengths and the views' share to
// whatever the file holds over the default's, and `bar->wanted` to the bar's
// remembered height (nought, fitting its content, when there is none).
void voe_editor_panels_start(voe_editor_dock_root *root, voe_editor_topbar *bar);

// The Scene list's, the Inspector's and the Assets panel's lengths and the
// views' share in `root`'s tree and the bar's `wanted`, written through
// voe_editor_settings_write. False when that fails.
[[nodiscard]] bool voe_editor_panels_remember(const voe_editor_dock_root *root,
					      const voe_editor_topbar *bar);
