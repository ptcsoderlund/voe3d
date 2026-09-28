// Registering the parent table, and (from card 03 on) the one call that
// parents: it keeps the child's world place under its new parent.
//
//     voe_scene_transform_register(world, 64);              // once, at startup
//     voe_scene_parent_register(world, 64);
//
// REGISTERING GIVES A WORLD ITS TREE. A world without the table has only roots,
// and every read in parent_component.h and transform_component.h answers as if
// nothing had a parent.
//
// THE TABLE HAS NO REPLACE INTENT AND NO MENU PATH (0281 point 1), so no tool
// writes the field directly; parenting lands here instead.
#pragma once

#include <ecs/world.h>
#include <scene/parent_component.h>

#include <stdint.h>

// Registers the table, its description, the default row (no parent) and the
// transform it needs. Call it once per world, after the transform table (it
// asserts on none). capacity is how many entities may have a parent.
void voe_scene_parent_register(voe_ecs_world *world, uint32_t capacity);
