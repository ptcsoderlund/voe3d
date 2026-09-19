// Which of the world's authored entities is selected, and the rows the Scene
// panel drew this frame.
//
// THIS FILE DOES NOT BUILD A WORLD. What a fresh project holds is project.h's
// decision — the untitled scene's two entities among them — and what an opened
// one holds is authoring/scene_read.h's; scene.world is set from whichever the
// current project's world is (main.c, and session.c when a NEW goes ahead
// replaces it). The one thing this file does to it is the Add menu's choice,
// handed to entities.h, which queues the new entity's rows (ADR-0193). session.c writes `selected` too, on that same NEW, back to a zeroed
// entity — the one other place outside this file that touches either field,
// and for the reason the next paragraph gives.
//
// SELECTION BELONGS TO THE EDITOR AND NOT TO THE DOCK TREE. It is held here,
// beside the roots in main.c, and a panel reads it; voe_editor_dock_tree does
// not know it exists and never will. Where a panel sits and what is selected in
// it are two unrelated facts, and a tree that held a selection would be a
// layout a person could not save without saving what they had clicked.
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
// AND SO DOES WHAT THE INSPECTOR DREW, WHICH IS THE SAME SENTENCE AND IS WHY IT
// IS IN HERE TOO. The Scene and Inspector panels are handed this struct and
// nothing else of the editor's (dock.h), so this is where they keep what they
// have to be asked about after the frame; the inspector's own is one field below
// and inspector.h owns every line of what is in it. A scene view's panel keeps
// its picture in view.h's struct instead, because a view is not the scene's.
//
// THE ADD MENU IS THE SCENE PANEL'S, AND SO IS WHETHER IT SHOWS. Add toggles
// `adding`; while it is set the panel draws the four choices under it. A press
// that starts on none of those five buttons hides them — which is why the
// clicks are read with the pointer's button, this file keeping last frame's to
// find the press. A choice made is an entity added, selected, and counted in
// `structural`, which main.c reads to mark the project unsaved.
#pragma once

#include "inspector.h"

#include <ecs/world.h>
#include <ui/layout.h>

#include <stdint.h>

// How many authored entities the Scene panel will list, and therefore how many
// identities the world is given room for — the two are the same number on
// purpose, so that the list cannot outgrow the array the clicks are read out of.
// Thirty-two is far more than the two this file builds and far fewer than the
// interface's node budget would refuse.
#define VOE_EDITOR_SCENE_ROWS 32

// Entity, Cube, Capsule and Cylinder.
#define VOE_EDITOR_SCENE_ADD_CHOICES 4

// One row the Scene panel drew: the button, and the entity it names.
typedef struct {
	voe_ui_node node;
	voe_ecs_entity entity;
} voe_editor_scene_row;

// The editor's scene: the current project's world, what is selected in it,
// and what the Scene panel drew this frame. Zeroed is a scene with no world;
// main.c sets world from the current voe_editor_project as soon as one
// exists.
typedef struct {
	voe_ecs_world *world;
	// Zeroed until something is clicked, and a zeroed entity is never a
	// live one (ecs/world.h) — so there is no separate "nothing" flag.
	voe_ecs_entity selected;
	voe_editor_scene_row listed[VOE_EDITOR_SCENE_ROWS];
	uint32_t listed_count;
	// The Add button and its four choices as the panel drew them this
	// frame, VOE_UI_NODE_NONE for any it did not. The choices are in
	// voe_editor_add's order (entities.h).
	voe_ui_node add;
	voe_ui_node add_choices[VOE_EDITOR_SCENE_ADD_CHOICES];
	// Whether the four choices show under Add.
	bool adding;
	// Last frame's primary button, so a press is found as an edge.
	bool pointer_was_down;
	// How many structural changes the panel made this frame. Zeroed with
	// the rows, every frame.
	uint32_t structural;
	// What the Inspector panel drew this frame, and the arena its labels
	// were formatted into. Opened and read by interface.c, filled in by
	// inspector.c, and untouched by anything in scene.c.
	voe_editor_inspector inspector;
} voe_editor_scene;

// Which entity is selected, or a zeroed one when nothing is — including when
// what was selected has since been destroyed.
voe_ecs_entity voe_editor_scene_selected(const voe_editor_scene *scene);

// Whether this is the selected entity. False for a zeroed `entity` and false
// for one that is not alive, so a caller comparing table rows needs no checks of
// its own.
bool voe_editor_scene_is_selected(const voe_editor_scene *scene,
				  voe_ecs_entity entity);

// Forgets what the Scene panel drew last frame, the Add menu's buttons among it,
// and zeroes `structural`. Called before the panel draws, because the nodes it
// holds name this frame's tree and last frame's are gone.
void voe_editor_scene_rows_clear(voe_editor_scene *scene);

// Records the Add button and, when `adding`, the four choices the panel just
// drew — `choices` is VOE_EDITOR_SCENE_ADD_CHOICES long, or NULL when they were
// not drawn.
void voe_editor_scene_add_menu_record(voe_editor_scene *scene, voe_ui_node add,
				      const voe_ui_node *choices);

// Records one row the Scene panel just drew. Silently keeps nothing past
// VOE_EDITOR_SCENE_ROWS — the identity table is that size, so a further row is
// not something a caller can cause.
void voe_editor_scene_row_add(voe_editor_scene *scene, voe_ui_node node,
			      voe_ecs_entity entity);

// Moves the selection to whichever recorded row fired this frame, and carries
// out the Add menu: Add toggles the choices, a choice adds that entity through
// entities.h, selects it, hides the choices and counts one in `structural`, and
// a press on none of the five hides the choices. `down` is the pointer's
// primary button this frame. Called after voe_ui_frame_end and before the
// frame's arena is rewound, which is the one window in which a widget will
// answer (ui/widgets.h).
//
// False when a choice was refused because the world or its queue is full; the
// caller says "The scene is full." and nothing was counted.
[[nodiscard]] bool voe_editor_scene_clicks_read(voe_editor_scene *scene,
						const voe_ui_context *ui,
						bool down);
