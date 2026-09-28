// Registering the prefab and prefab part tables (0283 point 2).
//
//     voe_scene_transform_register(world, 64);              // once, at startup
//     voe_scene_prefab_register(world, 64);
//
//     if (!voe_ecs_structure_add(world, prefab, root, &row))
//             ...                                   // the queue is full
//     // next frame: voe_ecs_structure_apply
//
// NEITHER TABLE HAS A REPLACE INTENT OR A MENU PATH, so Add component never
// offers either and the Inspector shows the path without editing it. Which
// prefab a copy is changes only by making or placing one.
//
// ROWS ARE ADDED THROUGH THE STRUCTURAL QUEUE OR BY A LOAD (rule 3): the
// editor's make and place queue them, and the expansion and the scene reader
// are loads. There is no drain and no intent here.
//
// THE PART TABLE IS RUNTIME-ONLY, so no writer ever saves it.
#pragma once

#include <ecs/world.h>
#include <scene/prefab_component.h>

#include <stdint.h>

// Registers both tables at `capacity` rows: the prefab table with its
// description, the default row (an empty path) and the transform it needs; the
// part table runtime-only, needing a transform. Call it once per world, after
// the transform table (it asserts on none).
void voe_scene_prefab_register(voe_ecs_world *world, uint32_t capacity);
