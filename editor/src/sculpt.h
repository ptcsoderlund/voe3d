// The sculpting brush a person holds: which kind is chosen, if any, and its
// radius, strength and softness (0379 point 3). The Inspector's Sculpt section
// (inspector_sculpt.h) draws and sets it, Layers (inspector_layers.h), Foliage
// (inspector_foliage.h) and Brush (inspector_brush.h) too; scene.h holds it.
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
// With kind PAINT the drag paints `layer` instead (0402 points 3 and 7), and
// the release's step carries the rectangle's weights. With SCATTER or ERASE
// it stamps foliage copies (0442 point 3); the step carries those it changed.
// A kind's look edit and a layer's grow edit (0476 points 1, 2 and 7) are
// settled each frame first, brush chosen or none: set live while the control
// is held, one step when let go. A tick asks VOE_EDITOR_SCULPT_GROW.
//
// Constraints: the numbers are kept inside the ranges below by whoever writes
// them, the Sculpt, Brush and Foliage sliders clamping, a kind's look to
// assets/landscape.h's ranges. A stroke mallocs a copy of the
// whole grid at the press, up to 513² heights, or for paint its weights at 8
// bytes a point, and frees it at the release; a foliage stroke's changed list
// doubles to fit a stamp's most, an erase's the landscape's copy count; a
// frame stamps at most VOE_EDITOR_SCULPT_STAMPS times, so a jump longer than
// that many quarter radii is stamped sparser; a higher cap would lift it.
#pragma once

#include "view.h"

#include <3d/landscape.h>
#include <3d/model_component.h>
#include <assets/landscape.h>
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

// The foliage brush's (0442 point 3, 0476 point 1): density in copies per
// 100 m².
#define VOE_EDITOR_SCULPT_DENSITY_MIN 0.1f
#define VOE_EDITOR_SCULPT_DENSITY_MAX 50.0f
#define VOE_EDITOR_SCULPT_DENSITY 4.0f

// A layer's growth of a kind (0476 point 2), in tenths of copies per 100 m²:
// the Density slider's range, and what a tick asks, the brush's 4 per 100 m².
#define VOE_EDITOR_SCULPT_GROW_MIN 1u
#define VOE_EDITOR_SCULPT_GROW_MAX VOE_ASSETS_LANDSCAPE_GROW_MAX
#define VOE_EDITOR_SCULPT_GROW 40u

// Raise, Lower, Smooth and Flatten: one button each.
#define VOE_EDITOR_SCULPT_KINDS 4

// The Brush section's Radius and Softness: one slider each.
#define VOE_EDITOR_SCULPT_BRUSH_SLIDERS 2

// Room for a slider's shown figure.
#define VOE_EDITOR_SCULPT_FIGURE 16

// The Layers section's rows: one per layer, and the add row.
#define VOE_EDITOR_SCULPT_LAYER_ROWS (VOE_ASSETS_LANDSCAPE_LAYERS + 1)

// The Foliage section's: Scatter and Erase, Density, and its rows, one per
// kind and the add row.
#define VOE_EDITOR_SCULPT_FOLIAGE_KINDS 2
#define VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS 1
#define VOE_EDITOR_SCULPT_KIND_ROWS (VOE_ASSETS_LANDSCAPE_KINDS + 1)

// A kind row's spreads (0476 point 1): Size ±%, Turn ±° and Tilt with slope %,
// one slider each in the look's own whole units, from 0 to assets/landscape.h's
// SIZE_VARY_MAX (100), TURN_VARY_MAX (180) and TILT_MAX (100).
#define VOE_EDITOR_SCULPT_SPREADS 3

