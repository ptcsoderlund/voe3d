// The question a model's levels ask (ADR 0430 point 6): a panel "Levels of
// <file>" with how many levels the model has in all, 1 to 4, and how hard each
// cuts, Gentle, Medium or Strong (0430 points 2 and 3), the chosen two lit,
// then Make and Cancel, as an anchored panel over the dock below the bar, drawn
// and read the way assets_ask.h's Delete question is.
//
//     voe_editor_levels_ask_open(&ask, project_folder, "Assets/a.glb", scratch);
//     voe_editor_levels_ask_draw(ui, &ask, top);
//     ... voe_ui_frame_end ...
//     answer = voe_editor_levels_ask_read(ui, &ask, pointer_down, at);
//     if (answer == VOE_EDITOR_LEVELS_ASK_MAKE &&
//         voe_editor_levels_ask_save(&ask, project_folder, scratch))
//             ... prepare with remake ...
//
// THE VALUES SHOWN ARE THE MODEL'S OWN: on opening, the `.parts` beside the
// `.glb` (assets/parts.h) is read for its `[Levels]`, quietly, and none, or
// one that will not read, shows the defaults. A press on a count or strength
// only changes the shown values; Make's save writes them back into that
// `.parts`, every material line kept. Nothing here prepares a model or closes
// the question: the caller carries out the answer.
//
// Constraints. A save refuses a `.parts` that is there but will not read,
// rather than write over its materials with an empty file. The path is cut at
// VOE_3D_MODEL_PATH bytes, as session.h's `prepare_path` holds it.
#pragma once

#include <3d/model_component.h>

#include <assets/parts.h>

#include <base/arena.h>

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>

// The strengths offered, in voe_assets_levels_strength's order.
#define VOE_EDITOR_LEVELS_ASK_STRENGTHS 3

// Zeroed is a question never asked.
typedef struct {
	bool open;
	// Project-relative, `Assets/….glb`, as session.h's `prepare_path`.
	char path[VOE_3D_MODEL_PATH];
	// The values shown and chosen, written by Make's save.
	voe_assets_levels levels;
	// Kept here because a label's text is drawn after the call that made it.
	char title[VOE_3D_MODEL_PATH + sizeof "Levels of "];
	voe_ui_node panel;
	voe_ui_node count_buttons[VOE_ASSETS_LEVELS];
	voe_ui_node strength_buttons[VOE_EDITOR_LEVELS_ASK_STRENGTHS];
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
// `.parts`), else the defaults. `scratch` is rewound before returning.
void voe_editor_levels_ask_open(voe_editor_levels_ask *ask,
				const char *project_folder, const char *path,
				voe_base_arena *scratch);
void voe_editor_levels_ask_close(voe_editor_levels_ask *ask);

// Draws the question centred across the surface, `top` below its top, over the
// dock, and records its nodes. Asserts when it is not open.
void voe_editor_levels_ask_draw(voe_ui_context *ui, voe_editor_levels_ask *ask,
				float top);

// After voe_ui_frame_end: a fired count or strength sets `ask->levels` and is
// NONE; Make or Cancel fired, or a `pointer_down` at `at` outside the panel,
// which is a Cancel; NONE otherwise.
voe_editor_levels_ask_answer
voe_editor_levels_ask_read(const voe_ui_context *ui, voe_editor_levels_ask *ask,
			   bool pointer_down, voe_math_float2 at);

// The `.parts` read (none: an empty one naming no material), its levels set to
// `ask->levels` and written back. False with a line on stderr when it is there
// but will not read, or the write fails. `scratch` is rewound before returning.
[[nodiscard]] bool voe_editor_levels_ask_save(const voe_editor_levels_ask *ask,
					      const char *project_folder,
					      voe_base_arena *scratch);
