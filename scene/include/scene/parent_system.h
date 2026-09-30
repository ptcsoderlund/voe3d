// Registering the parent table, and the one call that parents: it keeps the
// child's world place under its new parent (0281 point 4).
//
//     voe_scene_transform_register(world, 64);              // once, at startup
//     voe_scene_parent_register(world, 64);
//
//     if (voe_scene_parent_within(world, hull, turret))     // a loop: refuse
//             return;
//     if (!voe_scene_parent_set(world, turret, hull))
//             ...                                   // a queue is full
//     // next frame: voe_ecs_structure_apply, then the transform system
//
// REGISTERING GIVES A WORLD ITS TREE. A world without the table has only roots,
// and every read in parent_component.h and transform_component.h answers as if
// nothing had a parent.
//
// THE TABLE HAS NO REPLACE INTENT AND NO MENU PATH (0281 point 1), so no tool
// writes the field directly; parenting lands here instead. The editor's drag,
// the Inspector's Remove and game code while playing all come through
// voe_scene_parent_set (0271).
//
// REMOVE-THEN-ADD IS ONE CHANGE because the structural queue applies in
// submission order (ecs/structure.h): the old row goes, the new one lands, and a
// root simply has nothing to remove.
//
// THE WORLD PLACE IS READ AT THE SUBMIT. The intent holds the row that keeps the
// child where it is now, so the caller submits no other move of that child in
// the same frame; the later of the two would win with a row meant for the
// other parent.
//
// PARENTING INTO ONE'S OWN TREE ASSERTS. It would make a loop the walks only
// survive by their cap, and only the caller knows what refusing looks like, so a
// caller that cannot rule it out asks voe_scene_parent_within first.
#pragma once

#include <ecs/world.h>
#include <scene/parent_component.h>

#include <stdint.h>

// Registers the table, its description and the default row (no parent). Call it
// once per world, after the transform table (it asserts on none). capacity is
// how many entities may have a parent.
void voe_scene_parent_register(voe_ecs_world *world, uint32_t capacity);

// Queues the removal of the child's parent row, then for a non-zeroed `parent`
// an add naming it, then a transform intent whose row keeps the child's world
// place under it. A zeroed parent unparents. Either may lack a transform
// (0300): a bare child gets no transform intent, and a child under a bare
// parent gets its world place as it is now as its row. False when a queue is
// full; what was already queued stands. Asserts the table is registered, a
// non-zeroed parent is alive, and the parent is not the child or under it.
[[nodiscard]] bool voe_scene_parent_set(voe_ecs_world *world,
					voe_ecs_entity child,
					voe_ecs_entity parent);