typedef struct {
	// Whether a brush is chosen, and which; `kind` means nothing when not.
	bool chosen;
	voe_3d_brush_kind kind;
	float radius;
	float strength;
	float softness;
	// The layer a PAINT brush paints.
	uint32_t layer;
	// The foliage brush's: the kinds picked, bit k kind k, its density, and
	// the xorshift its scatters draw from, never 0.
	uint32_t picked;
	float density;
	uint64_t seed;
	// The Sculpt section's buttons and Strength slider as drawn this frame,
	// and the figure beside the slider, kept because a label is read after
	// the call that drew it. VOE_UI_NODE_NONE when not drawn
	// (inspector_sculpt.h).
	voe_ui_node buttons[VOE_EDITOR_SCULPT_KINDS];
	voe_ui_node strength_slider;
	char strength_figure[VOE_EDITOR_SCULPT_FIGURE];
	// The Brush section's Radius and Softness sliders and their figures, the
	// same way (inspector_brush.h).
	voe_ui_node brush_sliders[VOE_EDITOR_SCULPT_BRUSH_SLIDERS];
	char brush_figures[VOE_EDITOR_SCULPT_BRUSH_SLIDERS]
			  [VOE_EDITOR_SCULPT_FIGURE];
	// The Layers section's rows as drawn this frame, row i layer i and
	// row `layers_drawn` the add row, VOE_UI_NODE_NONE when not drawn; and
	// where each showed at the last read, for a drop the next frame
	// (assets_drag.h), no size for one not drawn (inspector_layers.h).
	voe_ui_node layer_rows[VOE_EDITOR_SCULPT_LAYER_ROWS];
	voe_ui_rect layers_seen[VOE_EDITOR_SCULPT_LAYER_ROWS];
	uint32_t layers_drawn;
	// The grow rows under the chosen layer's row, the same way: kind k's
	// tick, and its Density slider and figure, drawn only while ticked.
	voe_ui_node grow_ticks[VOE_ASSETS_LANDSCAPE_KINDS];
	voe_ui_node grow_sliders[VOE_ASSETS_LANDSCAPE_KINDS];
	char grow_figures[VOE_ASSETS_LANDSCAPE_KINDS][VOE_EDITOR_SCULPT_FIGURE];
	// The Foliage section's as drawn this frame, the Layers rows' way:
	// Scatter and Erase, the sliders and their figures, kind row i kind i
	// and row `kinds_drawn` the add row, and where each row showed at the
	// last read (inspector_foliage.h).
	voe_ui_node foliage_buttons[VOE_EDITOR_SCULPT_FOLIAGE_KINDS];
	voe_ui_node foliage_sliders[VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS];
	char foliage_figures[VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS]
			    [VOE_EDITOR_SCULPT_FIGURE];
	voe_ui_node kind_rows[VOE_EDITOR_SCULPT_KIND_ROWS];
	voe_ui_rect kinds_seen[VOE_EDITOR_SCULPT_KIND_ROWS];
	uint32_t kinds_drawn;
	// Each kind row's Base size slider and its figure, slider i kind i, its
	// Cast shadows button, its spread sliders and their figures, and each
	// drawn kind's look, which the read asks with one field changed.
	voe_ui_node kind_sliders[VOE_ASSETS_LANDSCAPE_KINDS];
	char kind_figures[VOE_ASSETS_LANDSCAPE_KINDS][VOE_EDITOR_SCULPT_FIGURE];
	voe_ui_node kind_shadows[VOE_ASSETS_LANDSCAPE_KINDS];
	voe_ui_node spread_sliders[VOE_ASSETS_LANDSCAPE_KINDS]
				  [VOE_EDITOR_SCULPT_SPREADS];
	char spread_figures[VOE_ASSETS_LANDSCAPE_KINDS][VOE_EDITOR_SCULPT_SPREADS]
			   [VOE_EDITOR_SCULPT_FIGURE];
	voe_assets_landscape_look kind_looks[VOE_ASSETS_LANDSCAPE_KINDS];
	// The last read's look ask, when `look_asked`: kind `look_kind` to
	// `look_to`; `look_held` that a kind's control is held. Both taken and
	// cleared by the next voe_editor_sculpt_read.
	bool look_asked;
	uint32_t look_kind;
	voe_assets_landscape_look look_to;
	bool look_held;
	// The look edit open, when `looking`: its kind, that kind's stored look
	// before it, and its landscape's path.
	bool looking;
	uint32_t looking_kind;
	voe_assets_landscape_look looking_before;
	char looking_path[VOE_3D_MODEL_PATH];
	// The last read's grow ask, when `grow_asked`: layer `grow_layer`'s
	// density of kind `grow_kind` to `grow_to` tenths; `grow_held` that its
	// control is held. Both taken and cleared by the next read, as the look's.
	bool grow_asked;
	uint32_t grow_layer;
	uint32_t grow_kind;
	uint16_t grow_to;
	bool grow_held;
	// The grow edit open, when `growing`: its layer and kind, that density
	// stored before it, and its landscape's path.
	bool growing;
	uint32_t growing_layer;
	uint32_t growing_kind;
	uint16_t growing_before;
	char growing_path[VOE_3D_MODEL_PATH];
	// The ground under the pointer this frame, when `hit`: (x, z) in the
	// grid's own space of `entity`'s landscape.
	bool hit;
	float x;
	float z;
	voe_ecs_entity entity;
	// The left button last frame, for its down edge.
	bool left_was;
	// The stroke held, when `stroking`: the last hit stamped to, flatten's
	// height, the points touched so far, the landscape's path, and its
	// grid's heights, or for PAINT both its paint planes, as they were at
	// the press, `cells` a side, malloced then and freed at the release;
	// the other copy NULL.
	bool stroking;
	float last_x;
	float last_z;
	float target;
	voe_3d_landscape_rect touched;
	char path[VOE_3D_MODEL_PATH];
	uint32_t cells;
	float *before;
	uint8_t *before_paint;
	// For SCATTER or ERASE instead of a copy: the copies the stroke added or
	// removed, `changed_room` of room, malloced at the press, grown by
	// doubling and freed at the release; NULL otherwise.
	voe_assets_landscape_copy *changed;
	uint32_t changed_count;
	uint32_t changed_room;
} voe_editor_sculpt;

// No brush chosen, the numbers at their defaults, nothing drawn.
void voe_editor_sculpt_start(voe_editor_sculpt *sculpt);

// Chooses no brush; the numbers stay.
void voe_editor_sculpt_choose_none(voe_editor_sculpt *sculpt);

// Whether `entity` wears a landscape: a model row whose path ends `.landscape`
// in any case, on a thing that is not a prefab's part. False for a dead one.
bool voe_editor_sculpt_wears(const voe_ecs_world *world, voe_ecs_entity entity);

// One frame of the brush on the selection's ground, after settling the
// look edit the Foliage read and the grow edit the Layers read asked for. Acts only while a brush is
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
