// Everything voe_dev does before its first frame, and the struct the loop then
// reads it out of: the arenas, the window and the device, the world and its
// component tables, every exhibit, the model, the monitor, and the lines printed
// before the loop starts.
//
// THE STRUCT IS THE PROGRAM'S STATE, WRITTEN AT STARTUP AND BY THE LOOP IN
// main.c, AND READ NOWHERE ELSE. It is one member per local main() used to
// declare and holds no more than that: nothing in this folder takes it as a
// parameter and no system knows it exists.
//
// ITS OWN FILE BECAUSE STARTUP IS NOT THE FRAME. Everything here happens once,
// in the order the calls stand in, and none of it runs again; the loop and the
// order of what is in it are main.c's (ADR-0135), which is why this file ends
// where the `while` begins.
//
// WHAT main() STILL DOES IS DESTROY WHAT THIS MADE. A refusal leaves every
// member the call did not reach zero, so the teardown after a false return is
// the same three destroys a close takes.
#pragma once

#include "monitor.h"
#include "readout.h"
#include "sprites.h"

#include <app/app.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <platform/window.h>
#include <render/device.h>
#include <text/font.h>
#include <ui/layout.h>

#include <stdbool.h>

// What the GPU makes room for per frame: geometry that lives one frame, which
// today is the readout and nothing else. Sized in glyphs because that is what
// fills it — four vertices and six indices each — and the readout is about
// seventy of them, so this is under double with nothing "to be safe" in it. The
// program prints what the readout actually took beside this number, once, so the
// next thing that needs transient room has a measurement to start from. Two
// ranges: the readout is one, and the other is for the next thing.
//
// THEY ARE HERE AND NOT IN startup.c BECAUSE BOTH ENDS NAME THEM: the capacity
// below is asked for once, and the loop prints what it spent against it.
#define VOE_DEV_TRANSIENT_GLYPHS 128
#define VOE_DEV_TRANSIENT_GEOMETRIES 2

struct voe_dev_program {
	// The window and the device, opened together by `app` and reached
	// through it. Both are taken into members once, at startup, because the
	// loop names them on nearly every line and voe_app_window(app) on each
	// of them would say nothing the name does not.
	voe_app *app;
	voe_platform_window *window;
	voe_render_device *gpu;
	// The world and everything read into it live in this arena, and it is
	// destroyed at the end: the world is the arena's, which is what rule 11
	// asks for. It is made before the window, because the app struct lives
	// in it too and has to outlive every call made through it.
	voe_base_arena *arena;
	voe_ecs_world *world;
	// The window as it opened. The loop asks the window again every frame
	// and prints a line when either of these has changed.
	voe_platform_size size;
	bool decorated;
	// The entities the loop names: the eye it orbits or flies, the sun, the
	// cube it spins, and the three it places in front of the camera.
	voe_ecs_entity eye;
	voe_ecs_entity sun;
	voe_ecs_entity turning;
	voe_ecs_entity hud;
	voe_ecs_entity panel;
	voe_ecs_entity readout;
	// The two element surfaces that are entities: the exhibit standing in
	// the world and the badge in the overlay. Their ranges are rewritten
	// every frame — see the loop.
	voe_ecs_entity exhibit_panel;
	voe_ecs_entity badge_panel;
	// The sprite exhibit, which the loop turns towards the camera, and the
	// second camera's picture and the screen in the world that shows it.
	// The monitor's target, camera and screen entity are all made once; the
	// loop asks it only what its pass is drawn with.
	voe_dev_sprites sprites;
	voe_dev_monitor monitor;
	// The font every letter in the program is laid out with, and the
	// context the interface is built in. The font holds GPU resources and
	// is destroyed before the device closes; the context is the arena's.
	voe_text_font *font;
	voe_ui_context *interface;
	// How big the heads-up line came out, which is what the transforms
	// placing it and the panel behind it are worked out from.
	voe_math_float2 hud_size;
	// The present mode this program is asking for. It starts true because
	// the request at startup is made with it, so the first press of P asks
	// for fifo rather than for what is already happening. What is in force
	// is the device's answer and is asked for rather than remembered.
	bool mailbox_wanted;
	// What the loop measures, and when the first reporting period started.
	// Every sample in it is the loop's; only that one reading is taken here.
	struct voe_dev_timing timing;
};

// Builds all of it into a struct the caller owns and zeroes first. False when
// anything refused, with the reason already printed: `app` and `render` say
// which of the window and the device refused and why, and every later refusal
// prints the line naming what could not be built. The model is the one thing
// allowed to fail without stopping the program.
//
// Whatever it filled in before a refusal is left in `program` and everything it
// did not reach is zero, so the caller can destroy either way.
bool voe_dev_start(struct voe_dev_program *program);
