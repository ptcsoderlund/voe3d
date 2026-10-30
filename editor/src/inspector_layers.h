// The Inspector's Layers section, under Sculpt on a thing that wears a loaded
// landscape (0402 point 7): a "Layers" heading as Sculpt's, one row per layer
// of the landscape's grid by its material's file name, the one being painted
// drawn chosen with its grow rows under it, then "Drop a material to add a
// layer" while it holds fewer than eight. Pressing a row chooses painting
// that layer with Sculpt's sliders, also while SCATTER or ERASE is chosen
// (inspector_foliage.h) and no row is lit; pressing it again chooses none. A
// `.material` dropped on a row replaces that layer, on the last row adds one
// (assets_drag.h).
//
// THE GROW ROWS CHOOSE WHAT GROWS ON THE LAYER (0473 point 3): "Grows", then
// per foliage kind a tick by its model's file name, and for a ticked one a
// Density slider in copies per 100 m², 0.1 to 50. The choice is the layer
// row's, saved with it, and a row given another material keeps it (0476 point
// 2). The ticked kinds grow by themselves where the layer is painted, thick
// where the paint is strong; each tick and each density is one undo step,
// asked of sculpt.h's grow edit.
//
// THE SECTION FOLDS by its fold button, named `section layers` (folds.h,
// 0502); folded, only its header is drawn and no row is recorded.
//
//     voe_editor_inspector_layers_forget(&scene->sculpt);        // frame opened
//     voe_editor_inspector_layers_draw(ui, &scene->sculpt, folds, store,
//                                      world, selected);         // inspector.c
//     ...voe_ui_frame_end...
//     voe_editor_inspector_layers_read(ui, &scene->sculpt);      // interface_read.c
//
// THE NODES LIVE IN THE SCULPT STATE (sculpt.h), for the reason the Sculpt
// section's do (inspector_sculpt.h): a widget answers only after
// voe_ui_frame_end. Each frame forgets them first; the read keeps each row's
// rectangle for a drop the next frame, as the material map rows' are kept
// (inspector_material.h), and a frame that does not draw the section leaves
// none.
//
// Constraints: draw, forget and read are called in one frame's window, read
// before the frame's arena is rewound; nothing here allocates. A row's label
// points into the store's grid, which stands until voe_ui_frame_end.
#pragma once

#include "folds.h"
#include "sculpt.h"

#include <3d/models.h>

#include <assets/landscape.h>

#include <ecs/world.h>

#include <ui/layout.h>

// The landscape a thing wears, as voe_editor_inspector_layers_grid finds it.
typedef struct {
	// The loaded grid, NULL when the thing wears none or it is not loaded.
	const voe_assets_landscape *grid;
	// The model row's path, NULL whenever grid is.
	const char *path;
} voe_editor_inspector_layers_worn;

// The loaded grid `entity` wears and its model row's path, both NULL when it
// wears none (sculpt.h's voe_editor_sculpt_wears) or its file is not loaded.
voe_editor_inspector_layers_worn voe_editor_inspector_layers_grid(
	const voe_3d_models *store, const voe_ecs_world *world,
	voe_ecs_entity entity);

// Forgets last frame's rows, keeping their rectangles. Called when the frame
// opens.
void voe_editor_inspector_layers_forget(voe_editor_sculpt *sculpt);

// Puts the Layers section for `entity`'s loaded grid on the panel, open as
// `folds` says, recording its rows in `sculpt`; nothing when it wears none.
void voe_editor_inspector_layers_draw(voe_ui_context *ui,
				      voe_editor_sculpt *sculpt,
				      voe_editor_folds *folds,
				      const voe_3d_models *store,
				      const voe_ecs_world *world,
				      voe_ecs_entity entity);

// After voe_ui_frame_end: a fired tick or a changed Density slider asks the
// chosen layer's growth of its kind; a fired layer row chooses PAINT with that
// layer, or none when it was the chosen one; each row's rectangle kept.
void voe_editor_inspector_layers_read(const voe_ui_context *ui,
				      voe_editor_sculpt *sculpt);
