// The one call that builds the program, and the numbers it is built with: how
// much room the device and the world are made with, where the two element
// panels stand and how big they are, and where the model is put once it has
// been read in. What the struct it fills is and who reads it is startup.h's.
//
// IT IS A CALL SITE LIKE EVERYTHING ELSE IN THIS FOLDER: `app`'s, `ecs`'s,
// `scene`'s and `3d`'s public calls in the order a program makes them, with
// each exhibit's own build behind one line. What every exhibit is and fails
// like stands in the file named for it.
//
// THE ORDER IS THE ONE THING IN HERE THAT IS NOT ARBITRARY. The arena before
// the app, because the app struct lives in it; the font before the interface,
// which cannot be made without it; and the monitor last, because its picture is
// of the world everything above it just built.
#include "startup.h"

#include "cubes.h"
#include "elements.h"
#include "interface.h"
#include "model.h"
#include "monitor.h"
#include "motion.h"
#include "quad.h"
#include "readout.h"
#include "sprites.h"
#include "surface.h"
#include "text.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <app/app.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>
#include <ecs/world.h>
#include <platform/clock.h>
#include <platform/window.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>
#include <text/font.h>

#include <stdio.h>


// Scratch for the questions starting the GPU asks the driver — how many cards,
// which queue families, which surface formats. It is handed over, used and
// destroyed here, because nothing the device keeps comes out of it.
#define STARTUP_SCRATCH (64 * 1024)

// The arena the world, the decoded pictures and everything the model reader
// builds come out of. It is the arena's block size and not a limit: the arena
// chains blocks, so a push larger than this gets one of its own. A megabyte at a
// time is enough that the model below takes two or three blocks.
#define WORLD_ARENA (1024 * 1024)

// How much of everything the world may hold. Numbers rather than guesses, so
// that a model too big for them says so at the call that could not fit it.
#define MAX_ENTITIES 4096
#define MAX_COMPONENT_TYPES 8
#define MAX_INTENT_TYPES 8

// What the GPU makes room for. The cube is 24 vertices and the model is not
// much more; the rest is headroom for the next thing dropped in here — the
// monitor's screen is one geometry, one shading record and one entity out of it.
//
// MAX_DRAWN_OBJECTS IS PER FRAME AND THE FRAME NOW HAS TWO PASSES IN IT. Every
// drawable is recorded once per pass it is drawn in, and the monitor draws the
// whole world a second time, so the number this program actually spends is about
// twice what is on the screen. It is still a small fraction of this.
#define MAX_VERTICES (64 * 1024)
#define MAX_INDICES (128 * 1024)
#define MAX_MESHES 64
#define MAX_DRAWN_OBJECTS 256
#define MAX_SHADINGS 64

// How many passes one frame opens, and how many targets this program makes over
// its life. Two passes: the monitor's own target and then the window. One
// target, the monitor's — the window's is not one of these and costs nothing
// here. Each target also spends one of the device's texture slots, which is the
// one number in this list that is not asked for by name.
#define MAX_PASSES 2
#define MAX_TARGETS 1

// The longest step the scene is ever advanced by, in seconds, however long the
// frame actually took.
//
// THE CLOCK IS REAL NOW AND CARD 020 IS WHAT MADE IT ONE. Every frame is stepped
// by however long the last one actually took, and not by a nominal sixtieth of a
// second — so the orbit takes the number of seconds it says it does on a display
// of any refresh rate. Since card 052 the reading and the subtraction are
// voe_app_frame_open's: it hands back both numbers, the interval that happened
// and that interval clamped, and main.c reports the first and steps the scene
// by the second. This is the ceiling it is handed, and nothing clamps what is
// reported.
//
// IT IS A CLAMP ON THE SCENE AND NOT ON THE MEASUREMENT. The numbers the
// readout prints are what the clock said, always; this is only what the orbit, the spin
// and the sun are stepped by. Without it, a frame that took two seconds — the
// window dragged to another monitor, the machine swapping, a debugger stopped at
// a breakpoint — teleports everything a sixth of the way round its lap in one
// step, and what a person sees is a scene that jumped rather than a frame that
// was slow. A quarter of a second is longer than any frame worth watching and
// shorter than any pause worth catching up on.
#define MAX_FRAME_SECONDS 0.25

