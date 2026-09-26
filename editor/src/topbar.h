// The bar across the top of the root surface: New, Open and Save, then Play,
// then Refresh, then Ship, then Preferences, then the project's name and
// whether it is unsaved, then whatever notice the session has to say. Play's
// label is the caller's (the session's play state, play.h): Play, Building or
// Stop; so are Refresh's (refresh.h), Refresh or Refreshing, and Ship's
// (session.h), Ship or Shipping.
//
// IT IS AT LEAST WHAT ITS BUTTONS AND LABELS MEASURED LAST FRAME (ADR-0225,
// amended by ADR-0226). A larger text size needs a taller bar, but dock.c
// divides a known height, so the bar's has to be a number before the frame is
// built: voe_editor_topbar_measure keeps what the panel's content measured as
// the least, `wanted` is the height the person made it, and
// voe_editor_topbar_high is the larger, short enough to leave the dock a
// view's room. Where the bar sits, above the dock tree, stretched to whatever
// width it is given, is interface.c's decision.
//
// THE SEVEN BUTTONS ARE RECORDED AND READ BACK, EXACTLY AS THE SCENE PANEL'S
// ROWS ARE (scene.h). A `ui` widget answers what the pointer did to it only
// after voe_ui_frame_end (ui/widgets.h), and this call returns long before
// that, so voe_editor_topbar_draw records where each button is and
// voe_editor_topbar_clicks_read asks afterwards, inside the same window
// scene.c's and inspector.c's own reads use.
//
// THE COMMAND IT HANDS BACK IS NOT CARRIED OUT HERE. This file knows nothing
// of a project, a world or an arm — it says which button fired and nothing
// more; what a command does is session.h's. Preferences is no session
// command — it only shows preferences.h's panel — so it is read on its own,
// by voe_editor_topbar_preferences_read.
#pragma once

#include "session.h"

#include <base/arena.h>

#include <ui/layout.h>

// How tall the bar is on the first frame, before anything has been measured, in
// the surface's own millimetres.
#define VOE_EDITOR_TOPBAR_HIGH 10.0f

// The bar's panel and seven buttons, recorded as they are drawn, and the height
// it is laid out at. Zeroed is a bar that has drawn nothing yet; `high` nought
// means VOE_EDITOR_TOPBAR_HIGH, `wanted` nought means fit the content.
typedef struct {
	voe_ui_node panel;
	voe_ui_node new_button;
	voe_ui_node open_button;
	voe_ui_node save_button;
	voe_ui_node play_button;
	voe_ui_node refresh_button;
	voe_ui_node ship_button;
	voe_ui_node preferences_button;
	float high;
	float wanted;
} voe_editor_topbar;

// The height the content measured last frame, or VOE_EDITOR_TOPBAR_HIGH on the
// first, in millimetres.
float voe_editor_topbar_least(const voe_editor_topbar *bar);

// The height to lay the bar out at this frame in a surface `surface_high` tall:
// the larger of the least and `wanted`, at most `surface_high` less
// VOE_EDITOR_DOCK_VIEW_ROOM, never below nought. Millimetres.
float voe_editor_topbar_high(const voe_editor_topbar *bar, float surface_high);

// Keeps what the panel's content measured as the next frame's height. Called
// after voe_ui_frame_end, in the window voe_editor_topbar_clicks_read uses; a
// panel the frame had no room for leaves the height as it was.
void voe_editor_topbar_measure(const voe_ui_context *ui,
			       voe_editor_topbar *bar);

// Draws the bar as one row: New, Open, Save, `play` as Play's label,
// `refresh` as Refresh's, `ship` as Ship's, Preferences, then `name` with " (unsaved)"
// appended when `unsaved` is true, then `notice` when it is not empty. `arena`
// is where " (unsaved)" is composed onto `name` — the frame's own, valid for
// exactly as long as the row's labels are (ui/widgets.h). Records the
// panel and the seven buttons into `bar`, laid out at
// voe_editor_topbar_high(bar, surface_high).
void voe_editor_topbar_draw(voe_ui_context *ui, voe_editor_topbar *bar,
			   voe_base_arena *arena, float surface_high,
			   const char *play, const char *refresh,
			   const char *ship, const char *name, bool unsaved,
			   const char *notice);

// Which button fired this frame, or VOE_EDITOR_COMMAND_NONE when none did.
// Called after voe_ui_frame_end and before the frame's arena is rewound — the
// one window in which a widget will answer.
voe_editor_command voe_editor_topbar_clicks_read(const voe_ui_context *ui,
						 const voe_editor_topbar *bar);

// Whether Preferences fired this frame, in the same window as
// voe_editor_topbar_clicks_read.
bool voe_editor_topbar_preferences_read(const voe_ui_context *ui,
					const voe_editor_topbar *bar);
