// The Scene list: the panel's heading, its Add entity button and one row per
// authored entity, drawn into the dock's Scene leaf. dock.c calls it for that
// leaf, and the clicks are asked after the frame through scene.h:
//
//     voe_editor_scene_list_draw(ui, palette, &scene);
//     voe_editor_scene_list_ghost_draw(ui, &scene, pointer.at);  // root
//     ... voe_ui_frame_end ...
//     (scene.h reads the rows and Add entity it recorded)
//     voe_editor_scene_list_drop(&scene, ui, pointer.down, pointer.at,
//                                over_assets, assets_take);
//
// THE LIST IS THE IDENTITY TABLE AND NOTHING ELSE, walked directly, so an
// entity the engine made for itself (no identity, not authored, ADR-0125)
// cannot appear and an authored one cannot go missing.
//
// THE ROWS ARE THE TREE, DEPTH-FIRST (ADR-0281 point 7): each root in table
// order, then the entities under it in table order, by an explicit stack capped
// at VOE_SCENE_PARENT_DEPTH_MAX, each indented by a fixed-size spacer per level.
// One in a loop or past the cap is listed after the rest at depth 0.
//
// EVERY ROW IS KEYED BY ONE NAME AND ITS IDENTITY-TABLE INDEX, so each row is
// its own button and its highlight follows its entity when the order changes.
// Its label points into the table, which outlives the frame. Add entity sits
// above the list and is asked after the frame, as the rows are (ADR-0217).
//
// A PLACED COPY'S ROOT SHOWS ITS PREFAB'S FILE NAME after its name, in the
// secondary text role, so it reads as a prefab at a glance; its parts are
// listed under it as any tree (0283 point 5).
//
// A ROW DRAGGED AND RELEASED PARENTS IT (ADR-0281 point 7): over another row it
// becomes its child, unless it already is or that row is itself or under it;
// over the heading (scene.heading) a root; elsewhere nothing. Any row, with or
// without a transform, parents and takes children (0300); a child with a
// transform under a bare row keeps its world place and its row. The drop is read after voe_ui_frame_end, when a button says it is
// held. A set counts one in `structural`; a full queue sets `full`.
//
// A PART IS NEVER DRAGGED OR DROPPED ONTO (0283 point 5): a press on one only
// selects, and no rim or drop lands on its row. A part is the prefab's, and a
// thing under one would be saved naming what no file holds. A copy's root
// drags and takes a drop as any row.
//
// A DRAG OVER THE ASSETS PANEL HAS THAT AS ITS TARGET (0283 point 6): no row is
// rimmed and the ghost still follows; released there, the held entity is left
// in `list_made` for the caller to make a prefab of, and nothing is parented.
//
// A DRAG STARTS ONLY PAST VOE_EDITOR_SCENE_DRAG_START FROM THE PRESS (ADR-0282),
// so a wobbling click stays a click. While dragging, `list_target` is the
// release's own answer at the pointer. Escape cancels a drag under way: the
// release that follows neither parents nor selects.
//
// A DRAG SHOWS THREE MARKS (ADR-0282): a raised ghost of its name beside the
// pointer (drag_ghost.h), the held row dimmed, and an `inverse` rim (not an
// accent, ADR-0194) round what a release would land on. Every row and the
// heading always sit in a keyed wrapper padded by the rim, so keys and spacing
// never change. REFUSED (ADR-0285) is no row or heading target and not over an
// Assets panel that takes it: the ghost is then dimmed under `list_dim` and
// says "Can't drop here".
#pragma once

#include "scene.h"

#include <ui/widgets.h>

// How far the pointer moves from the press, in millimetres, to start a drag.
#define VOE_EDITOR_SCENE_DRAG_START 1.0f

// `palette` is the theme in force; the drag's dim and rim are derived from it.
void voe_editor_scene_list_draw(voe_ui_context *ui,
				const voe_ui_theme *palette,
				voe_editor_scene *scene);

// While a drag is under way and the held entity is alive with an identity: the
// drag ghost of its name at `at`, refused when `list_refused` says so. Called
// in the root surface, `at` in its millimetres.
void voe_editor_scene_list_ghost_draw(voe_ui_context *ui,
				      const voe_editor_scene *scene,
				      voe_math_float2 at);

// Follows the held row in scene's `list_` fields — its start, the threshold and
// the target at `at`, the root's millimetres — and on the first frame none is
// held and `down` is false, drops it there if it was dragged and not cancelled,
// then zeroes them. A held entity no longer alive does nothing. `over_assets` is whether `at` is over the Assets
// panel: no target then, and a drop there sets `list_made` instead.
// `assets_take` is whether a make there would go ahead; while dragging it and
// the target set `list_refused`, and the release ignores it.
void voe_editor_scene_list_drop(voe_editor_scene *scene,
				const voe_ui_context *ui, bool down,
				voe_math_float2 at, bool over_assets,
				bool assets_take);

// When a drag is under way, cancels it for the rest of the press and returns
// true, so the caller spends Escape on it; else false and changes nothing.
bool voe_editor_scene_list_cancel(voe_editor_scene *scene);
