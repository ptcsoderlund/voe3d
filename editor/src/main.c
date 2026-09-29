// voe_editor — the program a person opens to author a scene. It opens a window
// on a top bar over three columns — `Scene`, two scene views stacked,
// `Inspector` — whose rectangles came out of a tree of data (dock.h) and not
// out of the order of the calls here, lists the project's authored entities,
// follows a click on one, and draws the world into each view from that view's
// own camera. The bar's New, Open and Save are session.h's: this file calls
// voe_editor_session_do and takes a refused window close back.
//
// WHICH PROJECT IT OPENS ON IS startup.h's; THE WORLD'S STEP IS world_step.h's.
//
// IT DRAWS IN THE THEME REMEMBERED IN `<settings>/voe3d/theme` (themes.h),
// looked at again once a second at the top of the loop, in the one font the
// editor makes, Oxanium (ADR-0185). A theme that is gone or refused draws Near
// black and says why in session.notice, unless a failure already put one there.
// The panels' sizes and the views' share are the default tree's with
// settings.h's file read over them.
// Their borders are dragged through resize.h, asked before anything else reads the pointer.
//
// OPEN AND, ON AN UNTITLED PROJECT, SAVE TOO SHOW browser.h'S OWN FILE BROWSER,
// which outlives every project. While it shows the middle-button drag moves no
// view's camera and (in interface.c) the top bar's buttons are drawn but never
// asked what the pointer did; a window close still asks session.h what it has.
//
// WHAT THIS FRAME'S KEYBOARD ASKED FOR IS ONE READ (shortcuts.h): the guards
// this file holds go in, a flag per shortcut comes back, and every act on one
// is below that read — the three commands through session.h, Delete and Ctrl+D
// through scene.h, Ctrl+Z, Ctrl+Shift+Z and Ctrl+Y through undo.h, whose step
// is taken at the top of the next frame, before the structural queue is
// applied. Escape's order is this file's (at the picker's close below).
// Escape, Backspace, Enter, Tab and the text read since the last poll are the
// interface's besides (dock.h, keys.h). A flying view keeps every key it reads
// from the interface and the shortcuts, and the pointer from both (view.h).
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING, the same standing dev/ has:
// the window's size, the loop and the one division that turns
// the mouse's pixels into the surface's millimetres. The left button is the
// interface's; the middle and right are the views'. THE PARTS IN THE LOOP ARE
// `app`'S (ADR-0135): a frame is opened, and a draw is bracketed, by it.
//
// A FRAME IS A PASS PER VIEW AND THEN ONE ONTO THE WINDOW (ADR-0148). Each view
// is drawn into its own target with its own camera first; the window's pass
// comes last, with no camera — how `render` is told there is no eye to invent —
// and shows each view's picture on its panel. The per-view part, and the
// device's capacities it needs, are view_passes.h's.
//
// IT CAN ALSO BE STARTED TO WRITE ONE PICTURE AND LEAVE. `--capture <path>`
// (options.h) opens the device with no window at all (voe_app_new_headless),
// builds the same world, font, themes, interface and scene, runs the loop body
// twice and writes the picture, both counted and written by capture.h. It first
// waits out a running refresh (session.h), for at most 120 s of the frame clock.
// EVERY READ OF THE WINDOW IS GUARDED.
#include "assets_drag.h"
#include "browser.h"
#include "capture.h"
#include "dock.h"
#include "gizmo.h"
#include "inspector_edit.h"
#include "interface.h"
#include "keys.h"
#include "models.h"
#include "preferences.h"
#include "notice.h"
#include "options.h"
#include "pick.h"
#include "project.h"
#include "resize.h"
#include "scene.h"
#include "scene_list.h"
#include "session.h"
#include "settings.h"
#include "shortcuts.h"
#include "startup.h"
#include "themes.h"
#include "undo.h"
#include "view.h"
#include "view_passes.h"
#include "world_step.h"

#include <3d/shape_geometry.h>
#include <3d/shape_system.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/world.h>

