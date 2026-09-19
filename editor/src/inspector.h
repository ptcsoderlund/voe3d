// What the selected entity is made of, and the controls that change it. The
// panel walks the world's component types, asks each one for a row, and expands
// the ones that came with a description (ADR-0132) — so this file names no
// component, includes no component's header, and would list a component written
// tomorrow without being touched.
//
// IT NAMES NOTHING AND THAT IS THE WHOLE CLAIM. `voe_ecs_component_type_count`
// and `_type_at` are the list, `voe_ecs_component_get` says whether the entity
// has one, `voe_ecs_component_key` is the heading — its last `_` word,
// capitalised, so `voe_3d_shape` reads "Shape" — and
// `voe_ecs_component_description` is the fields. A type registered runtime-only
// is the engine's own (a mesh, a material) and is not shown at all (ADR-0193). A
// described component whose description this build compiled out is its heading
// and nothing else, which is the honest answer rather than an empty panel: the
// entity has one, and this build cannot see inside it.
//
// WHICH ROWS AN ENTITY HOLDS IS CHANGED HERE TOO, THROUGH THE QUEUE (ADR-0190,
// 0193). Every section has a Remove button except the identity's, which is what
// the Scene list is built from and so is handed in as a type by dock.c rather
// than named here. Below the sections, Add component shows one button per
// described type the entity does not have; a second click, a choice or a press
// anywhere else hides them again. A section whose type needs another
// (`voe_ecs_component_needs`) the entity lacks says "Needs <Heading>". Both go to
// entities.h after the frame, and each success counts one in the scene's
// `structural`, a refusal setting its `full` — Delete's and Duplicate's two
// fields (scene.h).
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
// THE CONTROLS OUTLIVE THE CALL THAT DREW THEM, WHICH IS WHY THIS IS A STRUCT.
// A `ui` widget answers what the pointer did to it only after voe_ui_frame_end
// (ui/widgets.h) and the panel returned long before that — the same reason the
// Scene panel's rows are kept in scene.h. So each control records where its
// bytes live and the read happens afterwards, in the one window between
// voe_ui_frame_end and the rewind of the frame's arena.
//
// AND SO DOES EVERY LABEL'S TEXT, WHICH IS WHY THE ARENA IS IN HERE. A label is
// read at voe_ui_frame_end and not copied, so a number formatted into a buffer
// on this file's stack would be gone by the time it was drawn. Every string this
// panel formats goes into the frame's arena, handed over by
// voe_editor_inspector_frame_begin and valid for exactly as long as the nodes
// are.
//
// DUPLICATE AND DELETE HEAD THE PANEL WHEN AN ENTITY IS SELECTED, recorded like
// every other control and read after the frame by
// voe_editor_inspector_buttons_read, which calls the same scene.h function the
// Delete key and Ctrl+D call. scene.h holds this struct, so it is named here by
// its tag and not by including it.
#pragma once

#include <base/arena.h>
#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <ui/layout.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// How many draggable controls one frame of this panel may put on the screen. A
// field past it is drawn as a label — see the note about controls that lie
// above. Sixty-four is far more than the interface's node budget would let a
// frame reach, three nodes to a control, so it is a floor under the read and
// not a limit anybody is meant to meet.
#define VOE_EDITOR_INSPECTOR_CONTROLS 64

// The widest replace intent this panel will fill in. It is a buffer on the
// stack rather than an allocation because an edit happens between a frame's end
// and its rewind, and nothing that small is worth an arena; a registration
// wider than this asserts, which is the program's own sizing being wrong.
#define VOE_EDITOR_INSPECTOR_INTENT 256

// How many Remove buttons, and how many Add component choices, one frame may
// record — each is one per component type, and a project's world is made with
// room for eight (project.c). A type past it gets no button; nothing a person
// does can make one.
#define VOE_EDITOR_INSPECTOR_SECTIONS 8

// One control the panel drew, and everything the read needs to turn what the
// pointer did to it back into bytes in a component's row.
//
// `writes` IS WHAT THIS CONTROL PUTS THERE AND NOT WHAT THE FIELD IS. A box on
// one component of a FLOAT3 writes a FLOAT32 at its own offset, and a row of a
// rotation writes the whole QUAT — so the switch that builds the bytes reads
// one word and never has to ask which element of what it is on.
typedef struct {
	voe_ui_node node;
	voe_ecs_type type;
	// Bytes from the start of the component's row to the first byte this
	// control writes.
	size_t offset;
	voe_base_field_kind writes;
	// Bytes the field takes, for a CHAR field's text box: what its text is
	// read back into and truncated to. Nought for every other kind.
	size_t size;
	// Which of a rotation's three shown angles this is — 0, 1 or 2 for the
	// world's X, Y and Z — and the angle in degrees it was showing when it
	// was drawn. The edit is the difference between the two, about that
	// axis; no angle is stored anywhere. Both are nought for every other
	// kind.
	uint32_t axis;
	double shown;
} voe_editor_inspector_control;

