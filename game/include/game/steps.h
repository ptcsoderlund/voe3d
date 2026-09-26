// The game's fixed steps (0254): each frame's elapsed time banked, whole steps
// of VOE_GAME_STEP_SECONDS run out of it, and the lag the draw sits behind.
//
//     voe_game_steps steps = { 0 };
//     float lag = voe_game_steps_run(&steps, world, window, &shapes,
//                                    frame.tick.step,
//                                    voe_game_project_systems_run,
//                                    voe_game_project_systems_after_move);
//     voe_game_frame(app, world, &shapes, scratch, size, lag);
//
// ONE STEP, IN ORDER: the transforms remembered, `systems` with `seconds` the
// step, the world step (game/frame.h), the bodies' move, the transform
// system again so later queries see where the bodies went, `after_move` with
// the same step, then the world step again. The second world step drains what
// `after_move` submitted in this step, before the next remember, so a follow
// is drawn at the bodies' lag and not a step behind (0257).
//
// WHY FIXED: a body's move and a project's gravity integrate by the step, so a
// jump is as high at 30 frames a second as at 240, and a replay is the same.
//
// WHY A MAXIMUM: past VOE_GAME_STEPS_MAX steps in one frame, what is left is
// dropped to under one step. A machine under 15 frames a second then runs in
// slow motion rather than spending each frame catching up on the last.
//
// WHY THE SYSTEMS ARE HANDED IN: the game's archive is exported by the editor
// on Windows (0245), and every name it holds must resolve there. Only run.c
// names a project's entry points; this file takes them as functions.
//
// Constraints: the world is one voe_game_world_new made, which registers the
// previous transforms. `window` is NULL headless. `elapsed` is at least 0 and
// finite. The returned lag is in (0, 1]: 1 when nothing is left banked.
#pragma once

#include <3d/shape_system.h>

#include <ecs/world.h>

#include <game/project.h>

#include <platform/window.h>

// The fixed step, in seconds, and the most steps one frame runs.
#define VOE_GAME_STEP_SECONDS (1.0 / 60.0)
#define VOE_GAME_STEPS_MAX 4

// The time banked and not yet stepped, in seconds, under one step between
// calls. Zero is a fresh bank.
typedef struct {
	double banked;
} voe_game_steps;

// Banks `elapsed`, runs the whole steps it holds up to the maximum, and
// returns the lag: 1 − banked / step.
float voe_game_steps_run(voe_game_steps *steps, voe_ecs_world *world,
			 voe_platform_window *window,
			 const voe_3d_shapes *shapes, double elapsed,
			 void (*systems)(const voe_game_project_step *),
			 void (*after_move)(const voe_game_project_step *));
