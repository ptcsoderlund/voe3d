// What the selected entity is made of, and the controls that change it. The
// panel walks the world's component types, asks each one for a row, and expands
// the ones that came with a description (ADR-0132) — so this file names no
// component, includes no component's header, and would list a component written
// tomorrow without being touched.
//
// IT NAMES NOTHING AND THAT IS THE WHOLE CLAIM. `voe_ecs_component_type_count`
// and `_type_at` are the list, `voe_ecs_component_get` says whether the entity
// has one, `voe_ecs_component_key` is the heading — an engine key's
// `voe_<folder>_` dropped and every `_` word capitalised and joined by a space,
// so `voe_3d_shape` reads "Shape" and `follow_camera` "Follow Camera" — and
// `voe_ecs_component_description` is the fields. A type registered runtime-only
// is the engine's own (a mesh, a material) and is not shown at all (ADR-0193). A
// described component whose description this build compiled out is its heading
// and nothing else, which is the honest answer rather than an empty panel: the
// entity has one, and this build cannot see inside it.
//
// WHICH ROWS AN ENTITY HOLDS IS CHANGED HERE TOO, THROUGH THE QUEUE (ADR-0190,
// 0193). Every section has a Remove button except the kept types', which dock.c
// hands in — the identity, because the Scene list is built from it, and the
// camera — and a type another row of the entity needs (0302), which is how the
// transform stays while a shape is there. This panel names none of them.
// Below the sections, Add component opens a list of the top level of
// add_menu.h's tree, drawn and placed as the open dropdown's is, and a group
// row opens its children beside it, to any depth; a second click, a type
// picked, a press outside every list, Escape or another entity drawn closes it.
// A section whose type needs another
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
// A PREFAB'S PART IS SHOWN AND NEVER EDITED (0283 point 5): one line naming its
// root's prefab and the Assets panel, then every field as a label, with no
// Duplicate, Delete, Remove or Add component. Its rows come from the file at
// every read, so an edit would vanish. A copy's root is any entity.
#pragma once

#include <base/arena.h>
#include <base/describe.h>

#include "add_menu.h"

#include <ecs/component.h>
#include <ecs/world.h>

#include <game/project.h>
#include <game/world.h>

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

// How many Remove buttons one frame may record — one per component type: the
// engine's VOE_GAME_WORLD_TYPES (game/world.h) and a project's
// VOE_GAME_PROJECT_TYPES (game/project.h). A type past it gets no button;
// nothing a person does can make one.
#define VOE_EDITOR_INSPECTOR_SECTIONS (VOE_GAME_WORLD_TYPES + VOE_GAME_PROJECT_TYPES)

// How many rows the open list shows: an ENTITY field's None and every authored
// entity (entity_field.h), the longest list there is, so none of those is ever
// left out; a named field's values are far fewer, the shapes' three today.
#define VOE_EDITOR_DROPDOWN_ROWS (1 + VOE_GAME_WORLD_AUTHORED)

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
// scrolling changes neither of the two numbers. Its panel takes the pointer
// (ui/layout.h), so everything inside its outline is the list's: the gaps
// between the rows and the padding at its edges belong to it, and no field,
// button, swatch or number box it covers hovers, highlights or fires through it
// (ADR-0199). The area that clips the list is handed in by dock.c through
// voe_editor_inspector_area_set, because where the list fits is measured
// against that rectangle and this panel never sees the container it is drawn
// inside. The rows sit in a scroll area of their own inside the list's panel,
// at their natural height while the dropdown's `height` is nought and capped to
// it when it is not, so the wheel over a capped list moves the rows within it
// and the list keeps its size and its place (ADR-0200). What the rows wanted is
// read back from that area with voe_ui_node_measured even while it is capped
// (ui/layout.h), which is what lets the read decide whether they would have
// fitted. And what the rows cannot take of a wheel gesture passes outward to
// the panel's own area behind them, as it does between any two nested areas
// (ui/widgets.h): the panel scrolls, the button moves, and the list follows it
// — which is why that is not a hole in ADR-0200 but the same rule twice.
//
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
	// Set, the field is an ENTITY and the list is entity_field.h's
	// choices, each by its label, `names` being NULL; the choice is the
	// voe_ecs_entity at `offset`.
	bool entities;
	// Where the list's top-left corner goes, in millimetres from the
	// top-left of this panel's content column (`content` below) and not
	// from the surface. The column and the button the list hangs from are
	// moved by the same scroll offset, so this pair does not change as the
	// panel scrolls; what it is measured from is the button's rectangle,
	// every frame the list is open (ADR-0199, inspector_edit.h).
	float left;
	float top;
	// How tall the list's rows may be, in millimetres, or nought
	// for as tall as they come. Set only when the list fits neither
	// below the button nor above it: it is then capped to the room
	// on the roomier side and scrolls inside itself, so the last
	// value is still reachable (ADR-0200, inspector_edit.h). It is
	// the rows' own height and not the panel's — the panel is that
	// much plus its own padding and border.
	float height;
} voe_editor_dropdown;

