// voe_editor — the program a person opens to author a scene. Today it opens a
// window on three columns — `Scene`, two scene views stacked, `Inspector` —
// whose rectangles came out of a tree of data rather than out of the order of the
// calls in this file, lists the authored entities of a scene built in code,
// follows a click on one, and draws the scene's cubes into each view from that
// view's own camera. A middle-button drag in a view moves that view's camera.
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
// and only a call site can know which of the two it wants. The views' drag is
// handed the same millimetres, for the same reason. THE WHEEL IS THE SAME SHAPE
// OF DECISION: `platform` counts notches, `ui` takes a length in millimetres,
// and WHEEL_MILLIMETRES between them is this program saying how far a notch
// moves anything.
//
// A FRAME IS A PASS PER VIEW AND THEN ONE ONTO THE WINDOW (ADR-0148). Each view
// the tree shows is drawn into its own target with its own camera first; the
// window's pass comes last, is opened with no camera — an element draw needs
// none, and a pass without one is how `render` is told there is no eye to invent
// — and shows each view's picture as an image on its panel. The world is drawn
// through voe_3d_draw_system_run, but with the view's camera and the editor's
// sun (view.h) rather than with voe_3d_draw_system_frame, which reads a camera
// and a light out of the world and this world has neither.
//
// IT CAN ALSO BE STARTED TO WRITE ONE PICTURE AND LEAVE. `--capture <path>`
// opens the device with no window at all (voe_app_new_headless), builds the
// same world, font, interface and scene, runs the same loop body, writes the
// frame to `path` through voe_app_capture_png and returns. `--size <W>x<H>`
// says how big that picture is and defaults to the size the window would have
// opened at; it means nothing without `--capture`, so it is refused there
// rather than quietly ignored. `--size` without `--capture`, a missing value
// and any unknown argument print one usage line on stderr and return 2. With
// no arguments the editor is exactly what it has always been. THE EDITOR
// NAMES NEITHER `assets` NOR `platform`'s FILES FOR THIS: the three folders a
// capture crosses are `app`'s to tie together (ADR-0157).
//
// THE CAPTURE RUNS THE LOOP BODY TWICE, AND ONE FRAME WOULD BE THE WRONG
// PICTURE. A view's target is sized from the rectangle the dock walk recorded
// last frame, so the first frame draws every view at the size it was created
// with and stretches it (view.h). The second frame is the first one drawn at
// the layout's own sizes, and it is the frame that gets written.
//
// EVERY READ OF THE WINDOW IS GUARDED, because there is none to read on a
// capture: the pointer, the wheel, the buttons and the keys are zeroed input
// then, and nothing else in the loop changes shape — the same passes in the
// same order with the same draws, so the captured frame is the frame a person
// sees.
//
// main's SIGNATURE IS C'S OWN, `char *argv[]`, WHICH IS THE ONE DEVIATION FROM
// RULE 6 IN THIS FOLDER (DEVIATION: rule 6, an array of pointers is spelled as
// the array it is, because the C runtime calls main with this signature and
// nothing in this program chose it). Nothing else here holds a pointer to a
// pointer, and the argument list is read in main and nowhere else.
#include "cube.h"
#include "dock.h"
#include "interface.h"
#include "scene.h"
#include "view.h"

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>

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

#include <limits.h>
#include <stdio.h>
#include <string.h>

// The block size of the one arena everything lives in — the app struct, the
// world, the font, the interface context and every frame's tree. It is a block
// size and not a limit; the arena asks the operating system for another when it
// runs out.
#define EDITOR_ARENA (4u * 1024u * 1024u)

// Startup's working memory, handed to `app` and kept by nothing.
#define STARTUP_SCRATCH (1u * 1024u * 1024u)

// The capture's working memory: the pixels off the card, the encoder's tables
// and the file's bytes all come out of it and none of them outlives the write
// (app.h). A block size, not a limit — a picture larger than it gets a block of
// its own.
#define CAPTURE_SCRATCH (4u * 1024u * 1024u)

// How many frames a capture runs before it writes. Two, because a view's target
// size lags the layout by a frame; see the top of this file.
#define CAPTURE_FRAMES 2

// The ceiling on a frame's step, in seconds. Nothing here integrates over time
// yet, so it is `app`'s required policy and no more.
#define MAX_FRAME_SECONDS 0.25

#define EDITOR_WIDE 1280
#define EDITOR_HIGH 720

// How far one notch of the wheel scrolls, in the surface's millimetres. WHAT A
// NOTCH IS WORTH IS THIS PROGRAM'S TO CHOOSE (ADR-0153 point 10): `platform`
// counts detents and `ui` is handed a length, so the one multiplication between
// them is here, beside the division that turns the mouse's pixels into the same
// millimetres.
#define WHEEL_MILLIMETRES 10.0f

// What the world may hold. Five component types are registered below, two of
// them with an intent queue, and the entities are a number to author into rather
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

