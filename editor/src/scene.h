// Which of the world's authored entities is selected, and the rows the Scene
// panel drew this frame.
//
// THIS FILE DOES NOT BUILD A WORLD. What a fresh project holds is project.h's
// decision and what an opened one holds is authoring/scene_read.h's; main.c and
// session.c set scene.world from the current project's. What this file does to
// it is Add entity, handed to entities.h, which queues the new entity's rows
// (ADR-0193), a fold and a reveal's unfold. session.c writes `selected` too, on
// a NEW, back to a zeroed entity, for the reason the next paragraph gives.
//
// SELECTION BELONGS TO THE EDITOR AND NOT TO THE DOCK TREE. It is held here,
// beside the roots in main.c, and a panel reads it; voe_editor_dock_tree does
// not know it exists and never will. Where a panel sits and what is selected in
// it are two unrelated facts, and a tree that held a selection would be a
// layout a person could not save without saving what they had clicked. A row
// in the Scene panel moves it (voe_editor_scene_clicks_read) and so does a
// left click in a scene view (pick.h), which moves it through
// voe_editor_scene_select — one field with one meaning whichever of the two
// was clicked, and one Inspector showing it.
//
// AN ENTITY THAT NO LONGER EXISTS IS NO SELECTION. The id is kept as it was
// clicked and tested against the world every time it is read, so a destroyed
// entity reads as nothing selected rather than as a stale row — an ecs id
// carries a generation for exactly this (ecs/world.h) and there is nothing to
// clear on a destroy.
//
// THE ROWS THE PANEL DREW OUTLIVE THE CALL THAT DREW THEM, WHICH IS WHY THEY ARE
// IN HERE. A `ui` widget answers what the pointer did to it only after
// voe_ui_frame_end, through the node its begin handed back (ui/widgets.h), and
// the panel that made those nodes returned long before the frame ended. So the
// Scene panel records each button and the entity it names, and the clicks are
// read out afterwards, inside the same frame, by voe_editor_scene_clicks_read.
//
// AND SO DOES WHAT THE INSPECTOR DREW, for the same reason: the Scene and
// Inspector panels are handed this struct and nothing else of the editor's
// (dock.h); inspector.h owns every line of `inspector`. A scene view's panel
// keeps its picture in view.h's struct instead, because a view is not the scene's.
//
// A SELECTION MADE ELSEWHERE IS REVEALED (0355, 0366). A live selection other
// than `revealed` (a view click, Add entity, Duplicate, a drop, undo's re-found
// selection: every outside path at once) is revealed by voe_editor_scene_reveal;
// a row click sets `revealed` too, so the list's own never moves it. It waits
// while the entity's identity is still queued, unfolds every folded ancestor
// (counted in `unfolded`: unsaved but no undo step), scrolls the row to the
// middle of `list_area` a frame later once it is drawn, and gives up when no
// row was drawn.
//
// ADD ENTITY IS ONE BUTTON AND THE ONE WAY TO MAKE SOMETHING (ADR-0217). It
// makes an entity with only an identity (0300); its components come from the
// Inspector's Add component. A fired Add entity
// is an entity added, selected, and counted in `structural`, which main.c reads
// to mark the project unsaved. The Scene list's drag is held here too, across
// frames, in the `list_` fields.
//
// THE GIZMO'S MODE, `rings`, IS THE PERSON'S AND NOT THE PROJECT'S (ADR-0274):
// never saved, never undone, and kept by a new project.
//
// THE ASSETS PANEL'S STATE IS HERE TOO, in `assets`, so the dock reaches it
// with no parameter of its own (assets_panel.h); nothing in scene.c reads it.
#pragma once

#include "assets_panel.h"
#include "inspector.h"

#include <ecs/world.h>
#include <game/world.h>
#include <math/float2.h>
#include <math/float3.h>
#include <ui/layout.h>
#include <ui/theme.h>

#include <stddef.h>
#include <stdint.h>

// How many authored entities the Scene panel will list: the world's identity
// room itself (game/world.h), so the list and the table are one number by
// construction and the list cannot outgrow the array the clicks are read out
// of. The interface's node and element budgets are counted for it (interface.h).
#define VOE_EDITOR_SCENE_ROWS VOE_GAME_WORLD_AUTHORED