// The camera the program starts with. A sixty-degree vertical field of view, a
// near plane close enough to walk up to something and a far plane past anything
// in the scene.
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The two element panels: how big each one is in the world, and where it stands.
//
// And three surfaces of elements, which are two different kinds of thing:
//
//   - THE EXHIBIT, a panel standing in the world behind the cubes: forty
//     coloured rectangles and two lines of writing, all of it one draw command.
//     It is an object in metres, so the cubes and the figure pass in front of it
//     as the camera goes round, and it is seen from behind — writing and all —
//     for half of every lap.
//   - THE BADGE, a small violet panel in the overlay, sitting in the turning
//     cube. Nothing ever covers it, which is what the overlay layer means; it
//     still keeps a real position in metres and is still seen in perspective.
//   - THE PLATE AND THE ROW OF TICKS in the top-left corner, which is not a
//     panel at all: it is mapped straight onto the window and has no position.
//     Drag the window narrow and it is the only thing that changes — the same
//     rectangles at the same size, with the far ticks off the right edge. See
//     src/surface.h for why that is the decision working.
//
// A MILLIMETRE IS A MILLIMETRE AND THE SCALE IS WHAT MAKES THEM BIG ENOUGH TO
// LOOK AT. The exhibit is authored 240 by 135 mm, which is 0.24 by 0.135 metres
// — a postcard, and unreadable from seven metres out. The scale below is the
// entity's own transform doing what a transform does; it is not a second
// millimetre convention, and dividing the authored numbers by it would give the
// same picture with the layout's units made meaningless. See
// 3d/panel_component.h.
//
// THE EXHIBIT STANDS BEHIND THE CUBES SO THAT THEY PASS IN FRONT OF IT. That is
// the picture the whole card is for: a panel in the world layer is occluded by
// what is between it and the camera, which nothing drawn after the world could
// ever be. Half a lap it is partly hidden and half a lap it is not.
#define EXHIBIT_SCALE 14.0f
#define EXHIBIT_X 0.0f
#define EXHIBIT_Y 1.2f
#define EXHIBIT_Z (-2.2f)

// The badge sits in the turning cube, exactly where the three overlay quads do
// and for the same reason: something is in front of it for most of the lap and
// it is never hidden, which is what the overlay layer means. It is small
// because its job is to be a second range of the one element buffer rather than
// a second exhibit.
#define BADGE_SCALE 16.0f
#define BADGE_X 0.8f
#define BADGE_Y 0.35f
#define BADGE_Z 0.0f

// Where the model is put, once, after it is imported. A file places its
// contents wherever its author left them — a model should not have an opinion
// about what else is in the scene — so this is the call site moving it out of
// the cubes' way, and it moves it the way anything moves anything: by
// submitting a transform intent.
#define HUMAN_X 3.5f

