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
// it; over the heading it becomes a root again; anywhere else, nothing. A press
// and release on one row is that, so it stays a click. The world place is kept
// (scene/parent_system.h), so nothing moves in the view. The drop is read after
// voe_ui_frame_end because only then does a button say it is held and a node
// where it was drawn. A set counts one in `structural`; a full queue sets `full`.
#pragma once

#include "scene.h"

#include <ui/widgets.h>

void voe_editor_scene_list_draw(voe_ui_context *ui, voe_editor_scene *scene);

// Remembers the held row in scene->list_held, and on the first frame none is
// held and `down` is false, drops it at `at`, the root's millimetres. A held
// entity no longer alive, or either one without a transform, does nothing.
void voe_editor_scene_list_drop(voe_editor_scene *scene,
				const voe_ui_context *ui, bool down,
				voe_math_float2 at);
