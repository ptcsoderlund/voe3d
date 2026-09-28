// The Scene list: the panel's heading, its Add entity button and one row per
// authored entity, drawn into the dock's Scene leaf. dock.c calls it for that
// leaf, and the clicks are asked after the frame through scene.h:
//
//     voe_editor_scene_list_draw(ui, &scene);
//     ... voe_ui_frame_end ...
//     (scene.h reads the rows and Add entity it recorded)
//     voe_editor_scene_list_drop(&scene, ui, pointer.down, pointer.at);
//
// THE LIST IS THE IDENTITY TABLE AND NOTHING ELSE. It walks
// voe_scene_identity_rows and _entities rather than a list the editor keeps, so
// an entity the engine made for itself — no identity, hence not authored
// (ADR-0125) — cannot appear in it, and neither can an authored one go missing.
// There is nothing here to keep in step with the world.
//
// THE ROWS ARE THE TREE, DEPTH-FIRST (ADR-0281 point 7). Each root — no parent
// row, or a parent that is dead or has no identity — in identity-table order,
// and right after each row the authored entities whose parent it is, in table
// order, at any depth, by an explicit stack capped at
// VOE_SCENE_PARENT_DEPTH_MAX. An entity in a loop (only a hand-edited file
// makes one) or past the cap is reached from no root; it is still listed,
// after the rest, at depth 0, so no authored entity goes missing.
//
// EACH ROW IS INDENTED BY ITS DEPTH, a fixed number of millimetres per level
// set as a fixed-size spacer before the row, as ui/layout.h says a gap is made.
// The "Scene" heading's node is recorded in scene.heading for dragging.
//
// EVERY ROW IS KEYED BY ONE NAME AND ITS IDENTITY-TABLE INDEX, which is what
// `index` on a widget is for (ui/widgets.h): one name for every button would make
// the whole list one button sharing one highlight, and a key that is not the
// drawn position keeps a row's highlight on its entity when the order changes. The row's label points into
// the table, which outlives the frame — a name is 64 bytes in the component and
// never a pointer.
//
// THE ADD ENTITY BUTTON IS ABOVE THE LIST, recorded on the scene to be asked
// after the frame, as the rows are; what it makes is entities.h's (ADR-0217).
//
// A ROW DRAGGED AND RELEASED PARENTS IT (ADR-0281 point 7). Over another row it
// becomes that row's child, unless it already is or that row is itself or under
// it; over the heading it becomes a root again; anywhere else, nothing. The
// world place is kept (scene/parent_system.h), so nothing moves in the view. The
// drop is read after voe_ui_frame_end because only then does a button say it is
// held and a node where it was drawn. A set counts one in `structural`; a full
// queue sets `full`.
//
// A DRAG STARTS ONLY PAST VOE_EDITOR_SCENE_DRAG_START FROM THE PRESS (ADR-0282),
// so a click that wobbles stays a click, and a release before it drops nothing.
// While dragging, `list_target` is the release's own answer at the pointer, so
// what is drawn lit is what a release would do. Escape cancels a drag under way
// (voe_editor_scene_list_cancel): the release that follows neither parents nor
// selects.
#pragma once

#include "scene.h"

#include <ui/widgets.h>

// How far the pointer moves from the press, in millimetres, to start a drag.
#define VOE_EDITOR_SCENE_DRAG_START 1.0f

void voe_editor_scene_list_draw(voe_ui_context *ui, voe_editor_scene *scene);

// Follows the held row in scene's `list_` fields — its start, the threshold and
// the target at `at`, the root's millimetres — and on the first frame none is
// held and `down` is false, drops it there if it was dragged and not cancelled,
// then zeroes them. A held entity no longer alive, or either one without a
// transform, does nothing.
void voe_editor_scene_list_drop(voe_editor_scene *scene,
				const voe_ui_context *ui, bool down,
				voe_math_float2 at);

// When a drag is under way, cancels it for the rest of the press and returns
// true, so the caller spends Escape on it; else false and changes nothing.
bool voe_editor_scene_list_cancel(voe_editor_scene *scene);
