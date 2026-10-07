// The sculpting brush a person holds: which kind is chosen, if any, and its
// radius, strength and softness (0379 point 3). The Inspector's Sculpt section
// (inspector_sculpt.h) draws and sets it; scene.h holds the one there is.
//
//     voe_editor_sculpt_start(&scene.sculpt);
//     if (scene.sculpt.chosen &&
//         voe_editor_sculpt_wears(scene.world, scene.selected))
//             ... the brush acts on the selected thing's landscape ...
//
// THE BRUSH IS THE EDITOR'S, NEVER THE PROJECT'S: it is not saved, not undone
// and kept by a new project, as the gizmo's mode is (scene.h). CHOSEN STAYS
// CHOSEN ACROSS SELECTIONS, but acts only on a thing that wears a landscape:
// one whose model row names a `.landscape`, in any case, and that is not a
// prefab's part, whose rows come from its file and are never edited.
//
// Constraints: the numbers are kept inside the ranges below by whoever writes
// them, the Sculpt section's sliders clamping; nothing here allocates.
#pragma once

#include <3d/landscape.h>
#include <ecs/world.h>
#include <ui/layout.h>

#include <stdbool.h>

// The ranges and the values a start sets (0379 point 3). Radius in metres.
#define VOE_EDITOR_SCULPT_RADIUS_MIN 1.0f
#define VOE_EDITOR_SCULPT_RADIUS_MAX 200.0f
#define VOE_EDITOR_SCULPT_RADIUS 20.0f
#define VOE_EDITOR_SCULPT_STRENGTH_MIN 0.05f
#define VOE_EDITOR_SCULPT_STRENGTH_MAX 1.0f
#define VOE_EDITOR_SCULPT_STRENGTH 0.5f
#define VOE_EDITOR_SCULPT_SOFTNESS_MIN 0.0f
#define VOE_EDITOR_SCULPT_SOFTNESS_MAX 1.0f
#define VOE_EDITOR_SCULPT_SOFTNESS 0.5f

// Raise, Lower, Smooth and Flatten: one button each.
#define VOE_EDITOR_SCULPT_KINDS 4

// Radius, Strength and Softness: one slider each.
#define VOE_EDITOR_SCULPT_SLIDERS 3

// Room for a slider's shown figure.
#define VOE_EDITOR_SCULPT_FIGURE 16

typedef struct {
	// Whether a brush is chosen, and which; `kind` means nothing when not.
	bool chosen;
	voe_3d_brush_kind kind;
	float radius;
	float strength;
	float softness;
	// The Sculpt section's buttons and sliders as drawn this frame, and the
	// figures beside the sliders, kept because a label is read after the
	// call that drew it. VOE_UI_NODE_NONE when not drawn
	// (inspector_sculpt.h).
	voe_ui_node buttons[VOE_EDITOR_SCULPT_KINDS];
	voe_ui_node sliders[VOE_EDITOR_SCULPT_SLIDERS];
	char figures[VOE_EDITOR_SCULPT_SLIDERS][VOE_EDITOR_SCULPT_FIGURE];
} voe_editor_sculpt;

// No brush chosen, the numbers at their defaults, nothing drawn.
void voe_editor_sculpt_start(voe_editor_sculpt *sculpt);

// Chooses no brush; the numbers stay.
void voe_editor_sculpt_choose_none(voe_editor_sculpt *sculpt);

// Whether `entity` wears a landscape: a model row whose path ends `.landscape`
// in any case, on a thing that is not a prefab's part. False for a dead one.
bool voe_editor_sculpt_wears(const voe_ecs_world *world, voe_ecs_entity entity);