// One control the panel drew, and everything the read needs to turn what the
// pointer did to it back into bytes in a component's row.
//
// `writes` IS WHAT THIS CONTROL PUTS THERE AND NOT WHAT THE FIELD IS. A box on
// one component of a FLOAT3 writes a FLOAT32 at its own offset (of a DOUBLE3, a
// FLOAT64), and a row of a rotation writes the whole QUAT — so the switch that
// builds the bytes reads one word and never has to ask which element of what it
// is on.
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
	// An ENTITY field's button is the same with `names` NULL and `writes`
	// ENTITY, its list the authored entities (entity_field.h).
	const voe_base_field_names *names;
} voe_editor_inspector_control;

// A button that acts on one component type: a section's Remove.
typedef struct {
	voe_ui_node node;
	voe_ecs_type type;
} voe_editor_inspector_type_button;

// Where an open list sits and how tall its rows may be, as
// voe_editor_inspector_overlay_place (inspector_edit.h) works it out each frame:
// millimetres from the top-left of the content column, and nought for rows as
// tall as they come — the three numbers a voe_editor_dropdown carries.
typedef struct {
	float left;
	float top;
	float height;
} voe_editor_inspector_place;

// One row of the open list as this panel drew it: the choice button, and the
// value it names, or in entity mode the entity it names.
typedef struct {
	voe_ui_node node;
	uint32_t value;
	voe_ecs_entity entity;
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
	// VOE_UI_NODE_NONE when they were not: nothing selected, or the
	// camera's entity (ADR-0218), and no Duplicate for the light's
	// entity (ADR-0273).
	voe_ui_node duplicate;
	voe_ui_node remove;
	// Each section's Remove button as drawn this frame, the kept types'
	// having none.
	voe_editor_inspector_type_button
		removes[VOE_EDITOR_INSPECTOR_SECTIONS];
	uint32_t remove_count;
	// Add component's button, VOE_UI_NODE_NONE when not drawn.
	voe_ui_node add_component;
	// Whether its list is open, the entity it opened for and where it
	// sits, kept across frames, as is last frame's primary button, which
	// finds the press that closes a list.
	bool adding;
	voe_ecs_entity adding_for;
	voe_editor_inspector_place adding_at;
	bool pointer_was_down;
	// The open groups, kept across frames: the entry index of the group
	// open at each level, `open_count` of them, and where each one's
	// submenu sits. Level 0's group is a row of the top list.
	uint32_t open_groups[VOE_EDITOR_ADD_MENU_DEPTH - 1];
	voe_editor_inspector_place open_at[VOE_EDITOR_ADD_MENU_DEPTH - 1];
	uint32_t open_count;
	// This frame's menu, built while the list is open on the drawn entity,
	// and the lists as drawn from it: the top one, then one per open
	// group, a level whose group was not drawn left VOE_UI_NODE_NONE.
	voe_editor_add_menu menu;
	voe_editor_add_menu_list menu_lists[VOE_EDITOR_ADD_MENU_DEPTH];
	// What the open list is open on, as it stood when this frame began.
	// Copied out of scene.h's by voe_editor_inspector_frame_begin, so that
	// the panel draws from one value all frame and the read that follows
	// sees the same one; zeroed is a closed list.
	voe_editor_dropdown dropdown;
	// The one column everything this panel draws sits in, and the thing the
	// open list is anchored to. VOE_UI_NODE_NONE when nothing was drawn.
	voe_ui_node content;
	// The scroll area this panel's contents were drawn inside, as
	// dock.c handed it over this frame, and VOE_UI_NODE_NONE when
	// nobody did. voe_ui_node_visible of it is the room the open
	// list has to fit in: that area is what clips the list
	// (ADR-0199), so what is left of the area is exactly what can
	// be seen of anything drawn in it, however far it is scrolled.
	voe_ui_node area;
	// The open list's panel and the area its rows sit in, as drawn
	// this frame, VOE_UI_NODE_NONE when no list was drawn. Kept for
	// the read, which measures the rows against the room the panel
	// leaves (inspector_edit.h) and asks whether a press landed
	// inside the outline.
	voe_ui_node list;
	voe_ui_node list_rows;
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

// Hands over the scroll area this panel's contents are about to be drawn
// inside, for the open list to measure its room against (ADR-0200). Called
// between voe_ui_frame_begin and voe_editor_inspector_draw by whoever opened
// that area, which is dock.c's walk; the node is this frame's, like every other
// one on the struct, and a panel drawn inside nothing that clips hands over
// VOE_UI_NODE_NONE.
void voe_editor_inspector_area_set(voe_editor_inspector *inspector,
				   voe_ui_node area);

// Whether `entity` is a prefab's part: its part row names another entity, the
// copy's root (scene/prefab_component.h). A root names itself and is not one.
// `root`, when not NULL, is set to that root.
bool voe_editor_inspector_is_part(const voe_ecs_world *world,
				  voe_ecs_entity entity, voe_ecs_entity *root);

// Puts the selected entity's components on the panel. Called from inside the
// Inspector panel, so everything it emits is a child of it; an entity that is
// not alive — including nothing selected at all — is one line saying so.
// `kept` holds `kept_count` types whose sections have no Remove button.
void voe_editor_inspector_draw(voe_ui_context *ui,
			       voe_editor_inspector *inspector,
			       voe_ecs_world *world, voe_ecs_entity selected,
			       const voe_ecs_type *kept, uint32_t kept_count);
