// The Inspector's Foliage section, under Layers on a thing that wears a loaded
// landscape (0442 point 3): a "Foliage" heading as Layers', Scatter and Erase
// with the chosen one drawn chosen, one row per kind of the landscape's grid
// by its model's file name, the picked ones drawn chosen, then "Drop a model
// to add a kind" while it holds fewer than sixteen, and Density, Size (%),
// Turn (deg) and Lean (%) as sliders. Radius and Softness stay the Sculpt
// section's, shared by both brushes.
//
// Pressing Scatter or Erase chooses that brush; pressing the chosen one again
// chooses none. Pressing a kind row flips whether it is picked, and only
// picked kinds are scattered or erased. A `.glb` dropped on a kind row
// replaces that kind's model, on the last row adds one (assets_drag.h).
//
//     voe_editor_inspector_foliage_forget(&scene->sculpt);       // frame opened
//     voe_editor_inspector_foliage_draw(ui, &scene->sculpt, store,
//                                       world, selected);        // inspector.c
//     ...voe_ui_frame_end...
//     voe_editor_inspector_foliage_read(ui, &scene->sculpt);     // interface_read.c
//
// THE NODES LIVE IN THE SCULPT STATE (sculpt.h), for the reason the Layers
// section's do (inspector_layers.h): each frame forgets them first, and the
// read keeps each kind row's rectangle for a drop the next frame.
//
// Constraints: draw, forget and read are called in one frame's window, read
// before the frame's arena is rewound; nothing here allocates. A row's label
// points into the store's grid, which stands until voe_ui_frame_end.
#pragma once

#include "sculpt.h"

#include <3d/models.h>

#include <ecs/world.h>

#include <ui/layout.h>

// Forgets last frame's buttons, rows and sliders, keeping the rows'
// rectangles. Called when the frame opens.
void voe_editor_inspector_foliage_forget(voe_editor_sculpt *sculpt);

// Puts the Foliage section for `entity`'s loaded grid on the panel, recording
// its nodes in `sculpt`; nothing when it wears none.
void voe_editor_inspector_foliage_draw(voe_ui_context *ui,
				       voe_editor_sculpt *sculpt,
				       const voe_3d_models *store,
				       const voe_ecs_world *world,
				       voe_ecs_entity entity);

// After voe_ui_frame_end: a fired button chooses its kind, or none when it was
// the chosen one; a fired kind row flips its picked bit; each drawn slider's
// value taken inside sculpt.h's range; each row's rectangle kept.
void voe_editor_inspector_foliage_read(const voe_ui_context *ui,
				       voe_editor_sculpt *sculpt);
