// voe_editor — the program a person opens to author a scene. Today it opens a
// window, draws two named regions side by side, `Scene` and `Inspector`, whose
// rectangles came out of a tree of data rather than out of the order of the
// calls in this file, lists the authored entities of a scene built in code and
// follows a click on one. Nothing is draggable and there is no viewport.
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING, the same standing dev/ has.
// What is here is the window's size, the capacities, the loop and the one
// division that turns the mouse's pixels into the surface's millimetres.
// Anything in it that starts to look worth keeping belongs in a folder, with a
// test.
//
// THE LOOP IS THIS FILE'S AND THE PARTS IN IT ARE `app`'S (ADR-0135).
// voe_app_frame_open opens the frame, voe_app_draw_open and voe_app_draw_close
// bracket the draw, and what happens between them is this program deciding.
//
// THE POINTER'S DIVISION LIVES HERE AND NOWHERE ELSE (ADR-0141 point 4). The
// interface is handed a pointer already in panel millimetres and computes none
// of its own, because the day a panel is a quad standing in the world that
// conversion is a ray against the quad — a different sum, in a different file,
// and only a call site can know which of the two it wants.
//
// AND THE EDITOR DRAWS NO WORLD, which is why its one pass onto the window is
// opened with no camera. An element draw needs none, and a pass without one is
// how `render` is told there is no eye to invent.
#include "dock.h"
#include "interface.h"
#include "scene.h"

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/world.h>

#include <platform/input.h>
#include <platform/window.h>

#include <render/device.h>

#include <scene/identity_system.h>
#include <scene/transform_system.h>

#include <text/font.h>

#include <ui/widgets.h>

#include <stdio.h>

// The block size of the one arena everything lives in — the app struct, the
// world, the font, the interface context and every frame's tree. It is a block
// size and not a limit; the arena asks the operating system for another when it
// runs out.
#define EDITOR_ARENA (4u * 1024u * 1024u)

// Startup's working memory, handed to `app` and kept by nothing.
#define STARTUP_SCRATCH (1u * 1024u * 1024u)

// The ceiling on a frame's step, in seconds. Nothing here integrates over time
// yet, so it is `app`'s required policy and no more.
#define MAX_FRAME_SECONDS 0.25

#define EDITOR_WIDE 1280
#define EDITOR_HIGH 720

// What the world may hold. Two component types are registered below, each with
// its own intent queue, and the entities are a number to author into rather
// than a measurement of anything.
#define MAX_ENTITIES 1024
#define MAX_COMPONENT_TYPES 8
#define MAX_INTENT_TYPES 8

// How many transforms and identities the world has room for. The identities are
// VOE_EDITOR_SCENE_ROWS because that is how many the Scene panel can list, and
// a world that could hold an identity the list could not show would be a
// disagreement between two numbers in one program (scene.h). Transforms are
// wider: an entity the engine makes for itself has one and no identity.
#define MAX_TRANSFORMS 256
#define MAX_IDENTITIES VOE_EDITOR_SCENE_ROWS

// THE SURFACES THIS PROGRAM HAS ARE ALL INTERFACE, which is what makes every
// other capacity a one. The editor submits no vertices, no indices and no
// objects — it draws elements and nothing else — and `render` requires those
// five to be greater than nought whether anything uses them or not, so they are
// the smallest number that is. `passes` is one because there is one pass, the
// interface's. `elements` and the three transient numbers are
// the two groups that may be nought; see render/include/render/device.h.
#define EDITOR_CAPACITIES                                                      \
	(voe_render_capacities)                                                \
	{                                                                      \
		.vertices = 1, .indices = 1, .geometries = 1, .objects = 1,     \
		.shadings = 1, .elements = VOE_EDITOR_INTERFACE_ELEMENTS,      \
		.passes = 1                                                    \
	}

// One line at startup saying whether a field description reached the binary,
// because with them off every later card's inspector has nothing to expand and
// the program would simply look empty.
static void say_whether_descriptions_are_in(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	printf("descriptions  compiled in\n");
#else
	printf("descriptions  off — nothing will be expandable; build with `cmake --preset editor` to turn them on\n");
#endif
}

