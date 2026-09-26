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
// NO MOVE: the editor steps nothing (0254). A body's velocity is drained into
// its row and never integrated, so what is edited stays where it is put.
#pragma once

#include <3d/shape_system.h>

#include <ecs/world.h>

// One frame's step of world, the shapes' GPU side read by the shape system.
void voe_editor_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes);