// What the colour picker edits while it is open. Zeroed is a closed picker.
typedef struct {
	bool open;
	voe_ecs_entity entity;
	voe_ecs_type type;
	// Bytes from the start of the row to the colour's three floats.
	size_t offset;
	// Where the Inspector column's content began on the surface when the
	// swatch fired, in millimetres from the left: the picker is anchored to
	// end just short of it.
	float left;
} voe_editor_picking;

// One row the Scene panel drew: the button, and the entity it names.
typedef struct {
	voe_ui_node node;
	voe_ecs_entity entity;
	// The row's fold button as drawn, or VOE_UI_NODE_NONE for a row with
	// no children.
	voe_ui_node fold;
} voe_editor_scene_row;

// The editor's scene: the current project's world, what is selected in it,
// and what the Scene panel drew this frame. Zeroed is a scene with no world;
// main.c sets world from the current voe_editor_project as soon as one
// exists.
typedef struct voe_editor_scene {
	voe_ecs_world *world;
	// Zeroed until something is clicked, and a zeroed entity is never a
	// live one (ecs/world.h) — so there is no separate "nothing" flag.
	voe_ecs_entity selected;
	voe_editor_scene_row listed[VOE_EDITOR_SCENE_ROWS];
	uint32_t listed_count;
	// The Add entity button as the panel drew it this frame, or
	// VOE_UI_NODE_NONE when it did not.
	voe_ui_node add;
	// The Scene list's "Scene" heading as drawn this frame, or
	// VOE_UI_NODE_NONE when it was not.
	voe_ui_node heading;
	// How many structural changes this panel and the Inspector's buttons
	// made this frame, a row folded or opened among them. Zeroed with
	// the rows, every frame.
	uint32_t structural;
	// Whether a Delete, Duplicate, Remove or Add component was refused
	// this frame because the world or its queue is full. Zeroed with the rows, every frame.
	bool full;
	// The selection the Scene list last showed, kept across frames and not
	// cleared with the rows: a live selection other than it is revealed.
	voe_ecs_entity revealed;
	// The Scene leaf's scroll area as drawn this frame, or VOE_UI_NODE_NONE.
	voe_ui_node list_area;
	// Parents a reveal opened this frame. They mark the project unsaved but,
	// unlike `structural`, are no undo edit. Zeroed with the rows.
	uint32_t unfolded;
	// The Scene list's drag, owned by scene_list.c, kept across frames and
	// not cleared with the rows; the release zeroes all seven. `list_held` is
	// the row held down (a world swapped under it by New, Open or undo
	// leaves it stale, which the release tests for), `list_from` where the
	// pointer was when it was first held, `list_dragging` whether it has
	// since moved past VOE_EDITOR_SCENE_DRAG_START, `list_cancelled`
	// whether Escape cancelled this press, `list_target` the row a release
	// now would parent onto (zeroed if none) and `list_target_heading`
	// whether a release now would unparent it.
	voe_ecs_entity list_held;
	voe_math_float2 list_from;
	bool list_dragging;
	bool list_cancelled;
	voe_ecs_entity list_target;
	bool list_target_heading;
	// Whether a release now would drop nothing: the release's own answer,
	// worked out after the frame and drawn a frame late, as the target is.
	bool list_refused;
	// The entity a drag released over the Assets panel left for the caller
	// to make a prefab of (interface.c), which clears it; zeroed if none.
	voe_ecs_entity list_made;
	// The themes the Scene list pushes for the held row's dim and the
	// target's rim (ADR-0282), rebuilt by it from the palette each frame.
	// Here because a pushed theme must outlive the frame it is drawn in.
	// `list_row` is the palette at the rows' own spacing, pushed round
	// every entity row.
	voe_ui_theme list_row;
	voe_ui_theme list_dim;
	voe_ui_theme list_rim;
	// What the Inspector panel drew this frame, and the arena its labels
	// were formatted into. Opened and read by interface.c, filled in by
	// inspector.c, and untouched by anything in scene.c.
	voe_editor_inspector inspector;
	// The Assets panel: updated by main.c, drawn by dock.c, read by
	// interface.c (assets_panel.h).
	voe_editor_assets assets;
	// THE COLOUR PICKER'S TARGET IS HERE TOO, IN `picking`, because it
	// outlives the frame the Inspector's swatch fired in and the
	// Inspector's own struct forgets its controls every frame. It names an
	// entity, a component type and the colour's offset in the row, never a
	// component. It is opened by the Inspector (inspector.h) and closed by
	// interface.c on a press outside the picker, by main.c on Escape, and
	// here, on the next ask, when its entity is gone, no longer selected or
	// without the row. Closing changes no colour: every change was already
	// submitted as it happened.
	//
	// The colour picker's target, kept across frames.
	voe_editor_picking picking;
	// THE OPEN DROPDOWN'S TARGET IS HERE TOO, IN `dropdown`, for the
	// picker's reason: it outlives the frame the control fired in and the
	// Inspector's own struct forgets its controls every frame. Its shape is
	// inspector.h's, because the panel that draws the list and reads its
	// rows is the Inspector's; what is here is the one that is open, kept
	// across frames (ADR-0195). It is opened by the Inspector
	// (inspector_edit.h) and closed by interface.c on Escape and by
	// inspector_edit.c on a choice, on a press outside it and on the first
	// frame its field is not on the panel. Where it sits is measured by
	// that same file and set through `voe_editor_scene_dropdown_place`
	// every frame it is open, because an overlay follows its widget rather
	// than remembering where it opened (ADR-0199). How it fits is decided
	// there and then too: the list opens below its button when it fits
	// there, above it when it does not but fits there, and on the roomier
	// side capped and scrolling when it fits neither (ADR-0200) — so it is
	// never drawn with values that cannot be reached, and a list that
	// opened downward flips when the panel scrolls its button toward the
	// bottom edge. Closing chooses nothing: every choice was submitted as
	// it was made. And only one of it and the colour picker is ever open,
	// because opening either closes the other — so one popup shows at a
	// time and Escape means one thing.
	//
	// The open dropdown's target, kept across frames.
	voe_editor_dropdown dropdown;
	// Whether the selection's gizmo is rings that turn it, or arrows that
	// move it when false, so zeroed is move. See the header.
	bool rings;
} voe_editor_scene;