bool voe_dev_start(struct voe_dev_program *program)
{
	voe_base_arena *scratch;
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry quad = { 0 };
	voe_render_present present;
	voe_render_capacities capacities = {
		.vertices = MAX_VERTICES,
		.indices = MAX_INDICES,
		.geometries = MAX_MESHES,
		.objects = MAX_DRAWN_OBJECTS,
		.shadings = MAX_SHADINGS,
		.transient_vertices = 4 * VOE_DEV_TRANSIENT_GLYPHS,
		.transient_indices = 6 * VOE_DEV_TRANSIENT_GLYPHS,
		.transient_geometries = VOE_DEV_TRANSIENT_GEOMETRIES,
		// Everything every surface in the frame submits, into the one
		// buffer: the exhibit, the badge and the screen-filling
		// surface. Four ranges of it now — the interface is the
		// fourth, and its number is the only one of the four that is a
		// ceiling rather than a count, because `ui` emits one record
		// per letter of whatever the labels happen to say.
		.elements = VOE_DEV_ELEMENTS + VOE_DEV_BADGE_ELEMENTS +
			    VOE_DEV_SURFACE_ELEMENTS +
			    VOE_DEV_INTERFACE_ELEMENTS,
		// Two passes a frame: the monitor's, onto its own target with
		// its own camera, and then the window's with the world's
		// camera. Everything this program draws is inside one of the
		// two, and the monitor's target is the one target it makes.
		.passes = MAX_PASSES,
		.targets = MAX_TARGETS,
	};
	voe_ecs_limits limits = {
		.entities = MAX_ENTITIES,
		.component_types = MAX_COMPONENT_TYPES,
		.intent_types = MAX_INTENT_TYPES,
	};
	voe_scene_camera camera = {
		.fov_y = FIELD_OF_VIEW,
		.near_plane = NEAR_PLANE,
		.far_plane = FAR_PLANE,
	};

	VOE_BASE_ASSERT(program != NULL, "nowhere to start a program");

	// Zeroed here rather than by the caller, so that every member the
	// refusals below never reach is NULL and the teardown after a false
	// return has nothing to test.
	*program = (struct voe_dev_program){ 0 };

	// The world and everything read into it live here, and it is destroyed
	// at the end: the world is the arena's, which is what rule 11 asks for.
	// It is made before the window now, because the app struct lives in it
	// too and has to outlive every call made through it.
	program->arena = voe_base_arena_new(WORLD_ARENA);

	// The window and the device, in one call and in that order. Nothing is
	// kept out of `scratch`, so it goes as soon as this returns.
	//
	// NOTHING IS PRINTED HERE ON A FAILURE AND THAT IS NOT AN OVERSIGHT.
	// `app` says which of the two refused and `render` says why, both on
	// stderr, before this returns NULL; a third line from this file would
	// only repeat them.
	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	program->app = voe_app_new(program->arena, scratch,
				   (voe_app_settings){
					   .width = 960,
					   .height = 540,
					   .title = "voe3d — a model, two cubes, one camera",
					   .capacities = capacities,
					   .longest_step = MAX_FRAME_SECONDS },
				   &error);
	voe_base_arena_destroy(scratch);
	if (program->app == NULL)
		return false;

	program->window = voe_app_window(program->app);
	program->gpu = voe_app_device(program->app);

	program->world = voe_ecs_world_new(program->arena, limits);

	// Registration, once, and this is the whole of what a call site has to
	// know about which components exist. Each folder says what one of its
	// components is; nothing here does.
	voe_scene_transform_register(program->world, MAX_ENTITIES);
	voe_scene_camera_register(program->world, 4);
	voe_scene_light_register(program->world, 4);
	voe_3d_mesh_register(program->world, MAX_ENTITIES);
	voe_3d_material_register(program->world, MAX_ENTITIES);
	// The second kind of drawable. Two of them in this program, and the
	// table is registered like any other component — which is the whole of
	// what a panel costs a call site.
	voe_3d_panel_register(program->world, MAX_ENTITIES);

	// The eye is placed like anything else, where the orbit starts, and its
	// camera is only the lens it is seen through (0222).
	if (!voe_ecs_entity_create(program->world, &program->eye) ||
	    !voe_scene_transform_add(program->world, program->eye,
				     voe_dev_flight_pose(voe_dev_orbit(0.0f))) ||
	    !voe_scene_camera_add(program->world, program->eye, camera)) {
		VOE_BASE_ERROR("dev", "could not make a camera");
		return false;
	}

	// The sun, at wherever its lap starts. The draw system needs exactly one
	// light in the world, so this is not optional wiring — a world without it
	// asserts rather than drawing something black.
	if (!voe_ecs_entity_create(program->world, &program->sun) ||
	    !voe_scene_light_add(program->world, program->sun,
				 voe_dev_sunlight(program->sun, 0.0f).light)) {
		VOE_BASE_ERROR("dev", "could not make a sun");
		return false;
	}

	if (!voe_dev_add_the_cubes(program->world, program->gpu, program->arena,
				   &program->turning, &error)) {
		VOE_BASE_ERROR("dev", "could not build the two cubes: %s",
			       voe_base_error_string(error));
		return false;
	}

	if (!voe_dev_add_the_quads(program->world, program->gpu, &quad, &error)) {
		VOE_BASE_ERROR("dev", "could not build the two see-through quads: %s",
			       voe_base_error_string(error));
		return false;
	}

	if (!voe_dev_sprites_add(program->world, program->gpu, program->arena,
				 &program->sprites, &error)) {
		VOE_BASE_ERROR("dev", "could not build the sprites: %s",
			       voe_base_error_string(error));
		return false;
	}

	// The font and the three text entities. Not optional the way a model is:
	// there is one font, it is in the binary, and a failure here is a bug in
	// the reader rather than a file somebody could not open.
	program->font = voe_dev_add_the_text(program->world, program->gpu,
					     program->arena, quad, &program->hud,
					     &program->panel, &program->readout,
					     &program->hud_size, &error);
	if (program->font == NULL) {
		VOE_BASE_ERROR("dev", "could not build the text: %s",
			       voe_base_error_string(error));
		return false;
	}

	// The interface, which needs the font and so cannot be made before it.
	// It is the context and nothing else — what is on the interface is
	// built afresh every frame inside the loop.
	program->interface = voe_dev_interface_new(program->arena, program->font);

	// The two panels. Nothing is on them yet: what a panel holds is a range
	// of the frame that is open, and no frame is open until the loop starts.
	if (!voe_dev_add_panel(
		    program->world,
		    (voe_math_float3){ EXHIBIT_X, EXHIBIT_Y, EXHIBIT_Z },
		    EXHIBIT_SCALE,
		    (voe_math_float2){ VOE_DEV_ELEMENTS_PANEL_WIDE,
				       VOE_DEV_ELEMENTS_PANEL_HIGH },
		    VOE_3D_LAYER_WORLD, &program->exhibit_panel) ||
	    !voe_dev_add_panel(
		    program->world,
		    (voe_math_float3){ BADGE_X, BADGE_Y, BADGE_Z }, BADGE_SCALE,
		    (voe_math_float2){ VOE_DEV_BADGE_WIDE, VOE_DEV_BADGE_HIGH },
		    VOE_3D_LAYER_OVERLAY, &program->badge_panel)) {
		VOE_BASE_ERROR("dev", "could not build the two element panels");
		return false;
	}

	// The model is the one thing here that is allowed to fail without
	// stopping the program: the cubes are what says the renderer works, and
	// a person looking at a window is better served by seeing them and a
	// message than by seeing nothing.
	if (!voe_dev_add_a_model(program->world, program->gpu, program->arena,
				 "human", voe_dev_human_glb,
				 voe_dev_human_glb_size, HUMAN_X, &error))
		VOE_BASE_ERROR("dev", "could not read the human model: %s",
			       voe_base_error_string(error));

	// The monitor, last, because its screen stands in the world everything
	// above just built and its picture is that world seen from somewhere
	// else. Making a target waits for the card, so it happens here and never
	// in the loop — see src/monitor.h.
	//
	// AND OFF TO THE LEFT, A SCREEN SHOWING THIS SAME WORLD FROM SOMEWHERE
	// ELSE. A square standing on nothing, turned towards the middle of the
	// scene, holding a second camera's picture: the cubes and the figure
	// from high up and in front, lit by the same sun at the same instant,
	// with the turning cube turning in it and the sun crossing it. It is a
	// target drawn into once a frame and worn as an ordinary texture by an
	// ordinary quad — see src/monitor.h. The world's own camera orbits and
	// the second one does not, so what is on the screen holds still while
	// everything around it swings.
	//
	// IT IS ONE-SIDED, SO HALF OF EVERY LAP IT IS NOT THERE. A screen has a
	// back, and the back of this one is culled; the alternative would show
	// the picture mirrored, which is the one thing this program's whole
	// cast of lettered objects exists to make a person suspicious of.
	//
	// AND IT IS MISSING FROM ITS OWN PICTURE, WHICH IS THE POINT. The pass
	// that fills the target leaves the screen out (ADR-0158) — so there is
	// no screen inside the screen, and no hall of mirrors, and no image
	// being read while it is written.
	if (!voe_dev_monitor_create(&program->monitor, program->world,
				    program->gpu, &error)) {
		VOE_BASE_ERROR("dev", "could not build the monitor: %s",
			       voe_base_error_string(error));
		return false;
	}

	program->size = voe_platform_window_size(program->window);
	program->decorated = voe_platform_window_decorated(program->window);
	printf("opened     %dx%d\n", program->size.width, program->size.height);
	printf("decorated  %s\n", program->decorated ? "yes" : "no");
	printf("camera     orbit — Tab to fly, Escape to hand back or close\n");
	// Fifo, because nothing here asks for anything else: the device opens on
	// it and dev opens in step with the display like the editor (0215). P is
	// what asks for mailbox from here — see main.c. Every block from then on
	// says what is in force.
	present = voe_render_present_get(program->gpu);
	printf("present    %s\n",
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	voe_dev_say_what_is_measured();
	fflush(stdout);

	// When the first reporting period started. The frame's own interval is
	// the clock inside `app` and main.c no longer keeps a previous reading
	// of its own.
	program->timing.started = voe_platform_clock_now();

	VOE_BASE_ASSERT(program->world != NULL, "a program with no world");
	return true;
}
