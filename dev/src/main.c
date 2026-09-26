// voe_dev — the one program a person runs to see what the engine can currently
// do: a window holding a world of cubes, a model read out of a `.glb` file,
// see-through quads, a dozen sprites off a sheet built in code, writing, element
// panels, a plate and ticks mapped onto the window, an interface with a heading,
// two buttons and three number boxes answering the mouse, a screen off to the
// left showing a second camera's view, one sun going round it all, and a camera
// that either orbits or is flown. There is one of these and it always shows the current state, so
// what is here now is deleted rather than kept behind a flag when the next thing
// lands.
//
// THIS FILE IS THE LOOP. Building all of it, once, before the first frame is
// src/startup.h's, and so is the state the loop reads; what each exhibit is and
// fails like stands in the file named for it — cubes.c, quad.c, text.c, model.c,
// sprites.c, monitor.c. What is measured, printed and shown is src/readout.h's,
// and how the eye and the sun move is src/motion.h's.
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING: which key means which
// direction, what is submitted in the gap between two systems, and the loop that
// runs them in order. Anything that starts to look worth keeping belongs in a
// folder, with a test — the moment it is worth testing it is in the wrong place.
//
// THE LOOP OWNS THE FRAME (ADR-0098), AND SINCE CARD 052 ITS PARTS COME FROM
// `app` (ADR-0135). voe_app_frame_open opens the frame, voe_app_draw_open and
// voe_app_draw_close bracket the draw, and between them, in a fixed order: build
// what changes this frame (the readout and the two panels' records); a pass onto
// the monitor's target with the monitor's camera; a pass onto the window with
// the world's camera; close, which presents. Building comes after the open
// because one-frame geometry needs the frame's slot, and before both passes
// because both walk the same world. An open that says there is nothing to draw
// into — no area, a stale swapchain — skips all of it; that case is the loop's
// and not the draw system's.
//
// THE MONITOR'S PASS IS FIRST BECAUSE THE WINDOW'S PASS SHOWS WHAT IT DREW. A
// target written after it has been read in the same frame would put last frame's
// picture on the screen — see src/monitor.h.
//
// WHAT `app` DOES NOT DO IS THE POINT OF IT. No loop, no callback and no
// function pointer: the `while` below is this file's, and so are the systems'
// order, what is submitted between them, the keys, the present mode and the
// readout. Only the lines every program would write the same way were factored.
//
// THREE THINGS EXIST HERE THAT WILL NOT EXIST HERE LONG — the camera path, the
// spin and the sun's path — each a call site's business only until the folder
// that owns it exists. The first and the third are src/motion.h's, with the
// keys flying reads; the spin is one line beside the cube it turns.
//
// NOT ONE #ifdef. If this file needs to know its operating system, platform/,
// render/device.h or 3d's headers have a hole, and that is the finding.
//
// It prints a line whenever something changes — the window's size, who draws
// its frame, whether the camera is flown, whether the pointer is locked — one
// per model at startup, timings every couple of seconds, and exits zero on close.
//
// TAB FLIES IT AND TAB HANDS IT BACK, AND BOTH STATES ARE WORTH LOOKING AT:
// whether the rendering is right wants a camera nobody touches, whether the
// input is right wants a hand on it. Escape hands the camera back, and closes
// the window when it is already back; the orbit is what the program starts in.
#include "elements.h"
#include "facing.h"
#include "interface.h"
#include "monitor.h"
#include "motion.h"
#include "readout.h"
#include "sprites.h"
#include "startup.h"
#include "surface.h"

#include <3d/draw_system.h>
#include <3d/panel_component.h>
#include <app/app.h>
#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>
#include <base/samples.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <platform/clock.h>
#include <platform/input.h>
#include <platform/window.h>
#include <render/device.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>
#include <text/font.h>

#include <stdio.h>


// A full turn, radians, for the turning cube's spin. src/motion.c keeps its own
// for the orbit and the sun.
#define TURN 6.2831853f