// How many entities may be drawn: a mesh and a material each. The scene has two
// and this is the room, not the count. The panel table is walked by the draw
// system whether anything has one or not (3d/draw_system.h), so it is registered
// with room for one and nothing ever adds a row.
#define MAX_DRAWN 64
#define MAX_PANELS 1

// WHAT THE EDITOR UPLOADS IS ONE CUBE AND ONE MATERIAL, which is what makes the
// geometry numbers the cube's own and `shadings` a one. `objects` is per frame:
// every drawn entity is one object in every view's pass, so it is the room for
// drawn entities times the room for views. `passes` is a pass per view and the
// interface's, and `targets` a target per view — both from the room for views,
// not the two in use, so a third view is a leaf and not a capacity. The three
// transient numbers stay nought; see render/include/render/device.h.
#define EDITOR_CAPACITIES                                                      \
	(voe_render_capacities)                                                \
	{                                                                      \
		.vertices = VOE_EDITOR_CUBE_VERTEX_COUNT,                      \
		.indices = VOE_EDITOR_CUBE_INDEX_COUNT, .geometries = 1,       \
		.objects = MAX_DRAWN * VOE_EDITOR_VIEWS, .shadings = 1,        \
		.elements = VOE_EDITOR_INTERFACE_ELEMENTS,                     \
		.passes = VOE_EDITOR_VIEWS + 1, .targets = VOE_EDITOR_VIEWS    \
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

// Reads one run of ASCII '0'-'9' from `text`, writes the value through `out`
// and returns the pointer to the first character that is not a digit. NULL
// when there is no digit at all, and NULL when the value would pass INT_MAX —
// checked before the multiply, not after, so nothing overflows. No sign, no
// leading blank, no `errno`, no locale: --size's promised form is `<W>x<H>`
// and nothing relies on either.
static const char *number(const char *text, int *out)
{
	int value = 0;
	bool any = false;

	while (*text >= '0' && *text <= '9') {
		int digit = *text - '0';

		if (value > (INT_MAX - digit) / 10)
			return NULL;
		value = value * 10 + digit;
		any = true;
		text++;
	}
	if (!any)
		return NULL;
	*out = value;
	return text;
}

// One line, on stderr, and the return code that goes with it. Every way of
// getting the command line wrong ends here: there is one form to state and
// stating it twice in different words would be two forms to keep in step.
static int usage(void)
{
	fprintf(stderr,
		"usage: voe_editor [--capture <path> [--size <W>x<H>]]\n");
	return 2;
}

int main(int argc, char *argv[])
{
	// Where one picture is written, and NULL when the editor opens a window
	// instead. The size is the picture's, and it is the window's size until
	// --size says otherwise.
	const char *capture = NULL;
	int wide = EDITOR_WIDE;
	int high = EDITOR_HIGH;
	bool sized = false;
	// How many frames a capture has drawn so far.
	unsigned frames = 0;
	voe_app_settings settings;
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
	// Beside the scene and not in it: a view's camera is the editor's and
	// never the world's (view.h).
	voe_editor_views views = { 0 };
	int status = 0;

	// THE COMMAND LINE IS READ BEFORE ANYTHING IS OPENED, so a mistyped
	// argument costs nothing and says so straight away. `--size`'s value is
	// parsed here rather than by the C library (ADR-0159: Windows' runtime
	// deprecates `sscanf` under -Werror and rule 8 forbids silencing that
	// tree-wide), by `number`, above, twice, with an `x` between and nothing
	// after — refusing a trailing character is what `%n` and a length check
	// used to do, so "1280x720nonsense" is still a mistake and not a
	// 1280x720. `argv[a + 1]` is parsed in full before `a` moves, which the
	// old code did not need to do inside its `&&` chain. One clause on the
	// parser itself: it is stricter than `%d` about a leading blank or a
	// leading '+' — neither was ever part of the promised form `<W>x<H>`
	// and nothing relied on them.
	for (int a = 1; a < argc; a++) {
		if (strcmp(argv[a], "--capture") == 0 && a + 1 < argc) {
			capture = argv[++a];
		} else if (strcmp(argv[a], "--size") == 0 && a + 1 < argc) {
			int w, h;
			const char *rest = number(argv[a + 1], &w);

			if (rest != NULL && *rest == 'x')
				rest = number(rest + 1, &h);
			else
				rest = NULL;
			if (rest == NULL || *rest != '\0' || w <= 0 || h <= 0)
				return usage();
			wide = w;
			high = h;
			sized = true;
			a++;
		} else {
			return usage();
		}
	}
	// A size with nothing to size: the window's is the window system's
	// answer and not this program's to name (app.h), so the only picture
	// --size could mean is one nobody asked for.
	if (sized && capture == NULL)
		return usage();

	arena = voe_base_arena_new(EDITOR_ARENA);

	settings = (voe_app_settings){ .width = wide,
				       .height = high,
				       .title = "voe3d editor",
				       .capacities = EDITOR_CAPACITIES,
				       .longest_step = MAX_FRAME_SECONDS };

	// The device, with the window before it or with no window at all, in one
	// call. Nothing is kept out of `scratch`, so it goes as soon as this
	// returns, and nothing is printed on a failure: `app` says which piece
	// refused and `render` says why, both on stderr, before it returns NULL.
	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	app = capture != NULL ?
		      voe_app_new_headless(arena, scratch, settings, &error) :
		      voe_app_new(arena, scratch, settings, &error);
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
	voe_3d_mesh_register(world, MAX_DRAWN);
	voe_3d_material_register(world, MAX_DRAWN);
	voe_3d_panel_register(world, MAX_PANELS);

	// Both upload, so both are startup operations and both come before the
	// first frame. `render` says why on stderr when it refuses.
	if (!voe_editor_scene_build(&scene, world, gpu, &error) ||
	    !voe_editor_views_create(&views, gpu, &error)) {
		status = 1;
		goto stop;
	}

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
		// Zeroed input is what a capture reads: there is no window to
		// ask, so nothing is over anything, no button is down and the
		// wheel has not turned.
		voe_platform_pointer pointer = { 0 };
		voe_platform_wheel wheel = { 0 };
		bool left = false;
		bool middle = false;
		bool shift = false;
		bool control = false;
		float pixels_per_millimetre;
		bool drawing = false;
		bool drawn = true;

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
		// EVERY READ OF THE WINDOW IS IN HERE, and there is no window on
		// a capture. Notches since the last poll are turned into a
		// length once. Nothing else reads the wheel: a scene view takes
		// the middle button and no wheel at all, so a wheel over one is
		// a scroll no area under the pointer can take and is dropped.
		if (window != NULL) {
			pointer = voe_platform_input_pointer(window);
			wheel = voe_platform_input_wheel(window);
			left = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_LEFT);
			middle = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_MIDDLE);
			shift = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_SHIFT);
			control = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_CONTROL);
		}

		roots[0].pointer = (voe_ui_pointer){
			.at = { pointer.x / pixels_per_millimetre,
				pointer.y / pixels_per_millimetre },
			.over = pointer.over,
			.down = left,
			.fine = shift,
			.scroll = { wheel.x * WHEEL_MILLIMETRES,
				    wheel.y * WHEEL_MILLIMETRES }
		};

		// The middle button is the views' and the left is the
		// interface's, so the two never compete for one press.
		voe_editor_views_drag(&views, roots[0].pointer.at, middle,
				      shift, control);

		// Before the draw is opened, so a resize asked for here is
		// applied by this frame's begin and the picture is drawn at the
		// size it is shown at — last frame's rectangle, see view.h.
		for (uint32_t v = 0; v < views.count; v++)
			if (voe_editor_dock_shows_view(&roots[0].tree, v))
				voe_editor_view_fit(&views.views[v], gpu,
						    pixels_per_millimetre);

		if (!voe_app_draw_open(app, opened.size, &drawing)) {
			status = 1;
			break;
		}
		if (!drawing)
			continue;

		// A pass per view the tree shows, each onto its own target with
		// its own camera. A device made with a pass per view and one
		// more does not refuse these; if it did, the frame is still
		// closed below and the program stops.
		for (uint32_t v = 0; v < views.count && drawn; v++) {
			const voe_editor_view *view = &views.views[v];
			voe_render_pass_camera camera;

			if (!voe_editor_dock_shows_view(&roots[0].tree, v))
				continue;

			camera = voe_editor_view_pass_camera(view);
			drawn = voe_render_pass_begin(gpu, view->target,
						      &camera);
			if (!drawn)
				break;
			voe_3d_draw_system_run(
				world, gpu, arena,
				(voe_3d_frame){ .view = camera.view,
						.light = camera.light });
			voe_render_pass_end(gpu);
		}

		// Then one pass onto the window, with no camera, for the
		// interface and the pictures on it.
		if (drawn)
			drawn = voe_render_pass_begin(
				gpu, VOE_RENDER_TARGET_WINDOW, NULL);
		if (drawn) {
			drawn = voe_editor_interface_draw(
				gpu, ui, arena, roots,
				(uint32_t)(sizeof roots / sizeof roots[0]),
				&scene, &views);
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

		// A CAPTURE'S LOOP IS NOT A PERSON'S AND HAS TO STOP ITSELF:
		// there is no window to close it (app.h). Counted here, where a
		// frame has actually been drawn and submitted, so the picture
		// written below is the second drawn frame and not the second
		// time round.
		if (capture != NULL && ++frames == CAPTURE_FRAMES)
			break;
	}

stop:
	// Between frames and never inside one, which is where the loop above
	// left off. Its three steps each say on stderr what refused, so there is
	// nothing to add here beyond the status.
	if (status == 0 && capture != NULL) {
		scratch = voe_base_arena_new(CAPTURE_SCRATCH);
		if (!voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW, scratch,
					 capture, &error))
			status = 1;
		voe_base_arena_destroy(scratch);
	}


	// The device and the window, then the arena — the app struct lives in
	// the arena and has to outlive every call made through it.
	voe_app_destroy(app);
	voe_base_arena_destroy(arena);
	return status;
}
