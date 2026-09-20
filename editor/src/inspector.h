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
// A COLOUR IS A SWATCH (ADR-0192), inside a button when the type has a replace
// intent and the field is not read-only, bare otherwise. A fired swatch opens
// the colour picker on that entity, type and offset — scene.h's `picking` — and
// what the picker changes comes back through
// voe_editor_inspector_colour_submit, the same replace intent and the same
// count in `replaced` a dragged number is.
//
// A NAMED FIELD IS A DROPDOWN (ADR-0195, 0198). A field whose description
// carries names is shown by the name of the value it holds and not by its
// number, inside a button when the type has a replace intent and the field is
// not read-only and as a plain label otherwise, for the same reason a read-only
// number is a label. A value no entry names is shown as the number it is, which
// is what an older build seeing a newer file's kind shows. The button only opens
// the list, the choice arriving later through
// voe_editor_inspector_named_submit. What a fired button opens the list on is
// the `voe_editor_dropdown` below, whose shape is this file's because this panel
// is what draws the list and reads what was picked from it, and scene.h holds
// the one that is open and says who may open and close it.
//
// THE OPEN LIST IS DRAWN ON THIS PANEL AND NOT OVER IT (ADR-0199). An overlay
// belongs to the widget it opened from, so the list is an anchored child of this
// panel's own content column and is scrolled and clipped with it: when the
// button scrolls out of the panel the list goes with it instead of floating over
// the editor. It is emitted after every section because submission order is
// paint order (ui/layout.h), and a list emitted beside its button would be
// painted over by the rows below it. Its offset is in that column's space, so
// scrolling changes neither of the two numbers.
#pragma once

#include <base/arena.h>
#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>

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

// How many of a named field's values the open list shows. A further one gets no
// row; the shapes' three are what there is today.
#define VOE_EDITOR_DROPDOWN_ROWS 16

// What the open dropdown chooses among. Zeroed is a closed one.
typedef struct {
	bool open;
	voe_ecs_entity entity;
	voe_ecs_type type;
	// Bytes from the start of the row to the field's uint32_t.
	size_t offset;
	// The field's value names (base/describe.h): entry i names value i. A
	// table of the declaring folder's own, static and so outliving every
	// frame.
	const voe_base_field_names *names;
	// Where the list's top-left corner goes, in millimetres from the
	// top-left of this panel's content column (`content` below) and not
	// from the surface. The column and the button the list hangs from are
	// moved by the same scroll offset, so this pair does not change as the
	// panel scrolls; what it is measured from is the button's rectangle,
	// every frame the list is open (ADR-0199, inspector_edit.h).
	float left;
	float top;
} voe_editor_dropdown;

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
	// The value names of a named field (base/describe.h), NULL for every
	// other control. Set, this control is the dropdown's closed button and
	// writes nothing itself: a fired one opens the list
	// (inspector_edit.h), and `writes` is the UINT32 the chosen value is.
	const voe_base_field_names *names;
} voe_editor_inspector_control;

// A button that acts on one component type: a section's Remove, or one of Add
// component's choices.
typedef struct {
	voe_ui_node node;
	voe_ecs_type type;
} voe_editor_inspector_type_button;

// One row of the open list as this panel drew it: the choice button, and the
// value it names.
typedef struct {
	voe_ui_node node;
	uint32_t value;
} voe_editor_dropdown_row;

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
	// What the open list is open on, as it stood when this frame began.
	// Copied out of scene.h's by voe_editor_inspector_frame_begin, so that
	// the panel draws from one value all frame and the read that follows
	// sees the same one; zeroed is a closed list.
	voe_editor_dropdown dropdown;
	// The one column everything this panel draws sits in, and the thing the
	// open list is anchored to. VOE_UI_NODE_NONE when nothing was drawn.
	voe_ui_node content;
	// The open list's rows as drawn this frame, kept for the read for the
	// reason every other control here is.
	voe_editor_dropdown_row rows[VOE_EDITOR_DROPDOWN_ROWS];
	uint32_t row_count;
} voe_editor_inspector;

// Forgets last frame's controls and takes this frame's arena. Called between
// voe_ui_frame_begin and the walk, because the nodes and the text below both
// name things that live in that arena and last frame's have gone.
// `dropdown` is what the open list is open on, NULL saying a closed one.
void voe_editor_inspector_frame_begin(voe_editor_inspector *inspector,
				      voe_base_arena *arena,
				      const voe_editor_dropdown *dropdown);

// Puts the selected entity's components on the panel. Called from inside the
// Inspector panel, so everything it emits is a child of it; an entity that is
// not alive — including nothing selected at all — is one line saying so.
// `identity` is the type the Scene list is built from, whose section has no
// Remove button.
void voe_editor_inspector_draw(voe_ui_context *ui,
			       voe_editor_inspector *inspector,
			       voe_ecs_world *world, voe_ecs_entity selected,
			       voe_ecs_type identity);
