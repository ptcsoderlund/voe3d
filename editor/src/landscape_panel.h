// The editor's Landscape panel (0379 point 6): a "Landscape" title row with
// the file's name and an × at its right, a "Size (m)" and a "Cells" number box
// showing what it was shown with — an anchored panel over the dock, below the
// bar, exactly where project_panel.h's goes. Opened by a `.landscape` row clicked
// in the Assets panel and after Create -> Landscape:
//
//     voe_editor_landscape_panel panel = { 0 };
//     voe_editor_landscape_panel_show(&panel, "Assets/hill.landscape", 256.0f,
//                                     512);
//     voe_editor_landscape_panel_draw(ui, &panel, top, size);
//     ... voe_ui_frame_end ...
//     result = voe_editor_landscape_panel_clicks_read(ui, &panel);
//
// A CHANGE IS A SETTING WRITTEN AT ONCE, NEVER UNSAVED OR UNDONE. The caller
// hands a new size and cells to models.h's voe_editor_models_landscape_shape,
// which writes the file with its heights stretched and resampled; it marks
// nothing unsaved and never enters the undo line. Once written the caller
// shows the panel again with the new shape.
//
// THE CELLS ARE DETAIL PER METRE (0396 point 2): a 4 km landscape needs 2048
// to keep about 2 m a cell. Going down loses detail going back up does not
// return.
//
// A typed or dragged size is rounded to a whole metre and clamped to
// VOE_ASSETS_LANDSCAPE_SIZE_MIN..MAX, cells to a multiple of 4 from 4 to
// VOE_ASSETS_LANDSCAPE_CELLS_MAX, before they are handed back.
//
// THIS FILE CARRIES OUT NOTHING ITSELF, exactly as project_panel.h does not: a
// `ui` widget answers only after voe_ui_frame_end, so the draw records each
// control and voe_editor_landscape_panel_clicks_read says what it did.
//
// Constraints. The path is project-relative, `Assets/...`, and up to
// VOE_SCENE_PREFAB_PATH bytes with its zero, as the Assets panel's paths are.
// No scroll area: its three rows fit the smallest surface. Drawing asserts when
// it is not showing.
#pragma once

#include <math/float2.h>

#include <scene/prefab_component.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// Zeroed is a panel never shown.
typedef struct {
	bool showing;
	// The landscape's path, `Assets/...`, and the size and cells it was
	// shown with.
	char path[VOE_SCENE_PREFAB_PATH];
	float size;
	uint32_t cells;
	// A drag under way on each box, and the value it has reached.
	bool dragging;
	float dragged;
	bool cells_dragging;
	float cells_dragged;
	// The controls, recorded by the draw.
	voe_ui_node size_box;
	voe_ui_node cells_box;
	voe_ui_node close_button;
	// The boxes' text, `%d`, kept here because a label's text is drawn after
	// the call that made it (ui/widgets.h).
	char size_text[16];
	char cells_text[16];
} voe_editor_landscape_panel;

// Shows `path` at `size` metres and `cells` a side; `path` may be the panel's
// own. False, the panel unchanged, when it does not fit.
[[nodiscard]] bool voe_editor_landscape_panel_show(
	voe_editor_landscape_panel *panel, const char *path, float size,
	uint32_t cells);
void voe_editor_landscape_panel_hide(voe_editor_landscape_panel *panel);

// Draws the panel filling top..surface.y of `surface`'s width, the area below
// the bar, over the dock. Records every control into panel. Asserts when it is
// not showing.
void voe_editor_landscape_panel_draw(voe_ui_context *ui,
				     voe_editor_landscape_panel *panel,
				     float top, voe_math_float2 surface);

// What this frame did: the × fired, and whether a number was typed or a drag
// let go, with the size and cells it makes.
typedef struct {
	bool closed;
	bool changed;
	float size;
	uint32_t cells;
} voe_editor_landscape_panel_result;

// Called after voe_ui_frame_end and before the frame's arena is rewound. A
// drag shows as it goes and is handed back once, when let go, so the file is
// written once a drag rather than once a frame.
voe_editor_landscape_panel_result
voe_editor_landscape_panel_clicks_read(const voe_ui_context *ui,
				       voe_editor_landscape_panel *panel);
