// The Inspector's Foliage section, under Layers on a thing that wears a loaded
// landscape (0442 point 3): a "Foliage" heading as Layers', Scatter and Erase
// with the chosen one drawn chosen, one row per kind of the landscape's grid
// by its model's file name, the picked ones drawn chosen, each followed by its
// Base size slider, a Cast shadows button drawn chosen while the kind casts,
// a Blend with ground button drawn chosen while it blends, its Blend height
// slider, and its Size ±%, Turn ±° and Tilt with slope % sliders, then "Drop a
// model to add a kind" while it holds fewer than sixteen, and the brush's one
// slider, Density (0476 point 1). Radius and Softness are the Brush section's
// (inspector_brush.h), shared by every brush.
//
// THE SPREADS AND CAST SHADOWS ARE THE KIND'S (0476 point 1), as its base
// size is, each change one undo step: a spread slider's when it lets go, Cast
// shadows' at the press. Cast shadows off stops the kind's copies casting, but
// they are still shadowed.
//
// THE BLEND IS THE KIND'S LOOK TOO (0481 point 4), off by default, each change
// one undo step: Blend with ground's at the press, Blend height's when it lets
// go. Blend height is the model's slider and range, 0.05 to 10.00 m (0494
// point 2), stored in the look as whole centimetres. Its cost shows as
// `foliage blend` in the frame breakdown.
//
// A KIND'S BASE SIZE (0469), 0.10× to 10.00×, is part of its look: it resizes
// that kind's copies already painted as well as later ones, live while
// dragged, and is one undo step when the slider lets go or a typed value is
// taken. The read only asks the kind's look as drawn with its size changed,
// and voe_editor_sculpt_read (sculpt.h) sets it and records the step.
//
// Pressing Scatter or Erase chooses that brush; pressing the chosen one again
// chooses none. Pressing a kind row flips whether it is picked, and only
// picked kinds are scattered or erased. A `.glb` dropped on a kind row
// replaces that kind's model, on the last row adds one (assets_drag.h).
//
// THE SECTION FOLDS by its fold button, named `section foliage` (folds.h,
// 0502); folded, only its header is drawn and nothing is recorded.
//
//     voe_editor_inspector_foliage_forget(&scene->sculpt);       // frame opened
//     voe_editor_inspector_foliage_draw(ui, &scene->sculpt, folds, store,
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

#include "folds.h"
#include "sculpt.h"

#include <3d/models.h>

#include <ecs/world.h>

#include <ui/layout.h>

// Forgets last frame's buttons, rows and sliders, keeping the rows'
// rectangles. Called when the frame opens.
void voe_editor_inspector_foliage_forget(voe_editor_sculpt *sculpt);

// Puts the Foliage section for `entity`'s loaded grid on the panel, open as
// `folds` says, recording its nodes in `sculpt`; nothing when it wears none.
void voe_editor_inspector_foliage_draw(voe_ui_context *ui,
				       voe_editor_sculpt *sculpt,
				       voe_editor_folds *folds,
				       const voe_3d_models *store,
				       const voe_ecs_world *world,
				       voe_ecs_entity entity);

// After voe_ui_frame_end: a fired button chooses its kind, or none when it was
// the chosen one; a fired kind row flips its picked bit; each drawn slider's
// value taken inside sculpt.h's range; a changed kind slider asks for its
// kind's look with that size in thousandths, a spread slider with that spread,
// and a held one is recorded; a fired Cast shadows asks it with `shadowless`
// flipped, a fired Blend with ground with `blends` flipped, a Blend height
// slider with that height; each row's rectangle kept.
void voe_editor_inspector_foliage_read(const voe_ui_context *ui,
				       voe_editor_sculpt *sculpt);
