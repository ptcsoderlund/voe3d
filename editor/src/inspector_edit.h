// What the pointer did to this frame's controls, turned into replace intents and
// into scene.h's calls. inspector.c draws the panel and records its controls;
// every function in here is called afterwards, between voe_ui_frame_end and the
// rewind of the frame's arena, which is the one window in which a widget will
// answer (ui/widgets.h).
//
//     voe_editor_inspector_edits_read(&inspector, ui, world);
//     voe_editor_inspector_buttons_read(&inspector, ui, scene, down, at);
//
// AN EDIT IS A REPLACE INTENT AND NEVER A WRITE (ADR-0134 point 4). A control
// that moved does not touch the table: the row is read, copied into a zeroed
// intent whose entity sits at offset zero, the field's bytes are overwritten,
// and the intent is submitted for the owning system to drain. The editor never
// calls voe_ecs_component_set — the whole point of rule 4 is that a tool which
// knows nothing about a component cannot be the thing that writes it. A number
// typed into a box (ADR-0192) is the same `changed` a drag is and goes the same
// way, so a rotation's angle, a whole number's rounding and a read-only label
// behave alike for both. A CHAR array (rank 1) is a `ui` text field, which is
// how the name is edited: on `committed` with text that differs from the row,
// the text is copied into the row's bytes, truncated to leave room for its
// terminating zero, and submitted the same way. Tab walks the fields and boxes
// in the order they are drawn — the name, then each number.
//
// A COMPONENT WITH NO REPLACE INTENT IS SHOWN AND NOT EDITED, which is what
// ecs/component.h says such a type is for. Its fields are labels, because the
// alternative is a box that drags and changes nothing — and a control that lies
// is worse than a number a person can read and not touch. A field marked
// read-only is a label for the same reason (ADR-0139 point 2), whatever its kind.
//
// DUPLICATE AND DELETE HEAD THE PANEL WHEN AN ENTITY IS SELECTED, recorded like
// every other control and read after the frame by
// voe_editor_inspector_buttons_read, which calls the same scene.h function the
// Delete key and Ctrl+D call. A fired dropdown control is read there too and
// writes nothing itself: it opens the list through scene.h. So is the open list
// itself, every frame it is open. A row of it that fired is submitted at once as
// the field's value, the way the picker's colour is, and closes the list; a
// press that fired no row and no dropdown control closes it, on the press and
// not the release, only when it landed outside the list's visible rectangle,
// because everything inside that outline is the list's — the gaps between its
// rows, the padding at its edges, and the scrollbar a capped one has (ADR-0199);
// and a frame in which it was open and drew no rows closes it too, that being
// the frame its field left the panel — nothing selected, another entity
// selected, or the component gone. Where it sits is worked out from its button's
// rectangle and the room the panel's scroll area leaves round it — below the
// button when the whole list fits there, above it when it fits there instead,
// and on the roomier side capped to that room and scrolling when it fits neither
// (ADR-0200) — and set through scene.h every frame it is open, because an
// overlay is placed where it fits each frame and never once when it opened
// (ADR-0199). That arithmetic is the button's rectangle less the Inspector's
// content column's, so it is in that column's space and says nothing about how
// far the panel is scrolled. It is voe_editor_inspector_overlay_place, and Add
// component's list is placed by it too: that list opens and closes as the
// open list does, and a fired type's row adds the type (inspector.h). A group
// row opens its submenu, closing every group open at its level or deeper. A
// submenu sits right of its list when its whole width fits before the right of
// the area's visible rectangle, both in the surface's millimetres, else left of
// it, and is then moved wholly inside that rectangle; its top is its row's,
// fitted by the side-and-cap rule with the row as the widget (ADR-0221).
// scene.h holds this struct, so it is named by its tag.
#pragma once

#include "inspector.h"

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float2.h>
#include <math/float3.h>

#include <ui/layout.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct voe_editor_scene;

// Turns whatever the pointer did to this frame's controls into replace intents.
// Called after voe_ui_frame_end and before the frame's arena is rewound, which
// is the one window in which a widget will answer (ui/widgets.h).
void voe_editor_inspector_edits_read(voe_editor_inspector *inspector,
				     const voe_ui_context *ui,
				     voe_ecs_world *world);

// Carries out whichever of Duplicate and Delete fired this frame, through
// voe_editor_scene_duplicate or voe_editor_scene_delete on `scene`, and
// whichever Remove or Add component choice did, through entities.h on the
// entity they were drawn for, and opens the picker on whichever swatch did — counting one in scene->structural, or setting
// scene->full when refused. Add component toggles its menu, and once it is open
// only these close it (ADR-0221): a type row fired, which adds it and closes
// all; another group row fired, which closes that level and deeper; a press
// outside every open list's visible rectangle and the button; Escape, through
// voe_editor_inspector_add_close; and a frame that drew another entity than the
// one it opened for. `down` is the pointer's primary button this frame, a
// level, and a press is its down edge against last frame's, never the level;
// `at` is where the pointer is this frame, in the surface's millimetres. It takes the place as well as the button because a press is only
// the press on nothing that closes the open list when it landed outside that
// list. Called in the same window as voe_editor_inspector_edits_read and before
// the Scene panel's clicks can move the selection the buttons were drawn for.
void voe_editor_inspector_buttons_read(voe_editor_inspector *inspector,
				       const voe_ui_context *ui,
				       struct voe_editor_scene *scene,
				       bool down, voe_math_float2 at);

// Where a list hanging from `button` goes this frame, by the side-and-cap rule
// above (ADR-0200): `list` is its panel and `rows` the area inside it as drawn
// this frame, `list` VOE_UI_NODE_NONE for one not drawn yet, which goes below
// uncapped. Needs this frame's content column.
voe_editor_inspector_place
voe_editor_inspector_overlay_place(const voe_editor_inspector *inspector,
				   const voe_ui_context *ui, voe_ui_node button,
				   voe_ui_node list, voe_ui_node rows);

// Closes Add component's list: Escape's, which interface.c reads.
void voe_editor_inspector_add_close(voe_editor_inspector *inspector);

// Submits `colour` as the replace intent of `type`'s row on `entity`, its three
// floats at `offset`, and counts one in `replaced`. The picker's change, read
// by interface.c in the same window as the edits. An entity no longer alive, or
// without the row, submits and counts nothing past the check.
void voe_editor_inspector_colour_submit(voe_editor_inspector *inspector,
					voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_ecs_type type, size_t offset,
					voe_math_float3 colour);

// Submits `value` as the replace intent of `type`'s row on `entity`, the
// uint32_t at `offset`, and counts one in `replaced`. The open list's choice,
// read by voe_editor_inspector_buttons_read above off the rows this frame drew; what it submits is the
// index into the field's names, because entry `i` names value `i` (ADR-0198). An
// entity no longer alive, or without the row, submits and counts nothing past
// the check.
void voe_editor_inspector_named_submit(voe_editor_inspector *inspector,
				       voe_ecs_world *world,
				       voe_ecs_entity entity,
				       voe_ecs_type type, size_t offset,
				       uint32_t value);
