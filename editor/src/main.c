// voe_editor — the program a person opens to author a scene. Today it opens a
// window on a top bar over three columns — `Scene`, two scene views stacked,
// `Inspector` — whose rectangles came out of a tree of data rather than out of
// the order of the calls in this file, lists the authored entities of the
// project it opens on, follows a click on one, and draws the world into each
// view from that view's own camera. A middle-button drag in a view moves that
// view's camera. The bar's New, Open and Save — and Ctrl+N, Ctrl+O and Ctrl+S,
// the same three commands — are session.h's to carry out; what this file does
// with them is call voe_editor_session_do and, for a window close, take the
// close back when it is refused.
//
// WHICH PROJECT IT OPENS ON IS project.h'S, ARGUED HERE. `voe_editor
// [<folder>]` opens folder (project.h's voe_editor_project_new_opened); a
// folder that cannot be opened prints `voe_editor: <why>` on stderr and this
// program does not start (project.h names the file and the line). With no
// folder, last_project.h's one remembered path is tried the same way, and a
// project that can no longer be opened is not fatal there — the editor falls
// back to an untitled cube and light, and the notice explaining why goes into
// session.notice, for the bar to show, as well as onto stderr. A first start,
// with no folder argued and nothing remembered, is silently the same untitled
// scene. Unless `--capture` is given, whichever project is now open is
// written back as the last one, warning on stderr rather than stopping if that
// write fails; `--capture` never writes it, because a capture is not a person
// opening the editor.
//
// IT DRAWS IN THE THEME REMEMBERED IN `<settings>/voe3d/theme` (themes.h).
// The editor makes one font, Oxanium (ADR-0185), the themes folder is read
// into a list whose every palette is derived with it, whatever face a file
// names, and the chosen palette and its font are set on the interface. A
// remembered theme that is gone or refused draws Near black instead and puts
// the reason, naming the file, in session.notice — unless a last project's
// failure already put one there. Every frame, before the systems run,
// voe_editor_themes_check looks at the chosen file once a second: a good save
// is set on the interface, palette and font, for this frame to draw in; a
// mistake leaves the last good theme drawing and fills session.notice from
// the reader's report with the file's name, and the next good save clears
// that notice — only if it is still that one, so it never erases a project's.
//
// OPEN AND, ON AN UNTITLED PROJECT, SAVE TOO SHOW browser.h'S OWN FILE
// BROWSER. `browser`, beside `scene` and `views`, outlives every project the
// whole run through, and while it shows this file does two things for it and
// interface.c a third: Ctrl+N, Ctrl+O and Ctrl+S fire nothing, the
// middle-button drag moves no view's camera, and (in interface.c) the top
// bar's buttons are drawn but never asked what the pointer did to them, so a
// click on one is not carried out — and the dock is handed a pointer that
// cannot be hit either, the anchored browser already painting over it.
// Escape is this file's own edge, exactly as the other three shortcuts are,
// handed to the interface as the browser's Cancel; a window close still asks
// session.h the same question it always has, browser or not.
//
// THE BAR'S PREFERENCES SHOWS preferences.h'S PANEL IN THE SAME PLACE, and
// it suppresses nothing: the shortcuts and the drag go on as ever. Escape
// hides it when the browser is not showing; while the browser shows, Escape
// is the browser's. WHILE voe_ui_typing SAYS A FIELD OR NUMBER BOX HELD THE
// KEYBOARD AT THE LAST FRAME'S END, ESCAPE IS `ui`'s ALONE: it cancels the
// typing and neither cancels the browser nor hides Preferences that frame.
// OTHERWISE, WHILE THE COLOUR PICKER IS OPEN (scene.h's `picking`), ESCAPE
// CLOSES IT AND DOES NOTHING ELSE: typing first, then the picker, then the
// browser and Preferences.
//
// DELETE AND CTRL+D DELETE AND DUPLICATE THE SELECTED ENTITY (scene.h), on
// their down edge, and neither fires while the browser shows or while
// voe_ui_typing says a field holds the keyboard.
//
// BACKSPACE, ENTER AND TAB ARE THE SAME SHAPE OF EDGE, FOR WHATEVER FIELD OR
// NUMBER BOX HOLDS THE KEYBOARD — SAVE mode's name box, the Inspector's name
// and its numbers. None is this file's to act on: all three and Escape, with
// whatever voe_platform_input_text read since the last poll, are read every
// frame regardless of whether the browser shows and handed to the interface
// as this frame's voe_ui_keyboard (dock.h, interface.c) — `ui` acts on them
// only for whichever widget is focused.
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
// A FRAME STARTS WITH THE WORLD'S STRUCTURAL QUEUE APPLIED (ADR-0193), before
// the first system runs, so a row the Add menu queued last frame is there for
// every system this frame and none sees one appear partway through its run.
//
// A FRAME IS A PASS PER VIEW AND THEN ONE ONTO THE WINDOW (ADR-0148). Each view
// the tree shows is drawn into its own target with its own camera first; the
// window's pass comes last, is opened with no camera — an element draw needs
// none, and a pass without one is how `render` is told there is no eye to invent
// — and shows each view's picture as an image on its panel. The world is drawn
// through voe_3d_draw_system_run, but with the view's own camera and the light
// this file reads out of the world (view.h) rather than with
// voe_3d_draw_system_frame, which reads a camera out of the world too and this
// world's camera is never there — a view's is the editor's own.
//
// THE LIGHT IS THE WORLD'S FIRST LIGHT ROW, OR ZERO INTENSITY WHEN IT HAS NONE.
// Every view is lit the same way, once a frame, by whichever light `Light` (or
// whatever a saved project called it) carries; a world with none — nothing to
// see by, rather than a crash — is what a broken or half-built scene draws as.
//
// IT CAN ALSO BE STARTED TO WRITE ONE PICTURE AND LEAVE. `--capture <path>`
// opens the device with no window at all (voe_app_new_headless), builds the
// same world, font, themes, interface and scene, runs the same loop body,
// writes the frame to `path` through voe_app_capture_png and returns. `--size <W>x<H>`
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
// capture: the pointer, the wheel, the buttons, the keys and the typed text
// are zeroed input then, and nothing else in the loop changes shape — the
// same passes in the same order with the same draws, so the captured frame
// is the frame a person sees.
//
// main's SIGNATURE IS C'S OWN, `char *argv[]`, WHICH IS THE ONE DEVIATION FROM
// RULE 6 IN THIS FOLDER (DEVIATION: rule 6, an array of pointers is spelled as
// the array it is, because the C runtime calls main with this signature and
// nothing in this program chose it). Nothing else here holds a pointer to a
// pointer, and the argument list is read in main and nowhere else.
#include "browser.h"
#include "dock.h"
#include "interface.h"
#include "preferences.h"
#include "last_project.h"
#include "notice.h"
#include "project.h"
#include "scene.h"
#include "session.h"
#include "themes.h"
#include "view.h"

