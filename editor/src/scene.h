// The scene the editor opens on, and which of its entities is selected.
//
// THE SCENE IS BUILT IN CODE BECAUSE THERE IS NO LOADER YET. Four entities, put
// here by hand so that the list, the selection and the inspector have something
// real to stand on before a file format does. When a loader arrives this file is
// what it replaces, and nothing that reads a scene through the world has to
// change — which is why the panels below read the identity table and never this
// struct's entities.
//
// THREE OF THE FOUR ARE AUTHORED AND THE FOURTH IS THE POINT. `Cube`, `Cube_2`
// and `Marker` each have an identity and a transform; the fourth has a transform
// and NO identity, the way a probe or a gizmo the engine built for itself does.
// It is in the world, it is drawn by anything that walks transforms, and it must
// not appear in the Scene panel — because an identity's presence is the whole of
// what "authored" means (ADR-0125) and the list is the identity table and
// nothing else. A list that showed four is this card's failure and
// voe_editor_scene_build asserts the two counts rather than leaving it to a
// person to count names on a screen.
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
#pragma once

#include <ecs/world.h>
#include <ui/layout.h>

#include <stdint.h>

// How many authored entities the Scene panel will list, and therefore how many
// identities the world is given room for — the two are the same number on
// purpose, so that the list cannot outgrow the array the clicks are read out of.
// Thirty-two is far more than the four this file builds and far fewer than the
// interface's node budget would refuse.
#define VOE_EDITOR_SCENE_ROWS 32

// One row the Scene panel drew: the button, and the entity it names.
typedef struct {
	voe_ui_node node;
	voe_ecs_entity entity;
} voe_editor_scene_row;

// The editor's scene: the world it authors into, what is selected in it, and
// what the Scene panel drew this frame. Zeroed is a scene with no world, which
// voe_editor_scene_build is what fills in.
typedef struct {
	voe_ecs_world *world;
	// Zeroed until something is clicked, and a zeroed entity is never a
	// live one (ecs/world.h) — so there is no separate "nothing" flag.
	voe_ecs_entity selected;
	voe_editor_scene_row listed[VOE_EDITOR_SCENE_ROWS];
	uint32_t listed_count;
} voe_editor_scene;

// Builds the four entities into `world` and points `scene` at it. The world must
// already have had voe_scene_transform_register and voe_scene_identity_register
// called on it. A world too small to hold four entities, or four transforms, or
// three identities, is this program's own sizing being wrong and asserts.
void voe_editor_scene_build(voe_editor_scene *scene, voe_ecs_world *world);

// Which entity is selected, or a zeroed one when nothing is — including when
// what was selected has since been destroyed.
voe_ecs_entity voe_editor_scene_selected(const voe_editor_scene *scene);

// Whether this is the selected entity. False for a zeroed `entity` and false
// for one that is not alive, so a caller comparing table rows needs no checks of
// its own.
bool voe_editor_scene_is_selected(const voe_editor_scene *scene,
				  voe_ecs_entity entity);

// Forgets what the Scene panel drew last frame. Called before the panel draws,
// because the nodes it holds name this frame's tree and last frame's are gone.
void voe_editor_scene_rows_clear(voe_editor_scene *scene);

// Records one row the Scene panel just drew. Silently keeps nothing past
// VOE_EDITOR_SCENE_ROWS — the identity table is that size, so a further row is
// not something a caller can cause.
void voe_editor_scene_row_add(voe_editor_scene *scene, voe_ui_node node,
			      voe_ecs_entity entity);

// Moves the selection to whichever recorded row fired this frame. Called after
// voe_ui_frame_end and before the frame's arena is rewound, which is the one
// window in which a widget will answer (ui/widgets.h).
void voe_editor_scene_clicks_read(voe_editor_scene *scene,
				  const voe_ui_context *ui);