// A button that acts on one component type: a section's Remove, or one of Add
// component's choices.
typedef struct {
	voe_ui_node node;
	voe_ecs_type type;
} voe_editor_inspector_type_button;

// What the Inspector panel drew this frame. Zeroed is a panel that has drawn
// nothing yet, which is what it is before the first frame.
typedef struct {
	// The frame's arena: where every label's text is formatted, and valid
	// only between voe_editor_inspector_frame_begin and the caller's rewind.
	voe_base_arena *arena;
	// The entity the controls below were drawn for. Kept here so that a
	// click which moves the selection in the same frame cannot send this
	// frame's edit to the newly selected entity.
	voe_ecs_entity entity;
	voe_editor_inspector_control controls[VOE_EDITOR_INSPECTOR_CONTROLS];
	uint32_t control_count;
	// How many replace intents voe_editor_inspector_edits_read submitted
	// this frame. Zeroed by voe_editor_inspector_frame_begin, so a caller
	// reading it after the edits are read sees this frame's count and
	// nothing left over from the last — main.c is that caller, and a
	// non-zero count is what tells session.h an edit reached the project
	// (session.h).
	uint32_t replaced;
	// The Duplicate and Delete buttons as drawn this frame,
	// VOE_UI_NODE_NONE when they were not.
	voe_ui_node duplicate;
	voe_ui_node remove;
	// Each section's Remove button as drawn this frame, the identity's
	// having none.
	voe_editor_inspector_type_button
		removes[VOE_EDITOR_INSPECTOR_SECTIONS];
	uint32_t remove_count;
	// Add component and, while `choosing`, one choice per described type
	// the entity lacks. VOE_UI_NODE_NONE and nought when not drawn.
	voe_ui_node add_component;
	voe_editor_inspector_type_button
		choices[VOE_EDITOR_INSPECTOR_SECTIONS];
	uint32_t choice_count;
	// Whether the choices show. Kept across frames, as is last frame's
	// primary button, which finds the press that hides them.
	bool choosing;
	bool pointer_was_down;
} voe_editor_inspector;

struct voe_editor_scene;

// Forgets last frame's controls and takes this frame's arena. Called between
// voe_ui_frame_begin and the walk, because the nodes and the text below both
// name things that live in that arena and last frame's have gone.
void voe_editor_inspector_frame_begin(voe_editor_inspector *inspector,
				      voe_base_arena *arena);

// Puts the selected entity's components on the panel. Called from inside the
// Inspector panel, so everything it emits is a child of it; an entity that is
// not alive — including nothing selected at all — is one line saying so.
// `identity` is the type the Scene list is built from, whose section has no
// Remove button.
void voe_editor_inspector_draw(voe_ui_context *ui,
			       voe_editor_inspector *inspector,
			       voe_ecs_world *world, voe_ecs_entity selected,
			       voe_ecs_type identity);

// Turns whatever the pointer did to this frame's controls into replace intents.
// Called after voe_ui_frame_end and before the frame's arena is rewound, which
// is the one window in which a widget will answer (ui/widgets.h).
void voe_editor_inspector_edits_read(voe_editor_inspector *inspector,
				     const voe_ui_context *ui,
				     voe_ecs_world *world);

// Carries out whichever of Duplicate and Delete fired this frame, through
// voe_editor_scene_duplicate or voe_editor_scene_delete on `scene`, and
// whichever Remove or Add component choice did, through entities.h on the
// entity they were drawn for — counting one in scene->structural, or setting
// scene->full when refused. Add component toggles the choices, and a press on
// none of those buttons hides them; `down` is the pointer's primary button this
// frame. Called in the same window as voe_editor_inspector_edits_read and before
// the Scene panel's clicks can move the selection the buttons were drawn for.
void voe_editor_inspector_buttons_read(voe_editor_inspector *inspector,
				       const voe_ui_context *ui,
				       struct voe_editor_scene *scene,
				       bool down);
