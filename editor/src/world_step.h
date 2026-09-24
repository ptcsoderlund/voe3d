// The world's step once a frame: the structural queue applied, then the
// transform, identity and light systems, then the shape system. main.c calls
// it once a frame, after an undo step is taken and before anything reads the
// world.
//
// WHICH ROWS EXIST CHANGES HERE AND NOWHERE ELSE IN THE FRAME (ADR-0193,
// ecs/structure.h), before any system reads a table, so a row queued last
// frame is there for every one of them and none sees one appear partway
// through its run.
//
// EVERY OWNING SYSTEM RUNS EVERY FRAME, WHETHER ANYTHING SUBMITTED OR NOT
// (ADR-0134 point 7). An intent that reaches a queue on a frame its system
// does not drain is an edit that lands whenever the loop next happens to run
// it, which is a class of bug that does not exist if the run is unconditional
// — the Inspector can submit a replace intent for any editable field the
// moment it draws one, transform's and light's alike, so all three are run
// from the start rather than from whenever somebody remembers a first submit
// needs one. The shape system drains the shape's intent the same way and
// gives a fresh shape its mesh and material the first frame it exists
// (3d/shape_system.h).
#pragma once

#include <3d/shape_system.h>

#include <ecs/world.h>

// One frame's step of world, the shapes' GPU side read by the shape system.
void voe_editor_world_step(voe_ecs_world *world, const voe_3d_shapes *shapes);