#include <3d/draw_system.h>
#include <3d/shape_system.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/structure.h>
#include <ecs/world.h>

#include <platform/input.h>
#include <platform/window.h>

#include <render/device.h>

#include <scene/identity_system.h>
#include <scene/light_system.h>
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

// WHAT THE EDITOR UPLOADS IS ONE CUBE AND ONE MATERIAL — the shapes' own, see
// 3d/shape_system.h — which is what makes the geometry numbers its constants
// and `shadings` a one. `objects` is per frame: every drawn entity is one
// object in every view's pass, so it is the room for drawn entities
// (VOE_EDITOR_PROJECT_MAX_DRAWN, project.h's — every project's world is
// registered with that much room for a mesh and a material, so a device that
// draws one is sized from the same number) times the room for views. `passes`
// is a pass per view and the interface's, and `targets` a target per view —
// both from the room for views, not the two in use, so a third view is a leaf
// and not a capacity. The three transient numbers stay nought; see
// render/include/render/device.h.
#define EDITOR_CAPACITIES                                                     \
	(voe_render_capacities)                                               \
	{                                                                     \
		.vertices = VOE_3D_SHAPES_VERTICES,                            \
		.indices = VOE_3D_SHAPES_INDICES,                              \
		.geometries = VOE_3D_SHAPES_GEOMETRIES,                        \
		.objects = VOE_EDITOR_PROJECT_MAX_DRAWN * VOE_EDITOR_VIEWS,    \
		.shadings = VOE_3D_SHAPES_SHADINGS,                            \
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

// The light every view is shown with: the world's first light row, or a light
// of zero intensity — every surface black — when the world holds none. `render`
// does not normalize the direction (render/device.h) but voe_scene_light_add
// and the light system already have, so the row's is copied straight across.
static voe_render_light world_light(const voe_ecs_world *world)
{
	const voe_scene_light *row;

	if (voe_scene_light_count(world) == 0)
		return (voe_render_light){ 0 };

	row = &voe_scene_light_rows(world)[0];

	return (voe_render_light){ .direction = row->direction,
				   .colour = row->colour,
				   .intensity = row->intensity };
}

// One line, on stderr, and the return code that goes with it. Every way of
// getting the command line wrong ends here: there is one form to state and
// stating it twice in different words would be two forms to keep in step.
static int usage(void)
{
	fprintf(stderr,
		"usage: voe_editor [<folder>] [--capture <path> [--size <W>x<H>]]\n");
	return 2;
}

int main(int argc, char *argv[])
{
	// Where one picture is written, and NULL when the editor opens a window
	// instead. The size is the picture's, and it is the window's size until
	// --size says otherwise.
	const char *capture = NULL;
	// The one non-`--` argument, or NULL when none was given — see project
	// below on what each case opens.
	const char *folder = NULL;
	int wide = EDITOR_WIDE;
	int high = EDITOR_HIGH;
	bool sized = false;
	// How many frames a capture has drawn so far.
	unsigned frames = 0;
	// The project being worked on, its notice and its refuse-once state —
	// session.h. `session.project` is argued, remembered or untitled — see
	// the block below main's own locals — and never NULL past it: every
	// branch either fills it in or this program has already returned.
	voe_editor_session session = { 0 };
	// Only ever this program's own words, for the one project a folder on
	// the command line named and could not open: this program stops right
	// there, before session.notice would ever be read (see below), so a
	// notice destined for the bar would be one nobody could show.
	voe_editor_notice notice;
	// The last notice a refused theme save put in session.notice, so the
	// next good save clears it only while it is still the one showing.
	voe_editor_notice theme_notice = { 0 };
	// Last frame's Ctrl+N, Ctrl+O and Ctrl+S: a shortcut toggles on the
	// press and not while held, exactly as dev/src/main.c's Tab does, and
	// the modifier is folded into the level read every frame rather than
	// tracked on its own — holding Control and tapping N is the same edge
	// as holding N and tapping Control.
	bool new_was_down = false;
	bool open_was_down = false;
	bool save_was_down = false;
	// Last frame's Delete and Ctrl+D, the same shape — carried out on the
	// scene's selection (scene.h) rather than through session.h.
	bool delete_was_down = false;
	bool duplicate_was_down = false;
	// Last frame's Escape, the same shape without a modifier — the
	// browser's own Cancel (browser.h) while it shows, and Preferences'
	// Close (preferences.h) while it does not.
	bool escape_was_down = false;
	// Last frame's Backspace and Enter, the same shape again — task 14's
	// name box (browser.h) is the one thing either reaches, through
	// `ui`'s own voe_ui_keyboard and not through a command of this
	// file's. A held key does not repeat here, only edge, because a
	// repeating key waits on a later typing feature (spec's Defaults).
	bool backspace_was_down = false;
	bool enter_was_down = false;
	// Last frame's Tab, the same shape: `ui` moves the focus on it.
	bool tab_was_down = false;
	// Shown by Open, once its unsaved-changes refusal is past; hidden by
	// its own Cancel, Escape, or a folder it opened successfully
	// (session.h). Kept across the whole program's run, never one
	// project's — see browser.h on why it is not part of `session`.
	voe_editor_browser browser = { 0 };
	// Shown by the bar's Preferences, hidden by its Close or by Escape
	// while the browser is not showing (preferences.h).
	voe_editor_preferences preferences = { 0 };
	voe_app_settings settings;
	voe_base_arena *arena;
	voe_base_arena *scratch;
	voe_app *app;
	voe_base_error error;
	voe_platform_window *window;
	voe_render_device *gpu;
	// The built-in shapes' GPU side: their geometry and the one white
	// material every shape wears. Uploaded once, at startup, and read every
	// frame by voe_3d_shape_system_run.
	voe_3d_shapes shapes;
	// The one font the editor carries, Oxanium (ADR-0185); themes.h
	// derives every palette with it.
	voe_text_font *oxanium;
	// The two themes with no file, every one in the themes folder, and
	// the one in force (themes.h). It outlives `ui`, which keeps the palette.
	voe_editor_themes themes = { 0 };
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
	//
	// ONE ARGUMENT NOT STARTING WITH `--` IS THE FOLDER; A SECOND ONE IS
	// USAGE. There is only ever one project to open, so a second bare
	// argument is not something this program can mean anything by.
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
		} else if (argv[a][0] != '-' && folder == NULL) {
			folder = argv[a];
		} else {
			return usage();
		}
	}
	// A size with nothing to size: the window's is the window system's
	// answer and not this program's to name (app.h), so the only picture
	// --size could mean is one nobody asked for.
	if (sized && capture == NULL)
		return usage();

	// WHICH PROJECT OPENS, BEFORE THE DEVICE DOES: a folder argued on the
	// command line is opened or this program stops right here, on stderr,
	// before a window would ever have shown (criterion 12). With none, the
	// last project remembered (last_project.h) is tried the same way, but
	// its failure is not this program's to stop over — an untitled cube
	// and light is what a lost or broken last project falls back to, and
	// the notice explaining why goes into session.notice as well as onto
	// stderr, because the bar the session shows it in is up from the first
	// frame (criterion 11).
	if (folder != NULL) {
		session.project = voe_editor_project_new_opened(folder, &notice);
		if (session.project == NULL) {
			fprintf(stderr, "voe_editor: %s\n", notice.text);
			return 1;
		}
	} else {
		voe_base_arena *last_scratch =
			voe_base_arena_new(STARTUP_SCRATCH);
		const char *last = voe_editor_last_project_read(last_scratch);

		if (last != NULL) {
			session.project = voe_editor_project_new_opened(
				last, &session.notice);
			if (session.project == NULL) {
				fprintf(stderr, "voe_editor: %s\n",
					session.notice.text);
				session.project =
					voe_editor_project_new_untitled();
			}
		} else {
			session.project = voe_editor_project_new_untitled();
		}
		voe_base_arena_destroy(last_scratch);
	}

	// REMEMBERED FOR NEXT TIME, UNLESS THIS IS A CAPTURE OR THERE IS NO
	// FOLDER TO REMEMBER. An untitled project — whether this is a first
	// start or a last project that could not be opened — writes nothing
	// (criterion 11), and a capture draws what a start would have opened
	// without ever being the thing that decides what a start opens next.
	if (capture == NULL && session.project->folder != NULL &&
	    !voe_editor_last_project_write(session.project->folder))
		VOE_BASE_WARNING(
			"editor",
			"could not remember %s as the last project opened",
			session.project->folder);

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
		voe_editor_project_destroy(session.project);
		return 1;
	}

	window = voe_app_window(app);
	gpu = voe_app_device(app);

	// Both upload, so both are startup operations and both come before the
	// first frame. `render` says why on stderr when it refuses.
	if (!voe_3d_shapes_upload(gpu, &shapes, &error) ||
	    !voe_editor_views_create(&views, gpu, &error)) {
		status = 1;
		goto stop;
	}

	// project.world lives in project's own arena, not this program's — see
	// project.h on why every project owns its own world. NEW replaces it
	// later, and session.c keeps this in step when it does (session.h).
	scene.world = session.project->world;

	oxanium = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, gpu, arena,
				    &error);
	if (oxanium == NULL) {
		VOE_BASE_ERROR("editor", "the editor could not build its font: %s",
			       voe_base_error_string(error));
		status = 1;
		goto stop;
	}

	// A REMEMBERED THEME THAT IS GONE OR REFUSED IS A NOTICE, NOT A STOP:
	// Near black is drawn instead and the bar says why, naming the file,
	// as a failed open does. A last project's own notice, set above, is
	// the one left standing when both failed — it says more about what is
	// on screen.
	if (!voe_editor_themes_load(&themes, oxanium) &&
	    session.notice.text[0] == '\0')
		voe_editor_notice_from_report(&session.notice,
					      themes.remembered);

	ui = voe_editor_interface_new(arena,
				      &voe_editor_themes_chosen(&themes)->palette);
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
		voe_platform_text text = { 0 };
		bool left = false;
		bool middle = false;
		bool shift = false;
		bool control = false;
		bool escape = false;
		bool backspace = false;
		bool enter = false;
		bool tab = false;
		// This frame's Escape, Backspace, Enter and Tab edges, handed to
		// the interface below — set once each has been read, further
		// down.
		bool escape_fired;
		bool backspace_fired;
		bool enter_fired;
		bool tab_fired;
		// Whether a field or number box held the keyboard at the last
		// frame's end, when this frame's Escape is `ui`'s alone.
		bool typing;
		// This frame's Escape edge when neither typing nor the picker
		// took it: the browser's Cancel and Preferences' Close.
		bool escape_free;
		// This frame's Delete and Ctrl+D edges, carried out after the
		// interface has drawn — see where they are read.
		bool delete_fired;
		bool duplicate_fired;
		float pixels_per_millimetre;
		bool drawing = false;
		bool drawn = true;
		// The light every view is shown with this frame: the world's
		// first light row, or a light of zero intensity when it holds
		// none — see the top of this file on what that draws as. Read
		// once, before the pass loop, rather than once per view: every
		// view is lit the same light the same way.
		voe_render_light light = { 0 };

		// EVERY OWNING SYSTEM RUNS EVERY FRAME, WHETHER ANYTHING
		// SUBMITTED OR NOT (ADR-0134 point 7). An intent that reaches a
		// queue on a frame its system does not drain is an edit that
		// lands whenever the loop next happens to run it, which is a
		// class of bug that does not exist if the run is unconditional —
		// the Inspector can submit a replace intent for any editable
		// field the moment it draws one, transform's and light's alike,
		// so all three are run from the start rather than from whenever
		// somebody remembers a first submit needs one.
		// LIVE EDITING (themes.h): at most once a second, the chosen
		// theme's file read again; set on the context before this
		// frame's first ui call, so the frame draws in it.
		switch (voe_editor_themes_check(&themes)) {
		case VOE_EDITOR_THEMES_CHANGED:
			voe_ui_font_set(
				ui, voe_editor_themes_chosen(&themes)->palette.font);
			voe_ui_theme_set(
				ui, &voe_editor_themes_chosen(&themes)->palette);
			if (theme_notice.text[0] != '\0' &&
			    strcmp(session.notice.text, theme_notice.text) == 0)
				voe_editor_notice_clear(&session.notice);
			voe_editor_notice_clear(&theme_notice);
			break;
		case VOE_EDITOR_THEMES_REFUSED:
			voe_editor_notice_from_report(&session.notice,
						      themes.remembered);
			theme_notice = session.notice;
			break;
		case VOE_EDITOR_THEMES_UNCHANGED:
			break;
		}

		// Which rows exist changes here and nowhere else in the frame
		// (ecs/structure.h), before any system below reads a table.
		voe_ecs_structure_apply(session.project->world);

		voe_scene_transform_system_run(session.project->world);
		voe_scene_identity_system_run(session.project->world);
		voe_scene_light_system_run(session.project->world);

		// Drains the shape's intent, like the three above, and gives a
		// fresh shape its mesh and material the first frame it exists
		// (3d/shape_system.h).
		voe_3d_shape_system_run(session.project->world, &shapes);

		// The clock, the poll, and what the window says afterwards, in
		// that order and once.
		opened = voe_app_frame_open(app);
		if (opened.closing) {
			// A CLOSE THAT GOES AHEAD IS THE ONLY WAY OUT OF THIS
			// LOOP BESIDES A FAILURE BELOW. A refused one takes the
			// window's own close back and carries on exactly as
			// though nothing had asked for it — the notice
			// explaining why is session.notice's, read by the bar
			// next frame.
			if (voe_editor_session_do(&session, &scene, &browser,
						  VOE_EDITOR_COMMAND_CLOSE))
				break;
			voe_platform_window_close_refuse(window);
		}
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
			text = voe_platform_input_text(window);
			left = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_LEFT);
			middle = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_MIDDLE);
			shift = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_SHIFT);
			control = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_CONTROL);
			escape = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_ESCAPE);
			backspace = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_BACKSPACE);
			enter = voe_platform_input_key_down(
				window, VOE_PLATFORM_KEY_ENTER);
			tab = voe_platform_input_key_down(window,
							  VOE_PLATFORM_KEY_TAB);
		}

		// CTRL+N, CTRL+O AND CTRL+S DO WHAT THEIR BUTTON DOES, on the
		// frame the key goes down with Control already held — the
		// modifier is folded into the level before the edge compare,
		// the same shape dev/src/main.c's Tab toggle already has, just
		// with Control mixed into what is compared. A capture has no
		// window, so `control` stays false above and none of these
		// ever reads true.
		//
		// AND NONE OF THE THREE FIRES WHILE THE BROWSER SHOWS — "top
		// bar commands and shortcuts are ignored" (browser.h) — though
		// the edge is still tracked every frame, so a shortcut held
		// through the browser opening and closing does not fire the
		// moment it is let through.
		{
			bool new_down = control && voe_platform_input_key_down(
							   window, VOE_PLATFORM_KEY_N);
			bool open_down = control && voe_platform_input_key_down(
							    window, VOE_PLATFORM_KEY_O);
			bool save_down = control && voe_platform_input_key_down(
							    window, VOE_PLATFORM_KEY_S);

			if (new_down && !new_was_down && !browser.showing)
				voe_editor_session_do(&session, &scene,
						      &browser,
						      VOE_EDITOR_COMMAND_NEW);
			new_was_down = new_down;

			if (open_down && !open_was_down && !browser.showing)
				voe_editor_session_do(&session, &scene,
						      &browser,
						      VOE_EDITOR_COMMAND_OPEN);
			open_was_down = open_down;

			if (save_down && !save_was_down && !browser.showing)
				voe_editor_session_do(&session, &scene,
						      &browser,
						      VOE_EDITOR_COMMAND_SAVE);
			save_was_down = save_down;
		}

		// DELETE AND CTRL+D ARE THE SAME SHAPE OF EDGE, and neither fires
		// while the browser shows or while a field or number box holds
		// the keyboard — a person typing is not also commanding
		// (ui/widgets.h's voe_ui_typing). They are carried out after the
		// interface has drawn, because the dock walk zeroes the scene's
		// `structural` and `full` for the frame (scene.h).
		{
			bool delete_down =
				window != NULL &&
				voe_platform_input_key_down(
					window, VOE_PLATFORM_KEY_DELETE);
			bool duplicate_down = control && voe_platform_input_key_down(
								 window, VOE_PLATFORM_KEY_D);
			bool quiet = browser.showing || voe_ui_typing(ui);

			delete_fired = delete_down && !delete_was_down && !quiet;
			delete_was_down = delete_down;
			duplicate_fired = duplicate_down && !duplicate_was_down &&
					  !quiet;
			duplicate_was_down = duplicate_down;
		}

		// THIS FRAME'S ESCAPE, BACKSPACE, ENTER AND TAB EDGES, HANDED TO
		// THE INTERFACE BELOW — all four as `ui`'s keyboard for whichever
		// field or number box is focused, and Escape besides as the
		// browser's own Cancel (browser.h) when nobody is typing; a
		// capture has no window, so all four stay false and none of them
		// ever fires there either. NONE REPEATS WHILE HELD, for the
		// reason the local variables above already say.
		escape_fired = escape && !escape_was_down;
		escape_was_down = escape;
		typing = voe_ui_typing(ui);
		// THE PICKER TAKES ESCAPE WHEN NOBODY IS TYPING, and the edge
		// goes no further (the header's order).
		escape_free = escape_fired && !typing;
		if (escape_free && scene.picking.open) {
			voe_editor_scene_picker_close(&scene);
			escape_free = false;
		}
		// THE BROWSER KEEPS ESCAPE WHILE IT SHOWS; otherwise it hides
		// Preferences, which it does nothing else to. Neither while a
		// person is typing: then it cancels that and nothing more.
		if (escape_free && !browser.showing)
			voe_editor_preferences_hide(&preferences);
		backspace_fired = backspace && !backspace_was_down;
		backspace_was_down = backspace;
		enter_fired = enter && !enter_was_down;
		enter_was_down = enter;
		tab_fired = tab && !tab_was_down;
		tab_was_down = tab;

		roots[0].pointer = (voe_ui_pointer){
			.at = { pointer.x / pixels_per_millimetre,
				pointer.y / pixels_per_millimetre },
			.over = pointer.over,
			.down = left,
			.fine = shift,
			.scroll = { wheel.x * WHEEL_MILLIMETRES,
				    wheel.y * WHEEL_MILLIMETRES }
		};
		// BESIDE THE POINTER, AND FOR THE SAME REASON (dock.h): `ui`
		// reads this for whichever field or number box is focused, and
		// nothing here decides which one that is.
		roots[0].keyboard = (voe_ui_keyboard){
			.text = text.bytes,
			.size = text.size,
			.backspace = backspace_fired,
			.enter = enter_fired,
			.escape = escape_fired,
			.tab = tab_fired,
		};

		// The middle button is the views' and the left is the
		// interface's, so the two never compete for one press. Never
		// while the browser shows — "views get no drag" (browser.h) —
		// so a press that started before it opened does not carry on
		// moving a camera underneath it.
		voe_editor_views_drag(&views, roots[0].pointer.at,
				      middle && !browser.showing, shift,
				      control);

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

		light = world_light(session.project->world);

		// A pass per view the tree shows, each onto its own target with
		// its own camera and the world's light. A device made with a
		// pass per view and one more does not refuse these; if it did,
		// the frame is still closed below and the program stops.
		for (uint32_t v = 0; v < views.count && drawn; v++) {
			const voe_editor_view *view = &views.views[v];
			voe_render_pass_camera camera;

			if (!voe_editor_dock_shows_view(&roots[0].tree, v))
				continue;

			camera = voe_editor_view_pass_camera(view, light);
			drawn = voe_render_pass_begin(gpu, view->target,
						      &camera);
			if (!drawn)
				break;
			voe_3d_draw_system_run(
				session.project->world, gpu, arena,
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
				&scene, &views, &session, &browser,
				&preferences, &themes,
				escape_free);
			// Only when the Inspector's own buttons changed nothing
			// structural this frame: two changes before the queue is
			// applied would be given one id (entities.h).
			if (scene.structural == 0 && delete_fired)
				voe_editor_scene_delete(&scene);
			if (scene.structural == 0 && duplicate_fired)
				voe_editor_scene_duplicate(&scene);
			if (scene.full)
				voe_editor_notice_set(&session.notice,
						      "The scene is full.");
			// AN EDIT REACHED THE PROJECT, AND NOTHING ABOVE ASKED
			// FOR IT AS A COMMAND — dragging a number in the
			// Inspector is not New, Open, Save or Close, so
			// session.h has no case for it; this is the other half
			// of what marks the project unsaved (session.h).
			// And so is an entity the Add menu, Delete or Duplicate
			// queued (scene.h).
			if (scene.inspector.replaced > 0 ||
			    scene.structural > 0)
				voe_editor_session_edited(&session);
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
	// the arena and has to outlive every call made through it. The
	// project and the browser are each a separate arena again, and
	// outlive none of this, so they go last — the browser's own may
	// never have been made at all, on a run Open was never once asked
	// for, which is voe_editor_browser_destroy's to tell apart.
	voe_app_destroy(app);
	voe_base_arena_destroy(arena);
	voe_editor_project_destroy(session.project);
	voe_editor_browser_destroy(&browser);
	voe_editor_themes_destroy(&themes);
	return status;
}
