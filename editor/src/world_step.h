// The world's step once a frame: one call to voe_game_world_step
// (game/frame.h), which applies the structural queue, then the project's
// replace intents, then runs every owning system, the collider's and the
// body's drains among them. main.c calls it once a frame, after an undo step
// is taken and before anything reads the world.
//
// THE ORDER IS GAME'S NOW. The editor's world and a game's register the same
// types (0237), so they drain them in the same order, and the one place that
// order is written is game/frame.h. Which rows exist still changes before any
// system reads a table (ADR-0193), and every owning system still runs every
// frame whether anything submitted or not (ADR-0134 point 7).
//
// NO MOVE: the editor steps no body (0254). A body's velocity is drained into
// its row and never integrated, so what is edited stays where it is put.
//
// EMITTERS RUN WHILE EDITING (0298 point 4), so an effect is seen as it is
// tuned: voe_3d_emitter_system_run gets `seconds` after game's step. It is the
// one thing the editor steps by time; `seconds` of 0 spawns and moves nothing.
//
// PLACED COPIES ARE EXPANDED AFTER THE STEP (prefabs.h, 0283 point 4). A root
// queued this frame — a drop, a duplicate, a made prefab — exists only once the
// step has applied the queue, and expanding is a load that must come before
// anything reads the world, so it goes after the step and before any draw.
#pragma once

#include "notice.h"

#include <3d/shape_system.h>

#include <base/arena.h>

#include <ecs/world.h>

// One frame's step of world, the shapes' GPU side read by the shape system,
// its emitters run `seconds` on (>= 0), then its placed copies expanded from
// folder (NULL: none) with scratch, a copy that will not expand said in why.
void voe_editor_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes,
			   float seconds, const char *folder,
			   voe_base_arena *scratch, voe_editor_notice *why);
