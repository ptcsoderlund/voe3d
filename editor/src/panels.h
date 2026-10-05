// The editor's panels as the person left them: started from the default dock
// tree with settings.h's file read over it, and remembered back into that file
// when a drag of a border ends (resize.h says when) or a dock panel's × or
// the toggle below opens or closes it. The toggle also shows or hides Project,
// Errors and the frame breakdown, which no file remembers.
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
#include "frame_breakdown.h"
#include "preferences.h"
#include "project_panel.h"
#include "session.h"
#include "topbar.h"

// Sets `root->tree` to the default tree, its lengths and the views' share to
// whatever the file holds over the default's, `root->closed` from the file's
// open flags (open where it has none), and `bar->wanted` to the bar's
// remembered height (nought, fitting its content, when there is none).
void voe_editor_panels_start(voe_editor_dock_root *root, voe_editor_topbar *bar);

// The Scene list's, the Inspector's and the Assets panel's lengths and the
// views' share in `root`'s tree, the open flags from `root->closed` and the
// bar's `wanted`, written through voe_editor_settings_write, so a resize keeps
// which panels are open. False when that fails.
[[nodiscard]] bool voe_editor_panels_remember(const voe_editor_dock_root *root,
					      const voe_editor_topbar *bar);

// Opens a closed panel or closes an open one. A dock panel: `root->closed`
// flipped and remembered through voe_editor_panels_remember at once (0363
// point 5); false when the write fails, the flag staying flipped. PROJECT:
// shown is hidden, else shown with Preferences and Errors hidden. ERRORS:
// shown is hidden, else voe_editor_session_errors_show reads the build log
// and Project and Preferences hide (0363 point 4). Neither of the two writes
// the settings file, so both answer true. A failed build still shows Errors
// through the session whether or not it was closed here (0351). FRAME:
// `breakdown` shown or hidden, no other panel touched, true.
[[nodiscard]] bool voe_editor_panels_toggle(voe_editor_closable which,
					    voe_editor_dock_root *root,
					    const voe_editor_topbar *bar,
					    voe_editor_project_panel *project,
					    voe_editor_preferences *preferences,
					    voe_editor_session *session,
					    voe_editor_frame_breakdown *breakdown);

// Whether `which` is open: a dock panel not closed on `root`, the Project
// panel, Errors or the frame breakdown showing. The Panels menu's tick.
bool voe_editor_panels_open(voe_editor_closable which,
			    const voe_editor_dock_root *root,
			    const voe_editor_project_panel *project,
			    const voe_editor_session *session,
			    const voe_editor_frame_breakdown *breakdown);
