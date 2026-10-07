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
// A HELD DRAG SCULPTS (0379 points 3–5): voe_editor_sculpt_read, once a frame
// in frame_pointer.h's order, finds the ground under the pointer and, from a
// left press on it, stamps along the drag until the release, which records
// the touched rectangle as one undo step (strokes.h) and marks the project
// edited. A press that misses the ground is left to the gizmo and the pick.
//
// Constraints: the numbers are kept inside the ranges below by whoever writes
// them, the Sculpt section's sliders clamping. A stroke mallocs a copy of the
// whole grid at the press, up to 513² heights, and frees it at the release; a
// frame stamps at most VOE_EDITOR_SCULPT_STAMPS times, so a jump longer than
// that many quarter radii is stamped sparser; a higher cap would lift it.
#pragma once

#include "view.h"

#include <3d/landscape.h>
#include <3d/model_component.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <ui/layout.h>

#include <stdbool.h>
#include <stdint.h>

// scene.h, undo.h, session.h and models.h include this file, so they are
// named here by their tags.
struct voe_editor_scene;
struct voe_editor_undo;
struct voe_editor_session;
struct voe_editor_models;

// The most stamps one frame's stretch of a drag is cut into.
#define VOE_EDITOR_SCULPT_STAMPS 256u

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
	// The ground under the pointer this frame, when `hit`: (x, z) in the
	// grid's own space of `entity`'s landscape.
	bool hit;
	float x;
	float z;
	voe_ecs_entity entity;
	// The left button last frame, for its down edge.
	bool left_was;
	// The stroke held, when `stroking`: the last hit stamped to, flatten's
	// height, the heights touched so far, the landscape's path, and its
	// grid as it was at the press, `cells` a side, malloced then and freed
	// at the release.
	bool stroking;
	float last_x;
	float last_z;
	float target;
	voe_3d_landscape_rect touched;
	char path[VOE_3D_MODEL_PATH];
	uint32_t cells;
	float *before;
} voe_editor_sculpt;

// No brush chosen, the numbers at their defaults, nothing drawn.
void voe_editor_sculpt_start(voe_editor_sculpt *sculpt);

// Chooses no brush; the numbers stay.
void voe_editor_sculpt_choose_none(voe_editor_sculpt *sculpt);

// Whether `entity` wears a landscape: a model row whose path ends `.landscape`
// in any case, on a thing that is not a prefab's part. False for a dead one.
bool voe_editor_sculpt_wears(const voe_ecs_world *world, voe_ecs_entity entity);

// One frame of the brush on the selection's ground. Acts only while a brush is
// chosen and the selection wears a loaded landscape; else clears the hover,
// ends a held stroke and takes nothing. `pointer` is in the root surface's
// millimetres, `left` the left button down with the pointer in the window,
// `over` that no panel covers the views. True when this frame's press started
// a stroke. `scratch` is rewound.
bool voe_editor_sculpt_read(voe_editor_sculpt *sculpt,
			    struct voe_editor_scene *scene,
			    const voe_editor_views *views,
			    struct voe_editor_models *models,
			    struct voe_editor_undo *undo,
			    struct voe_editor_session *session,
			    voe_math_float2 pointer, bool left, bool over,
			    float seconds, voe_base_arena *scratch);
