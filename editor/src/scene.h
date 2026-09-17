// The untitled scene the editor opens on, and which of its entities is
// selected.
//
// THE UNTITLED SCENE IS BUILT IN CODE, BECAUSE IT IS NOT A FILE. It is what New
// and a first start make — one cube and the light that shows it — the same two
// entities every time, put here by hand rather than read off disk; a saved
// project's scene is `authoring`'s to read, and comes into this same world
// through a different call than this one.
//
// TWO ENTITIES AND BOTH ARE AUTHORED. `Cube` has an identity, a transform at the
// origin and a `voe_3d_shape` of kind cube; `Light` has an identity and a
// `voe_scene_light` and no transform — a directional light has no position to
// hold one for. voe_editor_scene_untitled asserts the identity count rather than
// leaving it to a person to count names on a screen.
//
// THE CUBE IS DRAWN BY THE SHAPE SYSTEM AND NOT BY THIS FILE. Giving `Cube` a
// `voe_3d_shape` is the whole of what this file does towards it being on screen;
// turning a shape into a mesh and a material is `3d/shape_system.h`'s, run once
// a frame by the loop, and this file names neither a mesh nor a material.
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

// One row the Scene panel drew: the button, and the entity it names.
typedef struct {
	voe_ui_node node;
	voe_ecs_entity entity;
} voe_editor_scene_row;

// The editor's scene: the world it authors into, what is selected in it, and
// what the Scene panel drew this frame. Zeroed is a scene with no world, which
// voe_editor_scene_untitled is what fills in.
typedef struct {
	voe_ecs_world *world;
	// Zeroed until something is clicked, and a zeroed entity is never a
	// live one (ecs/world.h) — so there is no separate "nothing" flag.
	voe_ecs_entity selected;
	voe_editor_scene_row listed[VOE_EDITOR_SCENE_ROWS];
	uint32_t listed_count;
	// What the Inspector panel drew this frame, and the arena its labels
	// were formatted into. Opened and read by interface.c, filled in by
	// inspector.c, and untouched by anything in scene.c.
	voe_editor_inspector inspector;
} voe_editor_scene;

// Builds the untitled scene's two entities into `world` and points `scene` at
// it. The world must already have had voe_scene_transform_register,
// voe_scene_identity_register, voe_scene_light_register and
// voe_3d_shape_register called on it. A world too small to hold two entities,
// one transform, two identities, one light or one shape is this program's own
// sizing being wrong and asserts.
//
// Neither uploads nor touches the device — the shape system does that once, at
// startup, and turns `Cube`'s shape into a mesh and a material the first time it
// runs — so this cannot fail.
void voe_editor_scene_untitled(voe_editor_scene *scene, voe_ecs_world *world);

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