// Which entity is selected, or a zeroed one when nothing is — including when
// what was selected has since been destroyed.
voe_ecs_entity voe_editor_scene_selected(const voe_editor_scene *scene);

// Selects this entity, or clears the selection when it is zeroed or no longer
// alive. The same selection a row in `Scene` moves (voe_editor_scene_clicks_read)
// and the same one the Inspector shows: there is one, and this is how anything
// that is not the Scene panel moves it.
void voe_editor_scene_select(voe_editor_scene *scene, voe_ecs_entity entity);

// Whether this is the selected entity. False for a zeroed `entity` and false
// for one that is not alive, so a caller comparing table rows needs no checks of
// its own.
bool voe_editor_scene_is_selected(const voe_editor_scene *scene,
				  voe_ecs_entity entity);

// Forgets what the Scene panel drew last frame, the Add entity button and the
// list's area among it, and zeroes `structural`, `full` and `unfolded`. Called before the panel draws, because the nodes it
// holds name this frame's tree and last frame's are gone.
void voe_editor_scene_rows_clear(voe_editor_scene *scene);

// Records the Add entity button the panel just drew.
void voe_editor_scene_add_record(voe_editor_scene *scene, voe_ui_node add);

// Records one row the Scene panel just drew, with its fold button or
// VOE_UI_NODE_NONE. Silently keeps nothing past
// VOE_EDITOR_SCENE_ROWS — the identity table is that size, so a further row is
// not something a caller can cause.
void voe_editor_scene_row_add(voe_editor_scene *scene, voe_ui_node node,
			      voe_ecs_entity entity, voe_ui_node fold);