// How fast the turning cube turns about its tilted axis.
#define SPIN_SECONDS 4.0f
#define SPIN_AXIS_X 1.0f
#define SPIN_AXIS_Y 1.0f
#define SPIN_AXIS_Z 0.0f

// P is the one key that is not about the camera: it switches the present mode,
// in either camera state, and is nowhere near the movement keys for that reason.
//
// What is wrong if it looks wrong:
//
//   - The world's picture gone and only the overlay left — the depth clear
//     cleared colour as well. Only the depth aspect may be named; see
//     render/src/frame.c.
//   - THE PROGRAM STOPPING AT AN ASSERT NAMING A DRAW WHOSE SHADING RECORD
//     NAMES THE OPEN PASS'S TARGET TEXTURE — the monitor's `hidden` is not
//     reaching the walk, so the pass filling the target is drawing the screen
//     that shows it. It is a debug check (render/device.h): a release build
//     draws it, and what a person sees is whatever the driver felt like doing
//     with an image being read and written at once. See src/monitor.h.
//   - The screen darkening and going black as the sun crosses it — its material
//     is not unlit, so what is on it is the picture times a lambert term rather
//     than the picture.
//   - The screen holding one still picture, or noise, or the background colour —
//     nothing drew into its target this frame. A target nobody draws into keeps
//     whatever its frame slot last held, which is a picture several frames old
//     and then never changes; the monitor's pass is not being opened, or it was
//     opened onto the wrong target.
//   - The screen showing a screen showing a screen — `hidden` is naming the
//     wrong entity, and in a release build that is what the undefined read
//     happens to look like on a card that keeps the previous contents.
//   - The screen's picture stretched or squashed — it is square, drawn square
//     and shown on a square quad, so every one of those three has to agree; the
//     aspect ratio in src/monitor.c is the target's own and never the window's.
//   - Everything drifting or growing — the projection or the aspect ratio.
//
// What there is to try:
//
//   - Resize it. A `size` line should follow, the background should still reach
//     every corner, and the cubes should stay cubes rather than stretching — a
//     wider window shows more of the scene, it does not squash it.
//   - Toggle the frame off and on. On KWin: right-click the titlebar ->
//     More Actions -> No Borders, or Alt+F3. A `size` line follows and a
//     `decorated` line does not, which is the measured answer and not a gap.
//   - Minimise it. Nothing should happen and nothing should crash: a window with
//     no area has no frame to draw and the frame is skipped. The scene does not
//     advance while it is away, because the step below is inside that same test
//     — and the timing blocks stop, because a frame on a hidden window waits
//     for it to be shown again (0215).
//   - PRESS P AND COMPARE. It asks for mailbox — the uncapped rate, which is a
//     thing to measure and not the way this program runs (0215) — and the
//     `present` word on every block says which is actually in force: a machine
//     that has no mailbox goes on saying fifo, and that is an answer. What to
//     look for is `frame` coming off the display's refresh interval and the
//     rate climbing above it, in exchange for drawing frames nobody ever sees.
//     Press it again to come back.
//   - Close it. It should print `closed` and exit zero.
//
// IT OPENS IN STEP WITH THE DISPLAY AND COSTS ONE FRAME'S WORK PER REFRESH
// (0215). Nothing here asks for a present mode, so the device's own fifo stands
// and a focused window waits for the display; out of focus it drops to a few
// frames a second and hidden it draws nothing at all, which is
// voe_app_frame_open's doing and not this loop's. P is how to leave that:
// mailbox runs as fast as the card and the program between them allow, spinning
// a core and the graphics card too, and that rate is what P is pressed to
// measure.
int main(void)
{
	// Everything startup built and everything the loop writes back into it,
	// one member per local this function used to declare — see
	// src/startup.h. What is below it is the loop's own and nothing else's.
	struct voe_dev_program program;
	voe_base_error error = VOE_BASE_OK;
	// How many glyphs the readout laid out, printed once against the room
	// made for it — see VOE_DEV_TRANSIENT_GLYPHS.
	uint32_t readout_glyphs = 0;
	bool readout_reported = false;
	// Whether every surface's submits and draws were accepted, and the
	// frame's draw count either side of the draw system's walk — the
	// difference is what the walk cost, which is every mesh plus one per
	// panel.
	//
	// THE FIRST IS THE MONITOR'S WHOLE PASS, WHICH IS WHY THE PAIR IS A
	// SUBTRACTION AND NOT ONE READ. The monitor walks the same world into
	// its own target before the window's pass opens, so by the time the
	// window's walk starts the frame already holds that pass's draws. The
	// difference is still exactly what the window's walk cost, which is what
	// the line printed below claims it is.
	bool elements_ok = true;
	uint32_t draws_before_walk = 0;
	uint32_t draws_after_walk = 0;
	// Where each panel's own records start in this frame's element buffer.
	// One buffer, three ranges: the exhibit, the badge and the
	// screen-filling surface, which takes its own.
	uint32_t elements_first = 0;
	uint32_t elements_count = 0;
	uint32_t badge_first = 0;
	uint32_t interface_elements = 0;
	voe_math_float3 spin_axis = { SPIN_AXIS_X, SPIN_AXIS_Y, SPIN_AXIS_Z };
	// Where the eye is and where it looks, kept here from frame to frame:
	// orbited or flown, and turned into the eye's pose every frame.
	voe_dev_flight flight = voe_dev_orbit(0.0f);
	voe_scene_transform pose;
	const voe_scene_camera *lens;
	bool flying = false;
	bool was_flying = false;
	bool locked = false;
	// Last frame's Tab, because a toggle is an edge and platform hands out
	// state. Two bools at a call site is what include/platform/input.h says
	// this costs instead of an event queue, and this is that call site.
	bool tab_was_down = false;
	// And last frame's Escape, for the same reason and one more: Escape does
	// two different things depending on which camera is in force, so held
	// down it would do both, one frame after the other.
	bool escape_was_down = false;
	// Last frame's P, for the same reason. Which mode is asked for is the
	// program's; startup asks for nothing, so it begins on the device's fifo
	// (0215) and the first press is the one that asks for mailbox.
	//
	// What is actually in force is the device's answer and is asked for
	// rather than remembered: a surface with no mailbox leaves that true and
	// the device on fifo, and printing what was asked for would be a lie.
	bool p_was_down = false;
	voe_render_present present;
	// The scene's clock: measured now, and the sum of every step taken, not
	// of every second that passed. See the longest step src/startup.c asks
	// for, and the skip below.
	float seconds = 0.0f;
	// The real clock, and what it is read into. `top` is this frame's
	// reading, taken off the tick `app` hands back rather than read again
	// here, so the interval the readout reports and the step the scene takes
	// are the same subtraction — see voe_app_frame_open.
	double top;
	double after_update;
	double after_draw;
	double step;

	if (!voe_dev_start(&program)) {
		// The window and the device refusing is the one failure that
		// leaves non-zero and prints nothing of its own: `app` and
		// `render` have already said which of them refused and why, and
		// there is no window for a `closed` line to be about. Every
		// later refusal has a window behind it and leaves by the door a
		// close leaves by.
		if (program.app == NULL) {
			voe_base_arena_destroy(program.arena);
			return 1;
		}
		goto stop;
	}

	// THE LOOP IS THIS FILE'S AND THE PARTS IN IT ARE `app`'S (ADR-0135).
	// Everything between the calls below — the key edges, the order the
	// systems run in, what is submitted in the gap between two of them — is
	// this program deciding, which is why `app` hands back a frame instead
	// of running one.
	while (true) {
		voe_app_frame opened;
		voe_platform_size now_size;
		bool now_decorated;
		bool now_locked;
		bool tab_down;
		bool escape_down;
		bool p_down;
		const voe_scene_transform *spinning;
		double gpu_seconds;
		voe_3d_frame frame;
		bool drawing = false;
		bool readout_ok = true;

		// The clock, the poll and what the window says afterwards, in
		// that order and once. The window closing is the only reason
		// this loop ends that is not a key or a failure.
		opened = voe_app_frame_open(program.app);
		if (opened.closing)
			break;

		// The reading that opened the frame, kept because the update
		// and the draw are measured from it. Everything below is inside
		// the interval it ends.
		top = opened.tick.now;

		// WHAT IS REPORTED AND WHAT THE SCENE TAKES ARE THE TWO NUMBERS
		// ON THE TICK, AND THEY ARE NOT THE SAME ONE. `elapsed` is what
		// really happened and is what the readout says; `step` is that
		// with the longest step startup asked for on it, and is all the
		// orbit, the spin and the sun are advanced by. The first tick
		// has no interval behind it and says so, and recording it would
		// make the readout's first line claim four million frames a
		// second.
		if (!opened.tick.first)
			voe_base_samples_add(&program.timing.frame,
					     opened.tick.elapsed);
		step = opened.tick.step;

		// The frame opened with a poll in it, so what follows is this
		// program reading state that is already this frame's. Everything
		// is asked every frame because platform hands out state, not
		// events.
		now_size = opened.size;
		if (now_size.width != program.size.width ||
		    now_size.height != program.size.height) {
			program.size = now_size;
			printf("size       %dx%d\n", program.size.width,
			       program.size.height);
			fflush(stdout);
		}

		now_decorated = voe_platform_window_decorated(program.window);
		if (now_decorated != program.decorated) {
			program.decorated = now_decorated;
			printf("decorated  %s\n",
			       program.decorated ? "yes" : "no");
			fflush(stdout);
		}

		// Tab toggles on the press and not while held, which is what
		// turning state back into an edge means. Escape and P below do
		// the same and for the same reason: `platform` hands out which
		// keys are down, and all three of these are actions rather than
		// things held.
		tab_down = voe_platform_input_key_down(program.window,
						       VOE_PLATFORM_KEY_TAB);
		if (tab_down && !tab_was_down)
			flying = !flying;
		tab_was_down = tab_down;

		// ESCAPE HANDS THE CAMERA BACK, AND ESCAPE WITH THE CAMERA
		// ALREADY BACK CLOSES THE WINDOW. Two presses and not one,
		// because the first thing anybody wants out of a locked pointer
		// is the pointer: a key that released the pointer and quit in
		// the same press would quit every time somebody wanted their
		// mouse back.
		//
		// AND IT HAS TO BE AN EDGE, WHICH IT DID NOT WHEN IT ONLY EVER
		// HANDED BACK. Held down, one frame would hand the camera back
		// and the very next would close the window, so a single long
		// press would look like the program exiting for no reason.
		escape_down = voe_platform_input_key_down(
			program.window, VOE_PLATFORM_KEY_ESCAPE);
		if (escape_down && !escape_was_down) {
			if (flying)
				flying = false;
			else
				break;
		}
		escape_was_down = escape_down;

		// P asks for the other present mode, on the press and not while
		// held, exactly as Tab does. It starts from fifo, so the first
		// press asks for mailbox: a person pressing it is asking to
		// measure the uncapped rate (0215), which is not how this
		// program runs. What the device does about it is
		// asked for below rather than assumed: a surface with no mailbox
		// stays on fifo however often this is pressed, and that is a
		// measurement of the machine rather than a failure.
		p_down = voe_platform_input_key_down(program.window,
						     VOE_PLATFORM_KEY_P);
		if (p_down && !p_was_down) {
			program.mailbox_wanted = !program.mailbox_wanted;
			voe_render_present_set(program.gpu,
					       program.mailbox_wanted ?
						       VOE_RENDER_PRESENT_MAILBOX :
						       VOE_RENDER_PRESENT_FIFO);
		}
		p_was_down = p_down;

		if (flying != was_flying) {
			was_flying = flying;
			printf("camera     %s\n", flying ? "flying" : "orbit");
			fflush(stdout);
		}

		// Asked every frame rather than on the change, because a lock is
		// a request the window system may have taken away — losing focus
		// takes it — and asking again is how it comes back.
		voe_platform_input_lock_pointer(program.window, flying);

		now_locked = voe_platform_input_pointer_locked(program.window);
		if (now_locked != locked) {
			locked = now_locked;
			printf("locked     %s\n", locked ? "yes" : "no");
			fflush(stdout);
		}

		// The clock, and then everything that moves on it. A minimised
		// window draws nothing, and the clock stops with it: nothing
		// below advances a scene nobody is looking at. The loop still
		// goes round rather than skipping to the top, because the draw
		// below is what finds out when there is something to draw into
		// again.
		if (!opened.minimised) {
			seconds += (float)step;

			// One of the two, never both. Flying starts from the
			// flight the orbit last answered, which is exactly what
			// a handover wants: no jump on Tab.
			if (flying)
				flight = voe_dev_fly(program.window, flight,
						     (float)step);
			else
				flight = voe_dev_orbit(seconds);

			// The sun, as an intent like everything else.
			(void)voe_scene_light_submit(
				program.world,
				voe_dev_sunlight(program.sun, seconds));

			// The turning cube, as an intent like everything else.
			//
			// THE SPIN. The turning cube is a transform intent
			// submitted every frame. Same reasoning as the
			// orbit's: how a transform is written is `scene`'s,
			// what turns and how fast is a scene's own, and there
			// is no scene file yet.
			spinning = voe_scene_transform_get(program.world,
							   program.turning);
			if (spinning != NULL) {
				voe_scene_transform moved = *spinning;

				moved.rotation = voe_math_quat_from_axis_angle(
					spin_axis,
					seconds * TURN / SPIN_SECONDS);
				(void)voe_scene_transform_submit(
					program.world,
					(voe_scene_transform_intent){
						.entity = program.turning,
						.transform = moved });
			}
		}

		// The eye, moved like anything else in 3D space: its flight as a
		// transform intent (0222). Every frame, minimised or not, so the
		// pose everything below is placed from is the one it stands at.
		pose = voe_dev_flight_pose(flight);
		(void)voe_scene_transform_submit(
			program.world,
			(voe_scene_transform_intent){ .entity = program.eye,
						      .transform = pose });

		// The systems, in order, and then the draw. Each of them drains
		// what was submitted since it last ran; nothing here calls into
		// one system from another.
		// The camera system moves nothing: it drains lens intents, and
		// nothing here submits one.
		voe_scene_camera_system_run(program.world);

		// THE HEADS-UP LINE IS PLACED FROM THE POSE THE EYE IS SUBMITTED
		// WITH, AND THAT IS THE WHOLE OF WHETHER IT WORKS. Both intents
		// drain in the same run of the transform system, so the line
		// and the eye move together and nothing lags (0223).
		//
		// PLACING IT FROM LAST FRAME'S EYE WOULD PUT IT ONE FRAME BEHIND,
		// AND ONE FRAME IS PLENTY. It reads as jitter rather than as
		// lag, and the reason is worth writing down because the frame
		// rate makes it look impossible: a mouse delivers motion in
		// lumps, so at a thousand frames a second most frames turn the
		// camera by nothing and the occasional one turns it by the whole
		// of a lump. A line placed from the previous frame's camera is
		// therefore not a fraction of a millimetre out — it is a whole
		// mouse movement out, for one frame — and mailbox shows whatever
		// frame happens to be newest when the display asks, so some of
		// those frames are the ones a person sees. Drawing faster makes
		// it worse rather than better.
		(void)voe_scene_transform_submit(
			program.world,
			voe_dev_facing_the_camera(pose, program.hud,
						  program.hud_size));
		// The panel travels with the line, from the same pose.
		(void)voe_scene_transform_submit(
			program.world,
			voe_dev_behind_the_line(pose, program.panel,
						program.hud_size));
		// And the readout, placed from the pose and the lens alone — its
		// geometry does not exist yet and its placement does not need it.
		lens = voe_scene_camera_get(program.world, program.eye);
		if (lens != NULL)
			(void)voe_scene_transform_submit(
				program.world,
				voe_dev_top_left_of_the_view(
					pose, *lens, program.readout, now_size,
					VOE_DEV_READOUT_EM));
		// And the two sprites that turn towards the camera, from the
		// same flight. The engine does not billboard, so this is a call
		// site turning them itself — see src/sprites.c.
		voe_dev_sprites_face(program.world, flight, &program.sprites);

		voe_scene_transform_system_run(program.world);
		voe_scene_light_system_run(program.world);

		// The line between `update` and `draw`, and the reason the two
		// are measured apart: everything above is this program's own
		// work and everything below is the GPU's frame, the wait for it
		// included. One number covering both would not say which of them
		// grew.
		after_update = voe_platform_clock_now();
		voe_base_samples_add(&program.timing.update, after_update - top);

		// THE FRAME, IN THE ORDER THE HEADER GIVES: the camera and the sun
		// out of the tables, the draw opened, build what changes this
		// frame, a pass onto the monitor's target with the monitor's
		// camera, a pass onto the window with the world's camera, the
		// walk in each of them, and the draw closed. An open that says
		// there is nothing to draw into skips everything in the middle
		// and the loop comes round again — it does not wait, which is
		// what the spin on a minimised window is.
		frame = voe_3d_draw_system_frame(program.world, now_size, 0.0f);
		if (!voe_app_draw_open(program.app, now_size, &drawing))
			break;
		if (drawing) {
			voe_render_pass_camera pass_camera = {
				.view = frame.view,
				.light = frame.light,
			};
			// The monitor's own answer to the same question, and the
			// one place in this program where a second camera
			// exists. Its `hidden` is its screen — see
			// src/monitor.h for why that is not optional.
			voe_3d_frame monitor_frame = voe_dev_monitor_frame(
				&program.monitor, program.world);
			voe_render_pass_camera monitor_camera = {
				.view = monitor_frame.view,
				.light = monitor_frame.light,
			};

			// Built inside the frame and before either pass, because
			// that is the only place a one-frame mesh can be built
			// and still be drawn — and because both passes walk the
			// world, so anything built after the first of them would
			// be missing from that one and stale in it the frame
			// after. Its failure is looked at after the frame has
			// been ended, so the slot's fence is never left waiting
			// on a frame that was abandoned half recorded.
			readout_ok = voe_dev_build_the_readout(
				program.world, program.gpu, program.font,
				program.arena, program.readout, program.window,
				&program.timing, &readout_glyphs, &error);

			// THE TWO PANELS' CONTENT, BEFORE THE WALK, WHICH IS
			// THE PHASE THIS EXISTS FOR. A panel holds a range of
			// the frame that is open and the buffer is empty at
			// the top of every frame, so a range not written here
			// is a panel that is not drawn. Read the count, submit
			// one surface, read it again, and the difference is
			// that surface's range — there is no id and nothing
			// allocated.
			//
			// AND BEFORE ANY PASS, WHICH IS ALLOWED AND IS WHAT
			// TWO PASSES NEED. Element submission belongs to the
			// frame and not to a pass (render/device.h), so both
			// the monitor's walk and the window's draw the same
			// ranges of the same buffer — which is why the panels
			// are on the picture the monitor shows.
			//
			// NOTHING IS DRAWN HERE. The draw system issues one
			// draw per panel a moment later, in layer and sort
			// order, which is what lets a cube stand in front of
			// the exhibit.
			elements_first =
				voe_render_frame_elements_submitted(program.gpu);
			elements_ok = voe_dev_elements_submit(program.gpu,
							      program.font);
			elements_count = voe_render_frame_elements_submitted(
						 program.gpu) -
					 elements_first;
			(void)voe_3d_panel_set_range(program.world,
						     program.exhibit_panel,
						     elements_first,
						     elements_count);

			badge_first =
				voe_render_frame_elements_submitted(program.gpu);
			elements_ok = voe_dev_elements_badge_submit(program.gpu) &&
				      elements_ok;
			(void)voe_3d_panel_set_range(
				program.world, program.badge_panel, badge_first,
				voe_render_frame_elements_submitted(program.gpu) -
					badge_first);

			// THE MONITOR'S PASS, AND IT IS FIRST BECAUSE THE
			// WINDOW'S SHOWS WHAT IT DREW. The same world, the same
			// walk and the same tables, from a camera of its own
			// into a target of its own; the screen that shows the
			// result is left out of it by monitor_frame.hidden.
			// Second would be a target read in one pass and written
			// in the next, and what the window showed would be the
			// picture of the frame before.
			if (!voe_render_pass_begin(program.gpu,
						   program.monitor.target,
						   &monitor_camera)) {
				(void)voe_app_draw_close(program.app);
				break;
			}
			voe_3d_draw_system_run(program.world, program.gpu,
					       program.arena, monitor_frame);
			voe_render_pass_end(program.gpu);

			// The second pass of a frame on a device made with room
			// for two; refused only if that number were too small,
			// which is startup's mistake. The frame is still
			// closed on the way out, so its slot is not left half
			// recorded.
			if (!voe_render_pass_begin(program.gpu,
						   VOE_RENDER_TARGET_WINDOW,
						   &pass_camera)) {
				(void)voe_app_draw_close(program.app);
				break;
			}

			// EITHER SIDE OF THE WALK, WHICH IS WHAT THESE TWO
			// BRACKET AND ALL THEY BRACKET. The difference is every
			// mesh drawn plus one per panel — the panels' own draws
			// are issued inside the walk, sorted among the
			// see-through meshes, so there is no moment between
			// them to read a count at and that is card 032 working
			// rather than something missing here.
			draws_before_walk =
				voe_render_frame_draw_count(program.gpu);

			voe_3d_draw_system_run(program.world, program.gpu,
					       program.arena, frame);

			draws_after_walk =
				voe_render_frame_draw_count(program.gpu);

			// The screen-filling surface, after the walk because it
			// is not in the world and has nothing to sort against,
			// and before the end so that it is in this frame at
			// all. It is the one surface here that is not a panel
			// and the only one a resize changes — see src/surface.h.
			// Its failure is looked at after the frame has been
			// ended, for the reason the readout's is.
			elements_ok = voe_dev_surface_draw(program.gpu,
							   now_size) &&
				      elements_ok;

			// And the interface, on the same terms and for the
			// same reasons: it is not in the world either. It goes
			// after the surface so that it is painted over it,
			// which is what submission order means on this path.
			elements_ok = voe_dev_interface_draw(
					      program.gpu, program.interface,
					      program.arena, now_size,
					      voe_platform_input_pointer(
						      program.window),
					      voe_platform_input_button_down(
						      program.window,
						      VOE_PLATFORM_BUTTON_LEFT),
					      voe_platform_input_key_down(
						      program.window,
						      VOE_PLATFORM_KEY_SHIFT),
					      &interface_elements) &&
				      elements_ok;

			voe_render_pass_end(program.gpu);
			if (!voe_app_draw_close(program.app))
				break;

			// THE FRAME IS COMPLETE, SO THIS IS THE ONE MOMENT
			// EITHER NUMBER IS THE WHOLE FRAME'S. Both are read
			// here, after _end, so that what the readout shows and
			// what the console block prints are the same two reads
			// — two numbers about one frame that disagreed would be
			// worse than either alone.
			//
			// THE PAIR IS THE CLAIM AND NEITHER HALF IS. Records
			// submitted against commands recorded is what "many
			// small things in one draw" means; see
			// voe_render_frame_elements_submitted, which says why
			// reading the wrong one of the two makes the claim
			// trivially true. Both survive until the next _begin,
			// which is what lets them be read after the frame has
			// been submitted.
			voe_base_samples_add(
				&program.timing.draws,
				(double)voe_render_frame_draw_count(program.gpu));
			program.timing.drawn_elements =
				voe_render_frame_elements_submitted(program.gpu);
			program.timing.draws_measured = true;

			// The element capacity is smaller than the three
			// surfaces need, which is startup's mistake in the
			// same way the readout's transient room would be.
			if (!elements_ok) {
				VOE_BASE_ERROR("dev",
					       "could not submit or draw an element surface — see the refusal above");
				break;
			}
			// A readout that could not be built means the transient
			// room asked for at startup is too small for it, which
			// is this program's mistake and worth stopping over
			// rather than a refusal line on stderr every frame for
			// as long as it runs.
			if (!readout_ok) {
				VOE_BASE_ERROR("dev",
					       "could not build the readout: %s",
					       voe_base_error_string(error));
				break;
			}
			if (!readout_reported) {
				printf("readout    %u glyphs — %u of %u transient vertices, %u of %u indices, 1 of %u ranges\n",
				       readout_glyphs, readout_glyphs * 4u,
				       4u * VOE_DEV_TRANSIENT_GLYPHS,
				       readout_glyphs * 6u,
				       6u * VOE_DEV_TRANSIENT_GLYPHS,
				       (unsigned)VOE_DEV_TRANSIENT_GEOMETRIES);
				// THREE ELEMENT SURFACES, AND THE TWO NUMBERS
				// AT THE END ARE THE SAME PAIR THE READOUT
				// SHOWS. They are read from the same two calls
				// after the same _end, so the console and the
				// screen cannot disagree — and the whole frame
				// is what is printed rather than the surfaces
				// alone, because the panels are drawn inside
				// the walk, sorted among the see-through
				// meshes, and there is no moment between them
				// to read a count at. That is card 032 working
				// rather than a limitation: a panel drawn in a
				// pass of its own would be easier to count and
				// would be the bug.
				//
				// The walk's own cost is printed beside it,
				// which is every mesh plus one per panel.
				printf("interface  %u element records for a panel, a heading, two buttons and three number boxes, in ONE draw command\n",
				       interface_elements);
				printf("elements   %u rectangles of %u colours and %u letters on the world panel, %u on the badge, %u on the screen-filling surface\n",
				       (unsigned)VOE_DEV_ELEMENTS_RECTANGLES,
				       (unsigned)VOE_DEV_ELEMENTS_RECTANGLES,
				       (unsigned)VOE_DEV_ELEMENTS_GLYPHS,
				       (unsigned)VOE_DEV_BADGE_ELEMENTS,
				       (unsigned)VOE_DEV_SURFACE_ELEMENTS);
				// This frame's exact numbers, read straight from
				// the device rather than out of the period
				// metric beside it: both survive until the next
				// _begin, and one frame's commands against one
				// frame's records is the pair ADR-0092's claim
				// is made of. The averaged version of the same
				// number is in every timing block below.
				printf("draws      %u commands for %u element records; the window's walk was %u of them\n",
				       voe_render_frame_draw_count(program.gpu),
				       program.timing.drawn_elements,
				       draws_after_walk - draws_before_walk);
				fflush(stdout);
				readout_reported = true;
			}
		}

		after_draw = voe_platform_clock_now();
		voe_base_samples_add(&program.timing.draw,
				     after_draw - after_update);

		// The card's own measurement of a frame two frames back, when
		// there is one. Asked after the draw because that is what moved
		// it on; a card that cannot time never answers and the gpu line
		// says so rather than reading nought.
		if (voe_render_frame_gpu_time(program.gpu, &gpu_seconds)) {
			voe_base_samples_add(&program.timing.gpu, gpu_seconds);
			program.timing.gpu_timed = true;
		}

		// The mode is asked for every period rather than remembered,
		// because a rebuild is what puts a requested mode in force and
		// that happens inside the draw above.
		if (after_draw - program.timing.started >=
		    VOE_DEV_REPORT_SECONDS) {
			present = voe_render_present_get(program.gpu);
			voe_dev_report(&program.timing,
				       after_draw - program.timing.started,
				       present);
			program.timing.started = after_draw;
		}
	}

stop:
	// The font holds GPU resources, so it goes before the device `app`
	// closes; the arena goes last of the three because the app struct is in
	// it.
	voe_text_font_destroy(program.font);
	voe_app_destroy(program.app);
	voe_base_arena_destroy(program.arena);
	printf("closed\n");
	return 0;
}
