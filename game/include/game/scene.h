// The one function a project's cooked scene defines, and everything its body
// names: the eight types' rows and keys (game/world.h) and
// voe_ecs_component_add. This is the include the cook is handed
// (authoring/scene_cook.h).
//
//     voe_ecs_world *world = voe_game_world_new(arena);
//     if (!voe_game_scene_build(world))
//             ...                        // the world had no room for it
//
// DEFINED BY THE COOKED scene.c IN A PROJECT'S GAME TREE, NOT BY THIS FOLDER
// (0237). The symbol is resolved at link time there, with no function pointer
// (ADR-0135); game/src/run.c is the only caller, and nothing in this folder
// or its tests links without a scene.c beside it that names it.
//
// Constraints: the world is one voe_game_world_new made and holds nothing
// yet. The cooked body creates one entity per authored entity and adds its
// rows, and edits nothing it did not just make.
#pragma once

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <scene/camera_component.h>
#include <scene/identity_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>

#include <stdbool.h>

// Builds the cooked scene into `world`. False when an entity or a row did not
// fit, which is the world's room refusing and not the scene's fault.
[[nodiscard]] bool voe_game_scene_build(voe_ecs_world *world);
