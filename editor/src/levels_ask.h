// The question a model's levels ask (ADR 0430 point 6, 0436 point 8): a panel
// "LODs of <file>" with how many LODs the model has in all, 1 to 6 (0433), the
// chosen one lit; then a row per LOD from LOD 2 on; then Make and Cancel, as
// an anchored panel over the dock below the bar, drawn and read the way
// assets_ask.h's Delete question is.
//
// A LOD'S ROW is its name, share, swap, swap distance and triangles (0435
// points 1, 2). The share is its triangles in percent of LOD 1's, a number box
// in tenths, kept below the share before it and at least a tenth per LOD from
// it on; the later ones at or above it are pushed down a tenth below the one
// before. The swap multiplies the distance its error allows, a number box in
// tenths, ×0.1 to ×10. The swap distance is the metres at which that LOD swaps
// in for the model at scale 1 through a new camera's lens, after the cap and
// the 1.25 spread, following the typed swaps as they change; "—" with no fresh
// levels file or past the file's LODs (0500 point 3). The triangles are the
// file's, as voe_game_models_levels_summarise counts them, "—" when not known.
// A model's own levels hide the LODs row and show each share as a label,
// measured against LOD 1. The columns have headers, "Triangles kept", "Swap",
// "Swap distance" and "Triangles", each column one fixed width; a line under
// the rows explains the swap (0437 points 5, 6).
//
//     voe_editor_levels_ask_open(&ask, project_folder, "Assets/a.glb", scratch);
//     voe_editor_levels_ask_draw(ui, &ask, top);
//     ... voe_ui_frame_end ...
//     answer = voe_editor_levels_ask_read(ui, &ask, pointer_down, at);
//     if (answer == VOE_EDITOR_LEVELS_ASK_MAKE &&
//         voe_editor_levels_ask_save(&ask, project_folder, scratch)) {
//             if (voe_editor_levels_ask_swaps_only(&ask))
//                     ... reload the model, no splash ...
//             else
//                     ... prepare with remake ...
//     }
//
// THE VALUES SHOWN ARE THE MODEL'S OWN: on opening, the `.parts` beside the
// `.glb` (assets/parts.h) is read for its `[Levels]`, quietly, and none, or
// one that will not read, shows the defaults. A press on a count, or a share
// or swap set, only changes the shown values; Make's save writes them back
// into that `.parts`, every material line and the shares and swaps it held kept.
// Nothing here prepares a model or closes the question: the caller carries
// out the answer. A Make that changed only swaps on a fresh levels file is
// answered by a reload, the game's loader patching the swaps in (0436 point
// 8); any other Make by a prepare with remake.
//
// Constraints. A save refuses a `.parts` that is there but will not read,
// rather than write over its materials with an empty file. The path is cut at
// VOE_3D_MODEL_PATH bytes, as session.h's `prepare_path` holds it. The column
// widths are millimetres sized at 100 % text, so a larger text size, or a swap
// distance past 99999.9 m, sticks out of them; measuring the cells last frame,
// as assets_menu.c does, would lift it.
#pragma once

#include <3d/model_component.h>
#include <3d/model_levels.h>

#include <assets/parts.h>

#include <base/arena.h>

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>

// Zeroed is a question never asked.
typedef struct {
	bool open;
	// Project-relative, `Assets/….glb`, as session.h's `prepare_path`.
	char path[VOE_3D_MODEL_PATH];
	// The values shown and chosen, written by Make's save.
	voe_assets_levels levels;
	// The levels as opening read them, which a swap-only Make is told by.
	voe_assets_levels opened;
	// The levels as voe_game_models_levels_summarise read them on opening.
	voe_3d_model_levels_summary summary;
	// Kept here because a label's text is drawn after the call that made it.
	char title[VOE_3D_MODEL_PATH + sizeof "LODs of "];
	// Per level index, each the longest it can be: a share measured on own
	// levels may reach a uint32 count's hundredfold.
	char row_names[VOE_ASSETS_LEVELS][sizeof "LOD 6"];
	char row_shares[VOE_ASSETS_LEVELS][sizeof "4.29497e+11 %"];
	char row_swaps[VOE_ASSETS_LEVELS][sizeof "×0.123457"];
	// A float's largest metres at one decimal; "—" is shorter.
	char row_distances[VOE_ASSETS_LEVELS]
			  [sizeof "340282346638528859811704939345611489280.0 m"];
	char row_triangles[VOE_ASSETS_LEVELS][sizeof "4294967295"];
	voe_ui_node panel;
	voe_ui_node count_buttons[VOE_ASSETS_LEVELS];
	voe_ui_node share_boxes[VOE_ASSETS_LEVELS];
	voe_ui_node swap_boxes[VOE_ASSETS_LEVELS];
	voe_ui_node make_button;
	voe_ui_node cancel_button;
} voe_editor_levels_ask;

typedef enum {
	VOE_EDITOR_LEVELS_ASK_NONE = 0,
	VOE_EDITOR_LEVELS_ASK_MAKE,
	VOE_EDITOR_LEVELS_ASK_CANCEL,
} voe_editor_levels_ask_answer;

// Opens the question about the `.glb` at `path` under `project_folder`, its
// levels read from the `.parts` beside it (its `.glb` ending, any case, made
// `.parts`), else the defaults, and its triangles counted. `scratch` is
// rewound before returning.
void voe_editor_levels_ask_open(voe_editor_levels_ask *ask,
				const char *project_folder, const char *path,
				voe_base_arena *scratch);
void voe_editor_levels_ask_close(voe_editor_levels_ask *ask);

// Draws the question centred across the surface, `top` below its top, over the
// dock, its LODs row unless the levels are own, a header row, a row per LOD
// from LOD 2 on and the swap line when there is a LOD row, and Make and
// Cancel, and records its nodes. Asserts when it is not open.
void voe_editor_levels_ask_draw(voe_ui_context *ui, voe_editor_levels_ask *ask,
				float top);

// After voe_ui_frame_end: a fired count, or a LOD row's changed share or swap
// box, sets `ask->levels` and is NONE; Make or Cancel fired, or a
// `pointer_down` at `at` outside the panel, which is a Cancel; NONE otherwise.
voe_editor_levels_ask_answer
voe_editor_levels_ask_read(const voe_ui_context *ui, voe_editor_levels_ask *ask,
			   bool pointer_down, voe_math_float2 at);

// Whether Make needs no levels made: the summary's levels file is `fresh` and
// either `own`, or the count and shares are as `opened` had them.
bool voe_editor_levels_ask_swaps_only(const voe_editor_levels_ask *ask);

// The `.parts` read (none: an empty one naming no material), its levels set to
// `ask->levels` and written back. False with a line on stderr when it is there
// but will not read, or the write fails. `scratch` is rewound before returning.
[[nodiscard]] bool voe_editor_levels_ask_save(const voe_editor_levels_ask *ask,
					      const char *project_folder,
					      voe_base_arena *scratch);
