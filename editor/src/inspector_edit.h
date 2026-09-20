// What the pointer did to this frame's controls, turned into replace intents and
// into scene.h's calls. inspector.c draws the panel and records its controls;
// every function in here is called afterwards, between voe_ui_frame_end and the
// rewind of the frame's arena, which is the one window in which a widget will
// answer (ui/widgets.h).
//
//     voe_editor_inspector_edits_read(&inspector, ui, world);
//     voe_editor_inspector_buttons_read(&inspector, ui, scene, down);
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
// press that is on neither those rows nor a dropdown control closes it, on the
// press and not the release; and a frame in which it was open and drew no rows
// closes it too, that being the frame its field left the panel — nothing
// selected, another entity selected, or the component gone. Where it sits is
// measured from its button's rectangle and set through scene.h every frame it is
// open, because an overlay is positioned from the widget it belongs to each
// frame and never once when it opened (ADR-0199). That arithmetic is the
// button's rectangle less the Inspector's content column's, so it is in that
// column's space and says nothing about how far the panel is scrolled. Which
// side it opens on and how tall its rows may be are worked out in the same
// step and from the same frame's rectangles: below its button when the whole
// list fits in what is left of the panel's scroll area, above it when it does
// not but fits there, and on the roomier side capped and scrolling when it
// fits neither (ADR-0200). scene.h
// holds this struct, so it is named here by its tag and not by including it.
#pragma once

#include "inspector.h"

#include <ecs/component.h>
#include <ecs/world.h>

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
// scene->full when refused. Add component toggles the choices, and a press on
// none of those buttons hides them; `down` is the pointer's primary button this
// frame. Called in the same window as voe_editor_inspector_edits_read and before
// the Scene panel's clicks can move the selection the buttons were drawn for.
void voe_editor_inspector_buttons_read(voe_editor_inspector *inspector,
				       const voe_ui_context *ui,
				       struct voe_editor_scene *scene,
				       bool down);

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