int main(void)
{
	voe_base_arena *arena;
	voe_base_arena *scratch;
	voe_app *app;
	voe_base_error error;
	voe_platform_window *window;
	voe_render_device *gpu;
	voe_ecs_world *world;
	voe_text_font *font;
	voe_ui_context *ui;
	// The roots the loop walks. One of them, and it is the window; see
	// dock.h on what a second one would cost.
	voe_editor_dock_root roots[1] = { 0 };
	// BESIDE THE ROOTS AND NOT IN ONE. What is selected is the editor's and
	// the dock tree does not know it exists (scene.h): where a panel sits
	// and what has been clicked in it are two unrelated facts.
	voe_editor_scene scene = { 0 };
	int status = 0;

	arena = voe_base_arena_new(EDITOR_ARENA);

	// The window and the device, in one call and in that order. Nothing is
	// kept out of `scratch`, so it goes as soon as this returns, and nothing
	// is printed on a failure: `app` says which of the two refused and
	// `render` says why, both on stderr, before it returns NULL.
	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	app = voe_app_new(arena, scratch,
			  (voe_app_settings){ .width = EDITOR_WIDE,
					      .height = EDITOR_HIGH,
					      .title = "voe3d editor",
					      .capacities = EDITOR_CAPACITIES,
					      .longest_step = MAX_FRAME_SECONDS },
			  &error);
	voe_base_arena_destroy(scratch);
	if (app == NULL) {
		voe_base_arena_destroy(arena);
		return 1;
	}

	window = voe_app_window(app);
	gpu = voe_app_device(app);

	// The world the editor authors into. Made here because it is the arena's
	// and has to outlive every frame, and registered into immediately: a
	// component's table, its description and its intent queue all come from
	// the one _register call, and nothing may add a component before it.
	world = voe_ecs_world_new(arena, (voe_ecs_limits){
						.entities = MAX_ENTITIES,
						.component_types =
							MAX_COMPONENT_TYPES,
						.intent_types = MAX_INTENT_TYPES });
	voe_scene_transform_register(world, MAX_TRANSFORMS);
	voe_scene_identity_register(world, MAX_IDENTITIES);
	voe_editor_scene_build(&scene, world);

	font = voe_text_font_new(gpu, arena, &error);
	if (font == NULL) {
		VOE_BASE_ERROR("editor", "the editor could not build its font: %s",
			       voe_base_error_string(error));
		status = 1;
		goto stop;
	}

	ui = voe_editor_interface_new(arena, font);
	roots[0].tree = voe_editor_dock_default();

	say_whether_descriptions_are_in();
	fflush(stdout);

	while (true) {
		voe_app_frame opened;
		voe_platform_pointer pointer;
		float pixels_per_millimetre;
		bool drawing = false;
		bool drawn;

		// EVERY OWNING SYSTEM RUNS EVERY FRAME, WHETHER ANYTHING
		// SUBMITTED OR NOT (ADR-0134 point 7). An intent that reaches a
		// queue on a frame its system does not drain is an edit that
		// lands whenever the loop next happens to run it, which is a
		// class of bug that does not exist if the run is
		// unconditional. Nothing here submits one yet — card 059's
		// inspector is what does — and the two calls are still here,
		// because the frame the first submit arrives on must not also
		// be the frame somebody remembers to add these.
		voe_scene_transform_system_run(world);
		voe_scene_identity_system_run(world);

		// The clock, the poll, and what the window says afterwards, in
		// that order and once.
		opened = voe_app_frame_open(app);
		if (opened.closing)
			break;
		// A window with no area has no surface to divide, and dividing
		// by its height is what every number below starts with.
		if (opened.minimised)
			continue;

		voe_editor_interface_surface(opened.size, &roots[0].size,
					     &pixels_per_millimetre);

		// THE ONE DIVISION. Both spaces run x right and y down from a
		// top-left corner, so no axis turns over on this line. `over`
		// is passed through as `platform` reports it, and `fine` is
		// this program's choice of which key means "slower" — `ui` is
		// handed values and never asks a window anything.
		pointer = voe_platform_input_pointer(window);
		roots[0].pointer = (voe_ui_pointer){
			.at = { pointer.x / pixels_per_millimetre,
				pointer.y / pixels_per_millimetre },
			.over = pointer.over,
			.down = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_LEFT),
			.fine = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_SHIFT)
		};

		if (!voe_app_draw_open(app, opened.size, &drawing)) {
			status = 1;
			break;
		}
		if (!drawing)
			continue;

		// One pass onto the window, with no camera: the interface is
		// all this program draws. The first pass of a frame on a device
		// with room for one is not refused; if it were, the frame would
		// still be closed below, with nothing drawn.
		drawn = voe_render_pass_begin(gpu, VOE_RENDER_TARGET_WINDOW, NULL);
		if (drawn) {
			drawn = voe_editor_interface_draw(
				gpu, ui, arena, roots,
				(uint32_t)(sizeof roots / sizeof roots[0]),
				&scene);
			voe_render_pass_end(gpu);
		}

		// The frame is closed either way: a refused interface is this
		// program's numbers being wrong, and abandoning a half-recorded
		// frame would leave the slot's fence waiting on it.
		if (!voe_app_draw_close(app)) {
			status = 1;
			break;
		}
		if (!drawn) {
			status = 1;
			break;
		}
	}

stop:
	// The device and the window, then the arena — the app struct lives in
	// the arena and has to outlive every call made through it.
	voe_app_destroy(app);
	voe_base_arena_destroy(arena);
	return status;
}