#include <platform/arguments.h>
#include <platform/clock.h>
#include <platform/input.h>
#include <platform/window.h>

#include <render/device.h>

#include <text/font.h>

#include <ui/widgets.h>

#include <stdio.h>
#include <string.h>

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

// How long a capture waits for the project's code to build before it gives up.
#define CAPTURE_REFRESH_SECONDS 120.0

#define EDITOR_WIDE 1280
#define EDITOR_HIGH 720

// How far one notch of the wheel scrolls, in the surface's millimetres. WHAT A
// NOTCH IS WORTH IS THIS PROGRAM'S TO CHOOSE (ADR-0153 point 10): `platform`
// counts detents and `ui` is handed a length, so the one multiplication between
// them is here, beside the division that turns the mouse's pixels into the same
// millimetres.
#define WHEEL_MILLIMETRES 10.0f

int main(int argc, char *argv[])
{
	// What the command line said (options.h), holding the window's size
	// until --size says otherwise.
	voe_editor_options options = { .wide = EDITOR_WIDE,
				       .high = EDITOR_HIGH };
	// How many frames a capture has drawn so far.
	unsigned frames = 0;
	// Whether a capture has waited on a refresh, and the frame clock's
	// reading when it began to.
	bool refresh_waited = false;
	double refresh_since = 0.0;
	// The project being worked on, its notice and its refuse-once state
	// (session.h); its project is never NULL past startup.h's call.
	voe_editor_session session = { 0 };
	// The last notice a refused theme save put in session.notice, so the
	// next good save clears it only while it is still the one showing.
	voe_editor_notice theme_notice = { 0 };
	// Last frame's keyboard, which this frame's edges are found against,
	// every key every frame (keys.h). A held key does not repeat, only
	// edge, because a repeating key waits on a later typing feature.
	voe_editor_keys keys = { 0 };
	// Shown by Open, once its unsaved-changes refusal is past; hidden by
	// its own Cancel, Escape, or a folder it opened successfully
	// (session.h). Kept across the whole program's run, never one
	// project's — see browser.h on why it is not part of `session`.
	voe_editor_browser browser = { 0 };
	// The top bar, kept between frames for the height it measured last
	// (topbar.h).
	voe_editor_topbar bar = { 0 };
	// Shown by the bar's Preferences, hidden by its Close or by Escape
	// while the browser is not showing (preferences.h).
	voe_editor_preferences preferences = { 0 };
	voe_app_settings settings;
	// The panels' sizes the tree and the bar start at (settings.h).
	voe_editor_settings panel_sizes;
	voe_base_arena *arena;
	voe_base_arena *scratch;
	voe_app *app;
	voe_base_error error;
	voe_platform_window *window;
	voe_render_device *gpu;
	// The built-in shapes' GPU side, uploaded once at startup and read
	// every frame by world_step.h.
	voe_3d_shapes shapes;
	// The one model store, destroyed before the device (models.h).
	voe_editor_models *models = NULL;
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
	// The left press in a view that moves the selection (pick.h), beside
	// the drag it shares the pointer with.
	voe_editor_pick pick = { 0 };
	// A model row held from the Assets panel (assets_drag.h).
	voe_editor_assets_drag drag = { 0 };
	// A GIZMO DRAG IS ONE MORE READER OF THE POINTER (gizmo.h, ADR-0205).
	// The middle button is the views'; the left is asked of the gizmo first
	// and of picking second, so a press on an arrow moves the entity and
	// selects nothing. One drag is one undo step without anything counted
	// here: the held button keeps `at_rest` false for every frame of it, so
	// undo.h settles on the release.
	voe_editor_gizmo gizmo = { 0 };
	// The borders between the root's panels, at rest (resize.h).
	voe_editor_resize resize = { .held = UINT32_MAX };
	voe_editor_resize_result resized;
	// The line of scene texts Ctrl+Z steps back through, made out of
	// `arena` below (undo.h). Beside the scene and the views because the
	// history is the editor's and never the world's.
	voe_editor_undo undo = { 0 };
	// This frame's undo and redo edges, acted on at the top of the next
	// frame, and whether the editor was at rest when they were read — the
	// same rest a step is recorded at (undo.h).
	bool step_back = false;
	bool step_forward = false;
	bool at_rest = false;
	// Whether a view flew last frame, so the pointer's lock is asked for
	// only on the frame that changes (platform/input.h).
	bool flew = false;
	// The built-in shapes' CPU side: the triangles a pick ray is cast
	// against, built once into the kept arena because the store outlives
	// every frame (3d/shape_geometry.h).
	voe_3d_shape_geometries geometries;
	int status = 0;

	// THE COMMAND LINE IS READ BEFORE ANYTHING IS OPENED, so a mistyped
	// argument costs nothing and says so straight away (options.h). It is
	// read as UTF-8 (platform/arguments.h), into the arena on Windows, so
	// the arena is the one thing made ahead of it.
	arena = voe_base_arena_new(EDITOR_ARENA);
	if (!voe_editor_options_read(
		    voe_platform_arguments_read(argc, argv, arena), &options)) {
		voe_base_arena_destroy(arena);
		return 2;
	}

	// WHICH PROJECT OPENS, BEFORE THE DEVICE DOES (startup.h): an argued
	// folder that would not open has already said why on stderr.
	if (!voe_editor_startup_project(&options, &session)) {
		voe_base_arena_destroy(arena);
		return 1;
	}

	voe_editor_undo_create(&undo, arena);

	settings = (voe_app_settings){ .width = options.wide,
				       .height = options.high,
				       .title = "voe3d editor",
				       .capacities = VOE_EDITOR_CAPACITIES,
				       .longest_step = MAX_FRAME_SECONDS };

	// The device, with the window before it or with no window at all, in one
	// call. Nothing is kept out of `scratch`, so it goes as soon as this
	// returns, and nothing is printed on a failure: `app` says which piece
	// refused and `render` says why, both on stderr, before it returns NULL.
	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	app = options.capture != NULL ?
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
	models = voe_editor_models_new();

	// The same three kinds on the CPU, for the ray a click is cast as
	// (pick.h). No device in it, and the kept arena because the store is
	// read for as long as the editor runs.
	voe_3d_shape_geometries_create(arena, &geometries);

	// project.world lives in project's own arena, not this program's — see
	// project.h on why every project owns its own world. NEW replaces it
	// later, and session.c keeps this in step when it does (session.h).
	scene.world = session.project->world;

	// The views open where the scene's camera is, not at the origin (0255).
	voe_editor_views_focus_camera(&views, scene.world);

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
	// The default tree's sizes, then whatever the person left (settings.h);
	// a capture reads them too, as it reads the themes.
	panel_sizes = (voe_editor_settings){
		.scene_wide = voe_editor_dock_panel_length(
			&roots[0].tree, VOE_EDITOR_PANEL_SCENE),
		.inspector_wide = voe_editor_dock_panel_length(
			&roots[0].tree, VOE_EDITOR_PANEL_INSPECTOR),
		.view_share = voe_editor_dock_view_share(&roots[0].tree),
		.assets_tall = VOE_EDITOR_DOCK_ASSETS_TALL,
	};
	voe_editor_settings_read(&panel_sizes);
	voe_editor_dock_panel_length_set(&roots[0].tree, VOE_EDITOR_PANEL_SCENE,
					 panel_sizes.scene_wide);
	voe_editor_dock_panel_length_set(&roots[0].tree,
					 VOE_EDITOR_PANEL_INSPECTOR,
					 panel_sizes.inspector_wide);
	voe_editor_dock_panel_length_set(&roots[0].tree, VOE_EDITOR_PANEL_ASSETS,
					 panel_sizes.assets_tall);
	voe_editor_dock_view_share_set(&roots[0].tree, panel_sizes.view_share);
	bar.wanted = panel_sizes.topbar_high;

	voe_editor_startup_say_descriptions();
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
		bool right = false;
		voe_platform_motion motion = { 0 };
		// Whether a view flies this frame (view.h), read before the
		// shortcuts so a flying view's keys command nothing.
		bool flying;
		// This frame's keyboard, read once below (keys.h).
		voe_editor_keys_frame keyboard;
		bool shift;
		bool control;
		// What this frame's keyboard asked for (shortcuts.h).
		voe_editor_shortcuts shortcuts;
		// This frame's Escape edge when neither typing nor the picker
		// took it: the browser's Cancel and Preferences' Close. This
		// file's own, because closing the picker spends the edge.
		bool escape_free;
		float pixels_per_millimetre;
		bool drawing = false;
		bool drawn = true;
		// The light every view is shown with this frame (view.h), read
		// once rather than once per view: every view is lit the same.
		voe_render_light light = { 0 };

		// A BUILT REFRESH LANDS HERE, before the undo take and the
		// world step, so the new world's rows get their meshes before
		// anything draws them (session.h). A swapped world holds none
		// of the entities Add component's list was open for.
		if (voe_editor_session_step(&session, &scene))
			voe_editor_inspector_add_close(&scene.inspector);

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

		// A DIFFERENT PROJECT EMPTIES THE HISTORY, AND OTHERWISE LAST
		// FRAME'S CTRL+Z OR CTRL+Y IS TAKEN HERE — before world_step.h,
		// so the rows a step puts back are given their meshes before
		// anything draws them (undo.h). A step leaves the project
		// unsaved and is never itself an edit to record.
		if (session.replaced) {
			session.replaced = false;
			voe_editor_undo_forget(&undo);
			voe_editor_views_focus_camera(&views, scene.world);
		} else if ((step_back || step_forward) &&
			   voe_editor_undo_take(&undo, session.project, &scene,
						&session.notice, step_forward)) {
			voe_editor_session_edited(&session);
		}
		step_back = false;
		step_forward = false;

		// The queue applied and every owning system run (world_step.h).
		voe_editor_world_step(session.project->world, &shapes);

		// THE STEP LAST FRAME'S EDIT SETTLES INTO, once the world holds
		// it: an edit reaches it through an intent or the structural
		// queue, so nothing above this line has it yet (undo.h).
		voe_editor_undo_settle(&undo, session.project, arena, at_rest);

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
		// EVERY READ OF THE WINDOW IS IN HERE. Notches since the last
		// poll are turned into a length once, and nothing else reads the
		// wheel: a scene view takes the middle button and no wheel at
		// all, so a wheel over one is a scroll no area under the pointer
		// can take and is dropped.
		if (window != NULL) {
			pointer = voe_platform_input_pointer(window);
			wheel = voe_platform_input_wheel(window);
			text = voe_platform_input_text(window);
			left = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_LEFT);
			middle = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_MIDDLE);
			right = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_RIGHT);
			motion = voe_platform_input_motion(window);
		}
		// Beside them, and the one read of the keyboard there is. It
		// takes the window being NULL itself, so a capture reads a
		// keyboard with nothing down and no edge on it (keys.h).
		voe_editor_keys_read(&keys, window, &keyboard);
		shift = keyboard.down[VOE_PLATFORM_KEY_SHIFT];
		control = keyboard.down[VOE_PLATFORM_KEY_CONTROL];

		// THE POINTER'S DIVISION LIVES HERE AND NOWHERE ELSE (ADR-0141
		// point 4), because the day a panel is a quad standing in the
		// world that conversion is a ray against the quad — a different
		// sum, in a different file, and only a call site can know which
		// it wants. The interface, the views' drag and pick.h get the
		// same millimetres; the wheel is WHEEL_MILLIMETRES below.
		roots[0].pointer = (voe_ui_pointer){
			.at = { pointer.x / pixels_per_millimetre,
				pointer.y / pixels_per_millimetre },
			.over = pointer.over,
			.down = left,
			.fine = shift,
			.scroll = { wheel.x * WHEEL_MILLIMETRES,
				    wheel.y * WHEEL_MILLIMETRES }
		};

		// THE RIGHT BUTTON FLIES THE VIEW IT WENT DOWN OVER (view.h),
		// never while the browser shows, turned by the mouse's motion and
		// moved by W, S, A, D, E and Q, Shift three times as fast. While it
		// flies the pointer is locked and hidden, and put back where it
		// was on the frame it stops.
		flying = voe_editor_views_fly(
			&views, roots[0].pointer.at, right && !browser.showing,
			(voe_math_float2){ motion.x, motion.y },
			(voe_editor_fly_keys){
				.forward = keyboard.down[VOE_PLATFORM_KEY_W],
				.back = keyboard.down[VOE_PLATFORM_KEY_S],
				.left = keyboard.down[VOE_PLATFORM_KEY_A],
				.right = keyboard.down[VOE_PLATFORM_KEY_D],
				.up = keyboard.down[VOE_PLATFORM_KEY_E],
				.down = keyboard.down[VOE_PLATFORM_KEY_Q],
				.fast = shift },
			(float)opened.tick.step);
		if (flying != flew && window != NULL)
			voe_platform_input_lock_pointer(window, flying);
		flew = flying;

		// WHICH EDGE MEANT WHICH COMMAND IS ANSWERED ONCE, HERE
		// (shortcuts.h), out of the keyboard above and the guards this
		// file is the one holding; everything below it is this file
		// acting on a flag.
		shortcuts = voe_editor_shortcuts_read(
			&keyboard, (voe_editor_shortcuts_guards){
					   .browser_showing = browser.showing,
					   .typing = voe_ui_typing(ui),
					   .picker_open = scene.picking.open,
					   .dropdown_open = scene.dropdown.open,
					   .pointer_down = left,
					   .flying = flying });

		// Ctrl+N, Ctrl+O and Ctrl+S are the bar's three commands.
		if (shortcuts.new_project)
			voe_editor_session_do(&session, &scene, &browser,
					      VOE_EDITOR_COMMAND_NEW);
		if (shortcuts.open)
			voe_editor_session_do(&session, &scene, &browser,
					      VOE_EDITOR_COMMAND_OPEN);
		if (shortcuts.save)
			voe_editor_session_do(&session, &scene, &browser,
					      VOE_EDITOR_COMMAND_SAVE);

		// Delete and Ctrl+D act after the interface has drawn, because
		// the dock walk zeroes the scene's `structural` and `full` for
		// the frame (scene.h); Ctrl+Z and Ctrl+Y at the top of the next
		// frame, with the rest they were read at beside them.
		at_rest = shortcuts.at_rest;
		step_back = shortcuts.undo;
		step_forward = shortcuts.redo;

		// ESCAPE'S ORDER IS THIS FILE'S, out of the free edge that read
		// leaves: a Scene list drag under way is cancelled first, then
		// the picker closes and goes no further, otherwise it is the
		// browser's Cancel or Preferences' Close.
		escape_free = shortcuts.escape_free;
		if (escape_free && voe_editor_scene_list_cancel(&scene))
			escape_free = false;
		if (escape_free && scene.picking.open) {
			voe_editor_scene_picker_close(&scene);
			escape_free = false;
		}
		// THE BROWSER KEEPS ESCAPE WHILE IT SHOWS; otherwise it hides
		// Preferences and the Errors panel, which it does nothing else
		// to. Neither while a person is typing: then it cancels that
		// and nothing more.
		if (escape_free && !browser.showing) {
			voe_editor_preferences_hide(&preferences);
			voe_editor_errors_hide(&session.errors);
		}

		// BESIDE THE POINTER, AND FOR THE SAME REASON (dock.h): `ui`
		// reads this for whichever field or number box is focused, and
		// nothing here decides which one that is.
		// A flying view keeps the keys it reads and the pointer: `ui`
		// gets no text, no Backspace, Enter or Tab, and no pointer.
		roots[0].keyboard = (voe_ui_keyboard){
			.text = flying ? NULL : text.bytes,
			.size = flying ? 0 : text.size,
			.backspace = !flying &&
				     keyboard.pressed[VOE_PLATFORM_KEY_BACKSPACE],
			.enter = !flying && keyboard.pressed[VOE_PLATFORM_KEY_ENTER],
			.escape = shortcuts.escape,
			.tab = !flying && keyboard.pressed[VOE_PLATFORM_KEY_TAB],
		};
		if (flying) {
			roots[0].pointer.over = false;
			roots[0].pointer.down = false;
			left = false;
		}

		// THE BORDERS ARE ASKED FIRST (resize.h): a seam is a fill the
		// walk draws, not a widget, so no widget answers for it. While
		// they have the pointer, `ui` sees no pointer and pick and the
		// gizmo no left button; the border they reach is drawn lit.
		resized = voe_editor_resize_frame(
			&resize, &roots[0], &bar,
			!browser.showing && !preferences.showing &&
				!session.errors.showing &&
				!scene.picking.open && !scene.dropdown.open &&
				!flying,
			voe_platform_clock_now());
		roots[0].lit = resized.reached;
		if (resized.taken) {
			roots[0].pointer.over = false;
			roots[0].pointer.down = false;
			left = false;
		}
		if (window != NULL)
			voe_platform_input_cursor(window, resized.cursor);
		if (resized.ended) {
			voe_base_report_error_clear();
			if (!voe_editor_resize_remember(&roots[0].tree, &bar))
				voe_editor_notice_from_report(&session.notice,
							      "editor_settings");
		}

		// The middle button is the views' and the left is the
		// interface's, so the two never compete for one press. Never
		// while the browser shows — "views get no drag" (browser.h) —
		// so a press that started before it opened does not carry on
		// moving a camera underneath it.
		voe_editor_views_drag(&views, roots[0].pointer.at,
				      middle && !browser.showing, shift,
				      control);

		// The left half of the same division: a press on an arrow of
		// the selected entity's gizmo drags it (gizmo.h), unless a panel
		// over the views has the press instead.
		voe_editor_gizmo_read(&gizmo, &scene, &views,
				      VOE_EDITOR_GIZMO_MILLIMETRES *
					      pixels_per_millimetre,
				      roots[0].pointer.at, left && pointer.over,
				      browser.showing || preferences.showing ||
					      session.errors.showing ||
					      scene.picking.open);

		// A held model row, released over a view or the Inspector
		// (assets_drag.h), under the pick's own `blocked`.
		voe_editor_assets_drag_read(
			&drag, &session, &undo, &scene, &views, &roots[0], &bar,
			&geometries, voe_editor_models_store(models),
			roots[0].pointer.at, left && pointer.over,
			browser.showing || preferences.showing ||
				session.errors.showing || scene.picking.open ||
				voe_editor_gizmo_taking(&gizmo));

		// Then a press over a view picks what is under it (pick.h). A
		// press the gizmo or a drag took is not a press that selects,
		// and the order is the point: the gizmo is asked first.
		voe_editor_pick_read(&pick, &scene, &views, &geometries,
				     voe_editor_models_store(models),
				     roots[0].pointer.at, left && pointer.over,
				     browser.showing || preferences.showing ||
					     session.errors.showing ||
					     scene.picking.open ||
					     voe_editor_gizmo_taking(&gizmo) ||
					     drag.holding);

		// Before the draw is opened, so a resize asked for here is
		// applied by this frame's begin and the picture is drawn at the
		// size it is shown at — last frame's rectangle, see view.h.
		for (uint32_t v = 0; v < views.count; v++)
			if (voe_editor_dock_shows_view(&roots[0].tree, v))
				voe_editor_view_fit(&views.views[v], gpu,
						    pixels_per_millimetre);

		// The files this frame's rows name, before the draw opens (models.h).
		voe_editor_models_update(models, &session, gpu, arena,
					 opened.tick.now);
		voe_editor_assets_update(&scene.assets, session.project->folder,
					 opened.tick.now);
		if (!voe_app_draw_open(app, opened.size, &drawing)) {
			status = 1;
			break;
		}
		if (!drawing)
			continue;

		light = voe_editor_view_light(session.project->world);

		// A pass per view the tree shows (view_passes.h). If one is
		// refused, the frame is still closed below and the program stops.
		// The preview's pass comes first, drawn only while the selected
		// entity has a camera.
		drawn = voe_editor_view_passes_preview(
				gpu, arena, session.project->world, &views,
				light, &scene, voe_editor_models_store(models)) &&
			voe_editor_view_passes_draw(
			gpu, arena, session.project->world, &views,
			&roots[0].tree, light, &scene, &geometries,
			voe_editor_models_store(models), &shapes,
			&voe_editor_themes_chosen(&themes)->palette, &gizmo,
			pixels_per_millimetre);

		// Then one pass onto the window, with no camera, for the
		// interface and the pictures on it.
		if (drawn)
			drawn = voe_render_pass_begin(
				gpu, VOE_RENDER_TARGET_WINDOW, NULL);
		if (drawn) {
			drawn = voe_editor_interface_draw(
				gpu, ui, arena, roots,
				(uint32_t)(sizeof roots / sizeof roots[0]),
				&scene, &views, &session, &bar, &browser,
				&preferences, &themes,
				escape_free);
			// Only when the Inspector's own buttons changed nothing
			// structural this frame: two changes before the queue is
			// applied would be given one id (entities.h).
			if (scene.structural == 0 && shortcuts.delete_entity)
				voe_editor_scene_delete(&scene);
			if (scene.structural == 0 && shortcuts.duplicate)
				voe_editor_scene_duplicate(&scene);
			if (shortcuts.gizmo_switch)
				voe_editor_scene_gizmo_switch(&scene);
			if (scene.full)
				voe_editor_notice_set(&session.notice,
						      "The scene is full.");
			// AN EDIT REACHED THE PROJECT, AND NOTHING ABOVE ASKED
			// FOR IT AS A COMMAND — an Inspector number dragged, an
			// entity Add entity, Delete or Duplicate queued, a gizmo
			// move: the other half of what marks the project
			// unsaved (session.h, scene.h, gizmo.h).
			if (scene.inspector.replaced > 0 ||
			    scene.structural > 0 || gizmo.moved > 0) {
				voe_editor_session_edited(&session);
				voe_editor_undo_edited(&undo);
			}
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

		// A CAPTURE'S LOOP HAS TO STOP ITSELF (capture.h), counted here
		// where a frame has actually been drawn and submitted. A
		// running refresh is waited for, its frames not counted, for at
		// most CAPTURE_REFRESH_SECONDS of the frame clock.
		if (options.capture != NULL &&
		    session.refresh.stage != VOE_EDITOR_REFRESH_IDLE) {
			if (!refresh_waited)
				refresh_since = opened.tick.now;
			refresh_waited = true;
			if (opened.tick.now - refresh_since >
			    CAPTURE_REFRESH_SECONDS) {
				fprintf(stderr,
					"voe_editor: the capture gave up on the project's code after %d s\n",
					(int)CAPTURE_REFRESH_SECONDS);
				status = 1;
				break;
			}
		} else if (voe_editor_capture_enough(options.capture, &frames)) {
			break;
		}
	}

stop:
	// Between frames, where the loop above left off; capture.h says what
	// refused.
	if (status == 0 && !voe_editor_capture_write(app, options.capture))
		status = 1;

	// The device and the window, then the arena — the app struct lives in
	// the arena and has to outlive every call made through it. The
	// project and the browser are each a separate arena again, and
	// outlive none of this, so they go last — the browser's own may
	// never have been made at all, on a run Open was never once asked
	// for, which is voe_editor_browser_destroy's to tell apart.
	voe_editor_refresh_end(&session.refresh);
	voe_editor_models_destroy(models, gpu);
	voe_app_destroy(app);
	voe_base_arena_destroy(arena);
	voe_editor_project_destroy(session.project);
	voe_editor_browser_destroy(&browser);
	voe_editor_assets_destroy(&scene.assets);
	voe_editor_themes_destroy(&themes);
	return status;
}
