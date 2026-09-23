// The monitor: a second camera's picture of this same world, drawn into a target
// of its own and shown on a screen standing in the world beside the cubes. It is
// the thing a person runs voe_dev to look at for "a program draws a view into a
// texture and puts that texture on something in the scene".
//
// IT IS A CALL SITE LIKE EVERYTHING ELSE IN THIS FOLDER. A target, a camera, a
// quad, a material wearing the target's texture, and the frame the pass onto that
// target is drawn with. Nothing here is engine work: it is `render`'s and `3d`'s
// public calls in the order a program would make them, the same way src/quad.c
// and src/surface.c are.
//
// THE CAMERA IS NOT AN ENTITY, AND IT MAY NOT BECOME ONE. A world drawn by
// voe_3d_draw_system_frame has exactly one camera entity and asserts on two
// (3d/draw_system.h) — a second camera means a second target and a second frame,
// which is exactly what this is. So the camera is a pose and a lens held in
// here, voe_3d_view builds the view from them, and the world's own
// camera table is left holding the one eye the orbit flies. That is also what
// editor/src/view.c does and for the same reason.
//
// THE SUN IS THE WORLD'S, READ OUT OF THE LIGHT TABLE, AND NOT A SECOND ONE. The
// picture on the screen is meant to be the same world lit the same way at the
// same instant; a sun of its own would light one scene two ways, which reads as a
// bug in the lighting from across the room and is not one.
//
// THE FRAME HIDES THE SCREEN AND THAT IS NOT OPTIONAL (ADR-0158). The screen's
// material names the target's texture, so the pass that fills the target would
// otherwise draw the surface that shows it — an image read while it is written,
// which Vulkan leaves undefined. voe_render_target_create's debug check catches
// it: a debug build asserts with a line about a draw whose shading record names
// the open pass's target texture, and a release build draws whatever the card
// happens to have. So voe_dev_monitor_frame sets `hidden` to the screen entity,
// every frame, and the window's frame leaves it zeroed — which is why the screen
// is missing from its own picture and present in the window's.
//
// THE SCREEN IS ONE-SIDED, UNLIKE src/quad.c's SQUARES. It is four vertices and
// not eight: a screen has a back, and half of the camera's lap is spent behind
// it with nothing to see, which is back-face culling working. The alternative —
// the double-sided pair the see-through quads use — would show the picture
// mirrored for half of every lap, and mirrored content is the one failure
// src/main.c's header tells a reader to hunt a coordinate bug over.
//
// AND ITS MATERIAL IS UNLIT, WHICH IS THE OTHER HALF OF "WHAT SHOWS IS THE
// PICTURE". A lit screen would be the picture times a lambert term, so it would
// darken as the sun went round and go black on the far side of the lap — a
// picture that is only sometimes readable, and indistinguishable at a glance from
// a target nothing drew into.
//
// WHAT IT SHOWS INCLUDES THE HEADS-UP THINGS, AND THAT IS THE ENGINE BEING
// HONEST. The readout and the line "locked to the camera" are ordinary entities a
// metre in front of the world's eye, because there is no screen space in this
// engine (src/main.c) — so the second camera sees them floating in the world,
// small and off to one side, exactly as it sees everything else. Nothing filters
// them out and nothing should: `hidden` is one entity and the case it exists for
// is this screen.
#pragma once

#include <3d/draw_system.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/transform_component.h>

// How big the picture is, in pixels. Square, so the screen quad is square and
// the aspect ratio is one — a picture whose shape did not match its surface's
// would stretch, and there is nothing here that would say which of the two was
// wrong. Big enough to read the cubes' writing on, small enough that a second
// pass over the whole world costs nothing worth measuring.
#define VOE_DEV_MONITOR_PIXELS 512u

// The monitor. The ids are made once at startup and never change — a target is
// kept and not asked for per frame (render/device.h) — and the camera is a value
// this file owns; see the header for why it is not an entity.
typedef struct {
	voe_render_target target;
	// The texture that shows the target's picture. Kept because the material
	// wearing it is built from it and because a reader looking for where the
	// two ends of this meet should find both in one struct.
	voe_render_texture texture;
	// Where the camera stands and what it sees through, built once from an
	// eye and two angles with voe_dev_flight_pose.
	voe_scene_transform pose;
	voe_scene_camera lens;
	// The quad standing in the world with that texture on it. It is what the
	// monitor's own pass hides — see the header.
	voe_ecs_entity screen;
} voe_dev_monitor;

// The target, the camera, the screen's geometry, its material and its entity.
// A startup operation, because making a target and uploading geometry both are:
// call it after the world is built and outside any frame.
//
// False when `render` refused the target, the geometry or the shading record, or
// when the world had no room for another entity — each of which has already said
// what happened on stderr.
[[nodiscard]] bool voe_dev_monitor_create(voe_dev_monitor *out,
					  voe_ecs_world *world,
					  voe_render_device *gpu,
					  voe_base_error *error);

// What the monitor's pass is drawn with: its camera at its target's aspect
// ratio, the world's sun, and its own screen hidden. Called every frame, before
// the pass onto its target is opened; the caller hands the view and the light
// straight to voe_render_pass_begin and the whole of this to
// voe_3d_draw_system_run, exactly as it does with voe_3d_draw_system_frame's
// answer for the window.
//
// The world needs the one light voe_3d_draw_system_frame already requires of it,
// and having none asserts here for the same reason it does there.
voe_3d_frame voe_dev_monitor_frame(const voe_dev_monitor *monitor,
				   const voe_ecs_world *world);