// Moves the selection to whichever recorded row fired this frame, unless the
// Scene list's drag is under way or was cancelled (a release that ends a drag
// is no click; this runs before voe_editor_scene_list_drop zeroes both); flips
// the identity's `folded` of a row whose fold fired, counting one in
// `structural` or setting `full` when the queue refuses it; and carries
// out Add entity: when it fired, adds an entity through entities.h, selects it
// and counts one in `structural`. Called after voe_ui_frame_end and before the
// frame's arena is rewound, which is the one window in which a widget will
// answer (ui/widgets.h).
//
// False when Add entity was refused because the world or its queue is full; the
// caller says "The scene is full." and nothing was counted.
[[nodiscard]] bool voe_editor_scene_clicks_read(voe_editor_scene *scene,
						const voe_ui_context *ui);

// Reveals a selection made elsewhere (see the header): nothing when it is not
// alive, is `revealed` or has no identity row yet; else unfolds its folded
// ancestors and returns, or, none folded, centres its drawn row in `list_area`
// and sets `revealed`. Called after the clicks and the drop are read, in the
// same window as voe_editor_scene_clicks_read.
void voe_editor_scene_reveal(voe_editor_scene *scene, voe_ui_context *ui);

// DELETE AND DUPLICATE ACT ON THE SELECTION, and are this file's so that the
// Delete key and Ctrl+D (main.c) and the Inspector's two buttons (inspector.c)
// are one call each and not two copies. Delete clears the selection and
// Duplicate selects the copy; each success counts one in `structural`, and a
// full world or queue sets `full`, which main.c turns into "The scene is full."
// The Inspector's Remove and Add component count and refuse into the same two
// fields (inspector.h). Both are zeroed with the rows, every frame, which is
// why main.c calls these after the interface has drawn and not before.
//
// Queues the destruction of the selected entity and its tree and clears the
// selection. Nothing selected does nothing, nor does a prefab's part (0283
// point 5; its placed copy's root deletes the tree), nor a tree holding the
// camera's entity: the scene's one camera is never deleted (ADR-0218), from
// either caller. Counts one in `structural`, or sets `full` when the
// queue is full.
void voe_editor_scene_delete(voe_editor_scene *scene);

// Queues a copy of the selected entity (entities.h) and selects the copy.
// Nothing selected does nothing, nor does a prefab's part (0283 point 5) or the
// camera's entity: a copy would be a second camera (ADR-0218), from either
// caller. A light's is copied, a second directional light (0357 point 5). Counts one in `structural`, or sets `full`
// when the world or the queue is full.
void voe_editor_scene_duplicate(voe_editor_scene *scene);

// Flips `rings`: move becomes turn and turn becomes move, whatever is selected.
void voe_editor_scene_gizmo_switch(voe_editor_scene *scene);

// Opens the picker on `picking`, replacing whatever it was open on, and closes
// the dropdown.
void voe_editor_scene_picker_open(voe_editor_scene *scene,
				  voe_editor_picking picking);

// Closes the picker. The colour stays whatever it last became.
void voe_editor_scene_picker_close(voe_editor_scene *scene);

// Whether the picker shows this frame, and the colour in its row when it does.
// Closes it first when its entity is not alive, is no longer the selection or
// no longer has the row — a change of selection is what closes it.
[[nodiscard]] bool voe_editor_scene_picker_showing(voe_editor_scene *scene,
						   voe_math_float3 *colour);

// Opens the dropdown on `dropdown`, replacing whatever it was open on, and
// closes the picker.
void voe_editor_scene_dropdown_open(voe_editor_scene *scene,
				    voe_editor_dropdown dropdown);

// Closes the dropdown. The value stays whatever it last became.
void voe_editor_scene_dropdown_close(voe_editor_scene *scene);

// Moves the open list to `left`, `top` — millimetres inside the Inspector's
// content column (inspector.h) — and caps its rows to `height`, how tall they
// may be, nought for as tall as they come. An overlay is positioned from the
// widget it opened from, every frame it is open and never once when it opened
// (ADR-0199), and the panel is the only thing that knows where its button
// sits; so inspector_edit.h measures it and this is where it lands. All three
// are worked out afresh there every frame the list is open, because an overlay
// is placed where it fits and the fit changes as the panel scrolls (ADR-0200).
// Does nothing while the list is closed.
void voe_editor_scene_dropdown_place(voe_editor_scene *scene, float left,
				     float top, float height);
