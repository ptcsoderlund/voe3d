// voe_dev — the one program a person runs to see what the engine can currently
// do: a window holding a world of cubes, a model read out of a `.glb` file,
// see-through quads, a dozen sprites off a sheet built in code, writing, element
// panels, a plate and ticks mapped onto the window, an interface with a heading,
// two buttons and three number boxes answering the mouse, a screen off to the
// left showing a second camera's view, one sun going round it all, and a camera
// that either orbits or is flown. There is one of these and it always shows the current state, so
// what is here now is deleted rather than kept behind a flag when the next thing
// lands. What each exhibit is and fails like stands above what builds it: here,
// or in the file named for it — cubes.c, quad.c, text.c, model.c, sprites.c.
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING: which key means which
// direction, where a placeholder cube stands, and the loop that runs the systems
// in order. Anything that starts to look worth keeping belongs in a folder, with
// a test — the moment it is worth testing it is in the wrong place.
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
// THREE THINGS LIVE HERE THAT WILL NOT LIVE HERE LONG — the camera path, the
// spin and the sun's path — each a call site's business only until the folder
// that owns it exists; each says why where it is submitted.
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
#include "cubes.h"
#include "elements.h"
#include "facing.h"
#include "interface.h"
#include "model.h"
#include "monitor.h"
#include "surface.h"
#include "quad.h"
#include "sprites.h"
#include "text.h"

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <app/app.h>
#include <base/arena.h>
#include <base/assert.h>
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
#include <scene/camera_component.h>
#include <scene/transform_system.h>
#include <text/font.h>

#include <math.h>
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

// What the GPU makes room for per frame: geometry that lives one frame, which
// today is the readout and nothing else. Sized in glyphs because that is what
// fills it — four vertices and six indices each — and the readout is about
// seventy of them, so this is under double with nothing "to be safe" in it. The
// program prints what the readout actually took beside this number, once, so the
// next thing that needs transient room has a measurement to start from. Two
// ranges: the readout is one, and the other is for the next thing.
#define MAX_TRANSIENT_GLYPHS 128
#define MAX_TRANSIENT_GEOMETRIES 2

// The longest step the scene is ever advanced by, in seconds, however long the
// frame actually took.
//
// THE CLOCK IS REAL NOW AND CARD 020 IS WHAT MADE IT ONE. Every frame is stepped
// by however long the last one actually took, and not by a nominal sixtieth of a
// second — so the orbit takes the number of seconds it says it does on a display
// of any refresh rate. Since card 052 the reading and the subtraction are
// voe_app_frame_open's: it hands back both numbers, the interval that happened
// and that interval clamped, and this file reports the first and steps the scene
// by the second. MAX_FRAME_SECONDS is the ceiling it is handed, and nothing
// clamps what is reported.
//
// IT IS A CLAMP ON THE SCENE AND NOT ON THE MEASUREMENT. The numbers reported
// below are what the clock said, always; this is only what the orbit, the spin
// and the sun are stepped by. Without it, a frame that took two seconds — the
// window dragged to another monitor, the machine swapping, a debugger stopped at
// a breakpoint — teleports everything a sixth of the way round its lap in one
// step, and what a person sees is a scene that jumped rather than a frame that
// was slow. A quarter of a second is longer than any frame worth watching and
// shorter than any pause worth catching up on.
#define MAX_FRAME_SECONDS 0.25

// How often the timing block below is printed, in seconds. Each block is an
// average and a worst over exactly the period since the last one, so this is
// also the window every number in it is measured over. Two seconds is long
// enough to average a hundred frames and short enough that changing something
// and looking at the console is the same motion.
#define REPORT_SECONDS 2.0

// A full turn, radians.
#define TURN 6.2831853f

// The camera the program starts with. A sixty-degree vertical field of view, a
// near plane close enough to walk up to something and a far plane past anything
// in the scene.
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The orbit: how far out, how high, and how long a lap takes. Three motions that
// can each be told apart — see orbit().
//
// THE RADIUS IS WHATEVER FITS WHAT IS IN THE SCENE, and the scene is about six
// metres across. It has grown twice for that reason and shrank once, when the
// lettered test model came out and took the left-hand end of the scene with it;
// the radius was left where it was, so there is more empty space on the left
// than on the right now. It decides nothing and is a number to change when
// somebody minds.
#define ORBIT_RADIUS 7.0f
#define ORBIT_HEIGHT 1.8f
#define ORBIT_SECONDS 12.0f

// How fast the turning cube turns about its tilted axis.
#define SPIN_SECONDS 4.0f
#define SPIN_AXIS_X 1.0f
#define SPIN_AXIS_Y 1.0f
#define SPIN_AXIS_Z 0.0f

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

// The readout: the same distance in front of the eye as the heads-up line,
// smaller than it, and snapped into the top-left corner of the view. Where the
// corner is in metres at that distance follows from the camera's field of view
// and the window's aspect ratio — see src/facing.h — so it stays in the corner
// when the window is resized. The string it holds is formatted into a buffer
// this long, which is well over the seven short lines it now holds.
#define READOUT_EM 0.040f
#define READOUT_CHARS 192

// The sun: how long a lap takes, how high it sits, and how strong it is.
//
// IT MOVES BECAUSE A STILL LIGHT PROVES NOTHING. A light that never moves is
// indistinguishable from a light pointing the wrong way, from a normal matrix
// that is the world matrix, and from shading that is stuck to the screen rather
// than to the surface. One lap every twenty seconds is slow enough to watch a
// face brighten and fast enough not to have to wait.
//
// THE HEIGHT IS THE VERTICAL PART OF THE DIRECTION IT TRAVELS, so a negative
// number is a sun above the scene shining downwards — see
// scene/light_component.h on which way a direction points. Not so steep that
// the sides of things go dark and not so shallow that the tops do.
//
// THE INTENSITY IS ABOVE ONE BECAUSE THE DIFFUSE TERM DIVIDES BY PI. A surface
// facing a white light of one comes back at about a third of its albedo, which
// is a scene that looks underexposed; π is what makes "one" mean "as bright as
// the texture". There is no exposure control and no tone mapping yet, so this is
// a number that looks right rather than a number that means something.
#define SUN_SECONDS 20.0f
#define SUN_HEIGHT (-0.8f)
#define SUN_INTENSITY 3.14159265f

// Where the model is put, once, after it is imported. A file places its
// contents wherever its author left them — a model should not have an opinion
// about what else is in the scene — so this is the call site moving it out of
// the cubes' way, and it moves it the way anything moves anything: by
// submitting a transform intent.
#define HUMAN_X 3.5f

// The bindings, and the only thing in this file that decides anything. Which key
// means forward is a call site's business — the engine's job is to know what
// forward does, not which finger asks for it.
//
// OPPOSITE KEYS HELD TOGETHER CANCEL, WHICH IS WHAT ADDING AND SUBTRACTING GIVES
// FOR FREE. W and S together is nought and not "whichever was pressed last",
// which needs an order this file does not keep.
//
// ---- FLYING: W A S D, Q E, SPACE, CTRL, SHIFT, AND THE MOUSE ----
//
// Tab, then: W and S forwards and back along where the camera is looking, A and
// D left and right, E or Space up and Q or Ctrl down — straight up and down
// whatever the camera is looking at — Shift to go four times as fast, and the
// mouse to look around. Tab again or Escape to give it back, and Escape once
// more to close the window. E and Q are there so that the whole of flying is
// reachable from the left hand alone.
//
// WHAT IS WRONG IF IT FEELS WRONG, AND EACH OF THESE IS A DIFFERENT MISTAKE:
//
//   - The view jumps the moment Tab is pressed — the handover. The orbit stops
//     submitting placements and the keyboard starts submitting motions, and
//     because a placement applies before a motion in the same run, the camera
//     simply continues from where the orbit left it. A jump means one of those
//     two is submitting when it should not be.
//   - The mouse turns the wrong way, or up looks down — a sign in
//     scene/src/camera_system.c.
//   - Looking straight up or straight down and everything vanishes — the pitch
//     clamp. Try to look further up than you can; it should simply stop.
//   - Strafing while looking at the floor sinks into it — right is being taken
//     from the camera's own frame rather than kept horizontal.
//   - A diagonal is faster than a straight line — the movement direction is not
//     being normalized. Hold W, then hold W and D, and the speed should not
//     change.
//   - The camera keeps flying with nobody touching anything — a held key that
//     was never released. This is the one to look for after alt-tabbing away
//     with W down.
//
// What there is to try:
//
//   - Alt-tab away while holding W, and come back. It must not still be flying
//     when focus is gone, and it must not need a fresh press of W to notice it
//     is still held on the way back.
//   - Look around, a lot, in one direction. It should not slow down, drift or
//     stick — mouse look with no pointer lock walks the cursor out of the window
//     and stops, which is what the `locked` line is for.
//   - Watch the `locked` line. Asking to fly asks for the pointer; a compositor
//     may say no, and then mouse look works only while the cursor happens to be
//     over the window. That is not a failure and nothing here treats it as one.
static voe_scene_camera_motion camera_motion(voe_platform_window *window,
					     voe_ecs_entity eye, float seconds)
{
	voe_scene_camera_motion motion = {
		.entity = eye,
		.seconds = seconds,
	};
	voe_platform_motion mouse;

	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_W))
		motion.forward += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_S))
		motion.forward -= 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_D))
		motion.right += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_A))
		motion.right -= 1.0f;
	// Space and E are the same instruction, and so are Ctrl and Q, which is
	// why each pair is one test and not two. Two tests adding a step each
	// would make Space and E held together a rise of two: normalizing later
	// fixes the speed but not the direction.
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_E))
		motion.up += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_CONTROL) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_Q))
		motion.up -= 1.0f;

	motion.fast = voe_platform_input_key_down(window,
						  VOE_PLATFORM_KEY_SHIFT);

	mouse = voe_platform_input_motion(window);
	motion.look_x = mouse.x;
	motion.look_y = mouse.y;

	return motion;
}

// Where the orbit is at this many seconds in, as a placement: an eye and the two
// angles that look at the origin from it.
//
// THE CAMERA PATH. The orbit is a function of the loop's clock and it submits a
// camera placement every frame. It is a demonstration and not a feature: what a
// camera does about being moved is `scene`'s, and where a camera should be is
// whatever is driving it.
//
// This is the thing to look at for the rendering, and the reason there are
// several objects rather than one:
//
//   - The camera orbits, once every twelve seconds or so. What says so is
//     parallax: the objects pass in front of and behind one another, which is
//     the only thing a moving camera can do and a rotating object cannot.
//   - One cube stands still, in the middle of the frame, and stays there. Its
//     faces turn because the camera goes round it. It never drifts, never
//     changes size and never leaves the centre — the camera looks straight at
//     it, from wherever it is.
//   - The other cube spins, three times as fast as the camera orbits, about a
//     tilted axis so that it cannot be mistaken for a second orbit.
//   - The model does neither: it sits where its file and one transform intent
//     put it.
//
// THE ANGLES ARE WORKED OUT HERE AND NOT LEFT TO THE CAMERA, because a placement
// is an absolute answer and the camera's job is to hold it, not to guess what it
// was aimed at. It is also what makes the handover to flying seamless: the last
// placement the orbit submitted is exactly where the hand takes over from.
static voe_scene_camera_placement orbit(voe_ecs_entity eye, float seconds)
{
	float angle = seconds * TURN / ORBIT_SECONDS;
	voe_math_float3 position = { sinf(angle) * ORBIT_RADIUS, ORBIT_HEIGHT,
				     cosf(angle) * ORBIT_RADIUS };
	voe_math_float3 towards = voe_math_float3_normalize(
		voe_math_float3_neg(position));
	voe_scene_camera_placement placement = {
		.entity = eye,
		.eye = position,
		.pitch = asinf(towards.y),
		// Zero yaw looks along -Z and a positive yaw turns towards -X,
		// which is what this pair of arguments says.
		.yaw = atan2f(-towards.x, -towards.z),
	};

	return placement;
}

// Where the sun is pointing at this many seconds in, as an intent.
//
// THE SUN'S PATH. The light circles the scene on the same clock, as a light
// intent every frame, for one reason: a still light is a light nobody can tell
// from a wrong one. A scene will say where its sun is the day there is a scene
// file.
//
// IT IS LIT BY ONE SUN AND THE SUN GOES ROUND. One directional light, circling
// the scene once every SUN_SECONDS, so the bright side of everything moves and
// the far side of everything is black — there is no ambient light and no bounce,
// so an unlit face really is nothing. The figure's ORM map is read now:
// occlusion, roughness and metalness out of one picture, which is why it does
// not look like plastic in the way the cubes do.
//
// NOTHING IS TONE MAPPED, SO THE BRIGHT SIDE CAN CLIP. A highlight that goes
// flat white is expected and is on the later list, not a mistake in the shading.
//
// IT CIRCLES ON THE HORIZONTAL PLANE AND LEANS DOWNWARDS. The x and z components
// go round with the clock and the y component is fixed, so the light comes from
// a different side of the scene every few seconds and always from above. The
// direction is not normalized here: the light system does that, which is the
// point of it doing it there — see scene/light_system.h.
//
// THE LAP IS NOT THE CAMERA'S LAP. SUN_SECONDS and ORBIT_SECONDS are different
// numbers on purpose: if the sun went round with the camera, every surface would
// keep the same brightness and the whole thing would look like shading stuck to
// the screen — which is one of the failures this program exists to show.
//
// What is wrong if it looks wrong:
//
//   - EVERYTHING BLACK — the sun is pointing away from everything, its intensity
//     is nought, or the light never reached the shader. The background is
//     cleared and not lit, so a black scene on a coloured background is a
//     lighting failure and a black window is not.
//   - The bright side of the still cube not moving as the sun goes round — the
//     light intent is not landing, or the light system is not being run.
static voe_scene_light_intent sunlight(voe_ecs_entity sun, float seconds)
{
	float angle = seconds * TURN / SUN_SECONDS;
	voe_scene_light_intent intent = {
		.entity = sun,
		.light = {
			.direction = { sinf(angle), SUN_HEIGHT, cosf(angle) },
			.colour = { 1.0f, 1.0f, 1.0f },
			.intensity = SUN_INTENSITY,
		},
	};

	return intent;
}

// The four numbers the loop measures, and the clock reading that says when a
// period ends. One struct because they are gathered together, reported together
// and reset together, and four loose pairs at the top of main() would be twelve
// variables to keep in step.
//
// AND IT PRINTS WHAT IT MEASURED. Four numbers every couple of seconds, each an
// average and a worst over exactly that period: the frame, this program's own
// work, the draw, and the graphics card's own clock. say_what_is_measured() is
// the legend and it is printed once at startup, because a number whose meaning
// is ambiguous is worse than no number. P switches between the two present
// modes, which is the measurement the frame-pacing decision is waiting on.
//
// EACH ONE BRACKETS EXACTLY ONE THING AND THE NAMES BELOW ARE THE WHOLE POINT. A
// single "frame time" would hide which of the four is the one that got longer,
// and that is the question a person is asking when they look at this at all.
struct timing {
	// One top of the loop to the next. Everything is inside it and its
	// average is what "frames per second" is the reciprocal of.
	voe_base_samples frame;
	// The poll, the input, and the three systems. This program's own work
	// before it asks the GPU for anything.
	voe_base_samples update;
	// The loop's draw phase, _begin to _end inclusive: waiting for the frame
	// slot, taking a swapchain image, laying out and uploading the readout,
	// recording every draw, submitting and presenting.
	voe_base_samples draw;
	// The graphics card's own clock, over that frame's commands only.
	voe_base_samples gpu;
	// The last full period's averages of the two numbers measured AFTER the
	// readout is built, and — for the card — whether it has ever answered.
	//
	// THEY EXIST BECAUSE THE PERIOD IS RESET WITH THE READOUT STILL
	// RUNNING, AND THEY ARE FOR THE READOUT AND NOT THE CONSOLE. `draw` and
	// `gpu` are sampled below build_the_readout in the loop, so for the one
	// frame after every console block their sample sets are empty while the
	// readout is being built — and voe_base_samples_average of nothing is
	// nought. Showing that nought is a draw phase that appears to have taken
	// no time at all, once every period, which is a number a person would
	// believe. `frame` and `update` need none of this: they are sampled
	// ABOVE build_the_readout, so they always hold at least this frame.
	bool gpu_timed;
	double gpu_last;
	double draw_last;
	// Draw commands per completed frame, averaged and worsted over the
	// period exactly as the four durations above are.
	//
	// IT IS A METRIC AND NOT A SNAPSHOT, WHICH IS THE PRINCIPAL'S CALL AND
	// REVERSES WHAT CARD 040 FIRST SAID. That card argued a draw count is
	// exact and that averaging it turns a true number into a smeared one.
	// That is right about this demo, where the scene is identical every
	// frame and the average is 32.0 for ever — and it is wrong about the
	// direction of travel. The moment anything varies what is drawn, culling
	// or streaming or an interface that grows, the number a person needs is
	// the WORST frame in the period rather than the typical one, which is
	// the same reason `frame` and `gpu` carry a worst beside their average.
	// A metric that only becomes interesting later is still the right shape
	// now; a snapshot that has to be replaced later is not.
	//
	// IT IS A voe_base_samples EVEN THOUGH A COUNT IS NOT A DURATION. That
	// type's real constraint is that `worst` is the LARGEST value, so a
	// quantity where small is bad does not belong in it. More draw commands
	// is worse, so this belongs. See base/include/base/samples.h, whose
	// header still says everything measured with one is a duration — true of
	// every other caller and no longer of this one.
	voe_base_samples draws;
	// What the readout shows for the one frame per period that has no sample
	// yet, and whether any frame has completed at all. The same three cases
	// `draw` and `gpu` have above and for the same reason: the period is
	// reset with the readout still running.
	bool draws_measured;
	double draws_last;
	double draws_last_worst;
	// And how many element records the last completed frame submitted, which
	// is the other half of ADR-0092's claim and the reason the pair is worth
	// showing.
	//
	// THIS ONE IS NOT AVERAGED AND THAT IS NOT AN OVERSIGHT. The claim is
	// "this many records cost that few commands", and a record count averaged
	// over a period no longer lines up with the commands beside it — one
	// would describe the period and the other a frame in it. The exact count
	// from the same completed frame keeps the pair a pair.
	//
	// THE GAP BETWEEN THE TWO IS THE CLAIM, AND NEITHER NUMBER SAYS IT
	// ALONE. Ninety-odd records and thirty-odd commands is "many small
	// things in one draw" stated rather than asserted; the commands alone
	// could be thirty-two of anything, and the records alone say nothing
	// about what they cost. See voe_render_frame_elements_submitted, whose
	// own header is where that gap is spelled out.
	uint32_t drawn_elements;
	// When this period began, on the same clock every sample is taken with.
	// Kept rather than a deadline, because the rate printed has to be over
	// the time the period really covered and a frame always straddles the
	// end of one.
	double started;
};

// The legend, once, at startup. It is here and not repeated in every block
// because the four names never change and the block is meant to be glanced at,
// and it is printed at all because a number whose meaning is ambiguous is worse
// than no number.
static void say_what_is_measured(void)
{
	printf("timing     four numbers every %.0f s, each averaged over that period with its worst\n",
	       REPORT_SECONDS);
	printf("           frame   one top of the loop to the next. The rate is its reciprocal\n");
	printf("           update  the poll, the input and the three systems\n");
	printf("           draw    the loop's draw phase, begin to end: the wait for the frame\n");
	printf("                   slot, the acquire, the readout's layout and upload, the\n");
	printf("                   recording, the submit and the present. On fifo the wait\n");
	printf("                   for the display is in here and is most of it\n");
	printf("           gpu     the graphics card's own clock, over that frame's commands\n");
	printf("                   only — the wait for the display is not in it. It runs two\n");
	printf("                   frames behind, and is absent on a card that cannot time\n");
	printf("           P switches between fifo and mailbox. Numbers from both are what\n");
	printf("           the frame-pacing decision wants; the mode is on every block below\n");
	printf("           The same five numbers stand top-left of the view as the period's\n");
	printf("           running averages, laid out again every frame\n");
}

// The readout's text this frame, out of what the period has measured so far,
// laid out through the transient path and put on the readout entity. Called
// between _begin and the draw system, which is the only place it can be called.
//
// AND SINCE CARD 028 IT DRAWS THEM TOO. The same numbers stand in the top-left
// of the view as the period's running averages, rebuilt every frame as a text
// block through voe_text_block_create_transient and never cached: a readout that
// remembered its string until it changed would, the day its invalidation missed,
// show yesterday's numbers and look exactly like a frozen program. The console
// block stays, because a period's worst is a different, still-useful thing.
//
// And in the top-left of the view, lines of numbers that change as you watch:
// the frame rate, the four timings — the same ones the console prints — the
// pointer, and what the last frame cost in draw commands and element records.
// Laid out again every frame.
//
// THE DRAWS LINE IS A PERIOD METRIC LIKE THE FOUR ABOVE IT, average and worst
// over the same window, and the element records beside it are the last
// completed frame's. It is built before anything is drawn, so the one frame
// after each console block has no sample yet and shows the previous period's;
// a program that has completed no frame at all says "no frame yet" rather than
// showing a nought that reads as a claim.
//
// AND THE PAIR ON IT IS ADR-0092'S CLAIM RATHER THAN A NUMBER STANDING FOR IT.
// Ninety-odd element records against thirty-odd draw commands is what "many
// small things in one draw" means; watch the records climb while the commands
// hold still. The count includes the draw that put this readout on screen,
// which is why counting the things you can see gives one fewer.
//
// AND IT COUNTS BOTH PASSES, WHICH IS WHY IT IS ABOUT TWICE WHAT IS ON SCREEN.
// The monitor walks the same world into its own target before the window's pass
// runs, so nearly everything visible is drawn twice a frame. The console line
// printed once at startup says how many of the frame's commands the window's own
// walk was, which is the half that matches what a person can count.
//
// THE WORST COLUMN IS THE POINT OF MAKING IT A METRIC. In this demo the scene
// is identical every frame, so the average and the worst are both the same
// number and the column looks like decoration. The day anything varies what is
// drawn — culling, streaming, an interface that grows — the worst frame in the
// period is the number that matters and the average is the one that hides it.
//
// THE NUMBERS ARE THE RUNNING AVERAGES OF THE CURRENT PERIOD. The console block
// prints a period's average and worst once the period is over; this prints the
// average so far, every frame, so it settles over the first few hundred
// milliseconds of a period and starts again with the next block. The rate is the
// reciprocal of the frame average rather than a count over elapsed time, because
// the frame sample always holds at least this frame. For the one frame after
// each console block the other three read nought — the period has no sample of
// them yet — and printing last period's number instead would be the cache this
// card forbids in a smaller coat.
//
// THERE IS NO CACHE AND NO "HAS IT CHANGED". The string is formatted and laid
// out every frame whether or not a digit moved. It is a few dozen glyphs and
// well under a tenth of a millisecond, and it is the design that was decided; a
// readout that stops changing is therefore a bug in the transient path and never
// a cache being clever.
//
// `glyphs` comes back as how many characters were laid out, so that what the
// readout actually consumed can be printed once beside what was asked for.
//
// What is wrong if it looks wrong:
//
//   - THE READOUT'S NUMBERS FROZEN WHILE THE CONSOLE BLOCKS KEEP COMING — the
//     transient path. Either the block is not being rebuilt each frame, or a
//     stale id is being drawn and refused on stderr every frame. There is no
//     cache anywhere that could make them merely slow to update, so frozen is
//     always a bug and never a saving.
//   - The readout's left edge jumping sideways as a number gains a digit — it
//     is being centred on its width. It is meant to be left-aligned, which is
//     why its placement asks nothing about its size.
//
// What there is to try:
//
//   - READ THE SAME NUMBERS OFF THE SCREEN. The readout shows the period's
//     running averages, so it settles over the first few hundred milliseconds
//     after each console block and should then agree with the block that
//     follows, to within the averaging. Watch a few periods go by: it resets
//     with every block, and the frame rate moves the moment the window is
//     resized or minimised and restored. The `gpu` line must not flash `no
//     measurement` at the reset — it holds the last period's average for the
//     one frame the new period has no sample yet. If it does flash, the
//     three-case fallback in build_the_readout has been collapsed to two.
//   - WATCH THE `mouse` LINE OF THE READOUT (card 029). It is the pointer's
//     position in the window's own pixels and the three buttons, live. Move
//     the pointer to each corner: top-left should read close to 0 0 and
//     bottom-right one less than the `size` line in both numbers, with neither
//     overshooting nor inverting — a swapped or mirrored axis is a backend
//     mistake. Press the buttons: the dots become L, M and R. Hold one and drag
//     out of the window: the numbers go negative or past the size and keep
//     following, which is the drag both window systems promise. Move the
//     pointer off the window with nothing held and the line says `away`.
//     Press Tab to fly and it says `locked`: the camera has the pointer and
//     there is nothing at its position to point at, as include/platform/input.h
//     says. Tab back and move, and the numbers are live again.
static bool build_the_readout(voe_ecs_world *world, voe_render_device *gpu,
			      const voe_text_font *font, voe_base_arena *arena,
			      voe_ecs_entity readout, voe_platform_window *window,
			      const struct timing *timing, uint32_t *glyphs,
			      voe_base_error *error)
{
	char gpu_line[READOUT_CHARS];
	char draws_line[READOUT_CHARS];
	char mouse_line[READOUT_CHARS];
	char text[READOUT_CHARS];
	voe_text_block block;
	double frame = voe_base_samples_average(&timing->frame);
	// The same two cases gpu has below, and for the same reason: the period
	// was reset with this readout still running, so for one frame per period
	// there is no sample of the draw phase yet and last period's average is
	// what holds the line still. Before the first console block there is no
	// last one either, and nought is then the truth rather than a stale
	// number.
	double draw = timing->draw.count > 0 ?
			      voe_base_samples_average(&timing->draw) :
			      timing->draw_last;
	uint32_t drawn = 0;
	voe_platform_pointer pointer = voe_platform_input_pointer(window);
	// The buttons as three letters in the order they sit on a mouse, a dot
	// for one that is up. Read every frame like everything else here: this
	// is a call site showing what platform hands out, not a GUI.
	char left = voe_platform_input_button_down(window,
						   VOE_PLATFORM_BUTTON_LEFT) ?
			    'L' :
			    '.';
	char middle = voe_platform_input_button_down(
			      window, VOE_PLATFORM_BUTTON_MIDDLE) ?
			      'M' :
			      '.';
	char right = voe_platform_input_button_down(
			     window, VOE_PLATFORM_BUTTON_RIGHT) ?
			     'R' :
			     '.';

	// THREE CASES AND NOT TWO, BECAUSE THE PERIOD IS RESET WITH THE READOUT
	// STILL RUNNING. A period with samples shows its running average. A
	// period with none yet — the one frame after each console block, since
	// the timestamp is asked for after the draw and this is built before it
	// — shows the last full period's average, so the line holds still
	// rather than flashing the fallback below for a single frame that
	// mailbox sometimes presents. Only a card that has never answered gets
	// the fallback, which is then the truth.
	if (timing->gpu.count > 0)
		snprintf(gpu_line, sizeof gpu_line, "gpu    %6.2f ms",
			 voe_base_samples_average(&timing->gpu) * 1000.0);
	else if (timing->gpu_timed)
		snprintf(gpu_line, sizeof gpu_line, "gpu    %6.2f ms",
			 timing->gpu_last * 1000.0);
	else
		snprintf(gpu_line, sizeof gpu_line, "gpu    no measurement");

	// THREE CASES, THE SAME THREE THE GPU LINE HAS AND FOR THE SAME REASON.
	// A period with samples shows its running average and worst. The one
	// frame after each console block has neither yet — the count is added
	// after the draw and this is built before it — and shows the last full
	// period's, so the line holds still rather than flashing. Only a program
	// that has not completed a frame at all gets the third case, which is
	// then the truth.
	//
	// AND THE COUNT INCLUDES THE DRAW THAT PUT THIS READOUT ON SCREEN. The
	// readout is a mesh like any other and is drawn like any other, so a
	// person counting the things they can see will find one fewer than this
	// says. That is correct and it is not adjusted for: subtracting it would
	// make this a number about something other than what the frame did.
	//
	// THE RECORDS BESIDE IT ARE ONE FRAME'S AND THE COMMANDS ARE A PERIOD'S,
	// which is a mixture on one line and is deliberate — see struct timing.
	// The pair only means something if both halves describe the same drawing.
	if (timing->draws.count > 0)
		snprintf(draws_line, sizeof draws_line,
			 "draws  %5.1f avg %4.0f worst  %u elements",
			 voe_base_samples_average(&timing->draws),
			 timing->draws.worst,
			 (unsigned)timing->drawn_elements);
	else if (timing->draws_measured)
		snprintf(draws_line, sizeof draws_line,
			 "draws  %5.1f avg %4.0f worst  %u elements",
			 timing->draws_last, timing->draws_last_worst,
			 (unsigned)timing->drawn_elements);
	else
		snprintf(draws_line, sizeof draws_line, "draws  no frame yet");

	// Three states and three words, because a number that looked live
	// while the pointer was locked or gone is exactly what the platform
	// header refuses to hand out. The position is printed whole: Wayland
	// reports fractions and they are not interesting to a person.
	if (pointer.over)
		snprintf(mouse_line, sizeof mouse_line,
			 "mouse  %5.0f %5.0f %c%c%c", pointer.x, pointer.y,
			 left, middle, right);
	else if (voe_platform_input_pointer_locked(window))
		snprintf(mouse_line, sizeof mouse_line, "mouse  locked      %c%c%c",
			 left, middle, right);
	else
		snprintf(mouse_line, sizeof mouse_line, "mouse  away        %c%c%c",
			 left, middle, right);

	snprintf(text, sizeof text,
		 "%5.0f fps\nframe  %6.2f ms\nupdate %6.2f ms\ndraw   %6.2f ms\n%s\n%s\n%s",
		 frame > 0.0 ? 1.0 / frame : 0.0, frame * 1000.0,
		 voe_base_samples_average(&timing->update) * 1000.0,
		 draw * 1000.0, gpu_line, draws_line, mouse_line);

	// Spaces and newlines lay out nothing; every other character in this
	// string is a glyph the font carries.
	for (const char *at = text; *at != '\0'; at++)
		if (*at != ' ' && *at != '\n')
			drawn++;
	*glyphs = drawn;

	if (!voe_text_block_create_transient(font, gpu, arena, text, READOUT_EM,
					     &block, error))
		return false;

	// The entity is this program's and is never destroyed, so a mesh that
	// is not there is a bug here and not a thing to handle.
	VOE_BASE_ASSERT(voe_3d_mesh_set_geometry(world, readout, block.geometry),
			"the readout entity has lost its mesh");
	return true;
}

// One block, and then the period starts again. `seconds` is how long the period
// really lasted rather than REPORT_SECONDS, because a frame straddles the end of
// one and the rate has to be over the time actually covered.
//
// THE GPU LINE IS ABSENT RATHER THAN NOUGHT WHEN THERE IS NO MEASUREMENT. A card
// that cannot write timestamps would otherwise report a graphics card that takes
// no time at all, which is the most misleading thing this could print.
//
// What there is to try:
//
//   - WATCH THE TIMING BLOCKS, AND WATCH THEM MOVE. On fifo, `frame` should sit
//     within a few tenths of a millisecond of the display's refresh interval —
//     16.7 ms at sixty hertz — and the reciprocal printed beside it should be
//     the refresh rate. `draw` should be nearly all of it and `update` almost
//     none: the program is waiting for the display, which is what fifo means.
//     Then make something happen: drag the window bigger and `gpu` should go up
//     with the pixel count, minimise it and the rate should go through the roof.
//     A number that never moves is a number that is not being measured.
//   - Hold the window still and read the `worst` column. It should be close to
//     the average. A worst several times the average is stutter, and stutter is
//     the thing a person actually notices — which is why it is printed at all.
static void report(struct timing *timing, double seconds,
		   voe_render_present present)
{
	printf("timing     %llu frames in %.2f s — %.1f per second, %s\n",
	       (unsigned long long)timing->frame.count, seconds,
	       seconds > 0.0 ? (double)timing->frame.count / seconds : 0.0,
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	printf("           frame  %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->frame) * 1000.0,
	       timing->frame.worst * 1000.0);
	printf("           update %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->update) * 1000.0,
	       timing->update.worst * 1000.0);
	printf("           draw   %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->draw) * 1000.0,
	       timing->draw.worst * 1000.0);
	if (timing->gpu.count > 0)
		printf("           gpu    %7.2f ms avg  %7.2f ms worst\n",
		       voe_base_samples_average(&timing->gpu) * 1000.0,
		       timing->gpu.worst * 1000.0);
	else
		printf("           gpu        no measurement — this card or its queue cannot write timestamps\n");
	// The one line here that is not milliseconds, in the same two columns as
	// the four that are. What varies it is the scene rather than the
	// machine, so in this demo the average and the worst are the same number
	// — and the day they differ is the day something started drawing more in
	// some frames than others, which is exactly what a worst column is for.
	printf("           draws  %7.1f    avg  %7.0f    worst\n",
	       voe_base_samples_average(&timing->draws), timing->draws.worst);
	fflush(stdout);

	// What the readout shows until this new period has a sample of its own.
	// Only the two measured after it is built need it; see struct timing.
	if (timing->draw.count > 0)
		timing->draw_last = voe_base_samples_average(&timing->draw);
	if (timing->gpu.count > 0)
		timing->gpu_last = voe_base_samples_average(&timing->gpu);
	// Both halves of this one, because the readout shows both.
	if (timing->draws.count > 0) {
		timing->draws_last = voe_base_samples_average(&timing->draws);
		timing->draws_last_worst = timing->draws.worst;
	}

	voe_base_samples_reset(&timing->frame);
	voe_base_samples_reset(&timing->update);
	voe_base_samples_reset(&timing->draw);
	voe_base_samples_reset(&timing->gpu);
	voe_base_samples_reset(&timing->draws);
}

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
//     — and the timing blocks keep coming, at thousands of frames a second,
//     which is the loop with nothing in it to wait for.
//   - PRESS P AND COMPARE. It asks for fifo, and the `present` word on every
//     block says which is actually in force — a machine that has no mailbox was
//     already saying fifo and will go on saying it, and that is an answer.
//     What to look for is `frame` dropping onto the display's refresh interval
//     and the rate pinned to the refresh rate, in exchange for not drawing
//     frames nobody ever sees. Press it again to come back.
//   - Close it. It should print `closed` and exit zero.
//
// IT WILL SPIN A CORE WHILE IT IS OPEN, AND ON MAILBOX IT WILL SPIN THE GRAPHICS
// CARD TOO. Mailbox is what this program asks for as soon as its device is open —
// the engine itself opens on fifo — so it runs as fast as the card and the
// program between them allow, drawing many frames for every one anybody sees. P
// is how to stop it doing that: fifo waits
// for the display, so a visible window then costs one frame's worth of work per
// refresh. Either way a minimised one presents nothing and _poll returns
// immediately, because platform has no way to wait yet.
int main(void)
{
	// The window and the device, opened together by `app` and reached
	// through it. Both are taken into locals once, below, because this file
	// names them on nearly every line and voe_app_window(app) on each of
	// them would say nothing the name does not.
	voe_app *app;
	voe_platform_window *window;
	voe_base_arena *scratch;
	voe_base_arena *arena;
	voe_ecs_world *world;
	voe_render_device *gpu;
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size;
	voe_ecs_entity eye = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_ecs_entity turning = { 0 };
	voe_ecs_entity hud = { 0 };
	voe_ecs_entity panel = { 0 };
	voe_ecs_entity readout = { 0 };
	// The two element surfaces that are entities: the exhibit standing in
	// the world and the badge in the overlay. Their ranges are rewritten
	// every frame — see the loop.
	voe_ecs_entity exhibit_panel = { 0 };
	voe_ecs_entity badge_panel = { 0 };
	// How many glyphs the readout laid out, printed once against the room
	// made for it — see MAX_TRANSIENT_GLYPHS.
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
	voe_render_geometry quad = { 0 };
	voe_dev_sprites sprites = { 0 };
	// The second camera's picture and the screen in the world that shows it.
	// Its target, its camera and its screen entity are all made once, below;
	// the loop asks it only what its pass is drawn with.
	voe_dev_monitor monitor = { 0 };
	voe_text_font *font = NULL;
	voe_ui_context *interface = NULL;
	uint32_t interface_elements = 0;
	voe_math_float2 hud_size = { 0.0f, 0.0f };
	voe_math_float3 spin_axis = { SPIN_AXIS_X, SPIN_AXIS_Y, SPIN_AXIS_Z };
	voe_render_capacities capacities = {
		.vertices = MAX_VERTICES,
		.indices = MAX_INDICES,
		.geometries = MAX_MESHES,
		.objects = MAX_DRAWN_OBJECTS,
		.shadings = MAX_SHADINGS,
		.transient_vertices = 4 * MAX_TRANSIENT_GLYPHS,
		.transient_indices = 6 * MAX_TRANSIENT_GLYPHS,
		.transient_geometries = MAX_TRANSIENT_GEOMETRIES,
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
	bool decorated;
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
	// Last frame's P, for the same reason, and the mode it asks for. It
	// starts true because this program asks for mailbox as soon as the device
	// is open, with this variable, so the first press of P asks for fifo
	// rather than for what is already happening.
	//
	// What is actually in force is the device's answer and is asked for
	// rather than remembered: a surface with no mailbox leaves this true and
	// the device on fifo, and printing what was asked for would be a lie.
	bool p_was_down = false;
	bool mailbox_wanted = true;
	voe_render_present present;
	// The scene's clock: measured now, and the sum of every step taken, not
	// of every second that passed. See MAX_FRAME_SECONDS and the skip below.
	float seconds = 0.0f;
	// The real clock, and what it is read into. `top` is this frame's
	// reading, taken off the tick `app` hands back rather than read again
	// here, so the interval the readout reports and the step the scene takes
	// are the same subtraction — see voe_app_frame_open.
	struct timing timing = { 0 };
	double top;
	double after_update;
	double after_draw;
	double step;

	// The world and everything read into it live here, and it is destroyed
	// at the end: the world is the arena's, which is what rule 11 asks for.
	// It is made before the window now, because the app struct lives in it
	// too and has to outlive every call made through it.
	arena = voe_base_arena_new(WORLD_ARENA);

	// The window and the device, in one call and in that order. Nothing is
	// kept out of `scratch`, so it goes as soon as this returns.
	//
	// NOTHING IS PRINTED HERE ON A FAILURE AND THAT IS NOT AN OVERSIGHT.
	// `app` says which of the two refused and `render` says why, both on
	// stderr, before this returns NULL; a third line from this file would
	// only repeat them.
	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	app = voe_app_new(arena, scratch,
			  (voe_app_settings){
				  .width = 960,
				  .height = 540,
				  .title = "voe3d — a model, two cubes, one camera",
				  .capacities = capacities,
				  .longest_step = MAX_FRAME_SECONDS },
			  &error);
	voe_base_arena_destroy(scratch);
	if (app == NULL) {
		voe_base_arena_destroy(arena);
		return 1;
	}

	window = voe_app_window(app);
	gpu = voe_app_device(app);

	// Mailbox, asked for rather than inherited: a device opens on fifo, and
	// `app` asks for no mode at all, so this is the program's own request. It
	// is the call P makes and the variable P flips, so what was asked for at
	// startup and what the first press takes back cannot disagree.
	voe_render_present_set(gpu, mailbox_wanted ? VOE_RENDER_PRESENT_MAILBOX :
						     VOE_RENDER_PRESENT_FIFO);

	world = voe_ecs_world_new(arena, limits);

	// Registration, once, and this is the whole of what a call site has to
	// know about which components exist. Each folder says what one of its
	// components is; nothing here does.
	voe_scene_transform_register(world, MAX_ENTITIES);
	voe_scene_camera_register(world, 4);
	voe_scene_light_register(world, 4);
	voe_3d_mesh_register(world, MAX_ENTITIES);
	voe_3d_material_register(world, MAX_ENTITIES);
	// The second kind of drawable. Two of them in this program, and the
	// table is registered like any other component — which is the whole of
	// what a panel costs a call site.
	voe_3d_panel_register(world, MAX_ENTITIES);

	if (!voe_ecs_entity_create(world, &eye) ||
	    !voe_scene_camera_add(world, eye, camera)) {
		VOE_BASE_ERROR("dev", "could not make a camera");
		goto stop;
	}

	// The sun, at wherever its lap starts. The draw system needs exactly one
	// light in the world, so this is not optional wiring — a world without it
	// asserts rather than drawing something black.
	if (!voe_ecs_entity_create(world, &sun) ||
	    !voe_scene_light_add(world, sun, sunlight(sun, 0.0f).light)) {
		VOE_BASE_ERROR("dev", "could not make a sun");
		goto stop;
	}

	if (!voe_dev_add_the_cubes(world, gpu, arena, &turning, &error)) {
		VOE_BASE_ERROR("dev", "could not build the two cubes: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	if (!voe_dev_add_the_quads(world, gpu, &quad, &error)) {
		VOE_BASE_ERROR("dev", "could not build the two see-through quads: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	if (!voe_dev_sprites_add(world, gpu, arena, &sprites, &error)) {
		VOE_BASE_ERROR("dev", "could not build the sprites: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	// The font and the three text entities. Not optional the way a model is:
	// there is one font, it is in the binary, and a failure here is a bug in
	// the reader rather than a file somebody could not open.
	font = voe_dev_add_the_text(world, gpu, arena, quad, &hud, &panel,
				    &readout, &hud_size, &error);
	if (font == NULL) {
		VOE_BASE_ERROR("dev", "could not build the text: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	// The interface, which needs the font and so cannot be made before it.
	// It is the context and nothing else — what is on the interface is
	// built afresh every frame inside the loop.
	interface = voe_dev_interface_new(arena, font);

	// The two panels. Nothing is on them yet: what a panel holds is a range
	// of the frame that is open, and no frame is open until the loop starts.
	if (!voe_dev_add_panel(
		    world, (voe_math_float3){ EXHIBIT_X, EXHIBIT_Y, EXHIBIT_Z },
		    EXHIBIT_SCALE,
		    (voe_math_float2){ VOE_DEV_ELEMENTS_PANEL_WIDE,
				       VOE_DEV_ELEMENTS_PANEL_HIGH },
		    VOE_3D_LAYER_WORLD, &exhibit_panel) ||
	    !voe_dev_add_panel(
		    world, (voe_math_float3){ BADGE_X, BADGE_Y, BADGE_Z },
		    BADGE_SCALE,
		    (voe_math_float2){ VOE_DEV_BADGE_WIDE, VOE_DEV_BADGE_HIGH },
		    VOE_3D_LAYER_OVERLAY, &badge_panel)) {
		VOE_BASE_ERROR("dev", "could not build the two element panels");
		goto stop;
	}

	// The model is the one thing here that is allowed to fail without
	// stopping the program: the cubes are what says the renderer works, and
	// a person looking at a window is better served by seeing them and a
	// message than by seeing nothing.
	if (!voe_dev_add_a_model(world, gpu, arena, "human",
				 voe_dev_human_glb, voe_dev_human_glb_size,
				 HUMAN_X, &error))
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
	if (!voe_dev_monitor_create(&monitor, world, gpu, &error)) {
		VOE_BASE_ERROR("dev", "could not build the monitor: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	size = voe_platform_window_size(window);
	decorated = voe_platform_window_decorated(window);
	printf("opened     %dx%d\n", size.width, size.height);
	printf("decorated  %s\n", decorated ? "yes" : "no");
	printf("camera     orbit — Tab to fly, Escape to hand back or close\n");
	// Fifo here, whatever was asked for above: the swapchain the device
	// opened with is already built on fifo, and the request is acted on at
	// the top of the first frame. Every block from then on says what is in
	// force.
	present = voe_render_present_get(gpu);
	printf("present    %s\n",
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	say_what_is_measured();
	fflush(stdout);

	// When the first reporting period started. The frame's own interval is
	// the clock inside `app` and this file no longer keeps a previous
	// reading of its own.
	timing.started = voe_platform_clock_now();

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
		opened = voe_app_frame_open(app);
		if (opened.closing)
			break;

		// The reading that opened the frame, kept because the update
		// and the draw are measured from it. Everything below is inside
		// the interval it ends.
		top = opened.tick.now;

		// WHAT IS REPORTED AND WHAT THE SCENE TAKES ARE THE TWO NUMBERS
		// ON THE TICK, AND THEY ARE NOT THE SAME ONE. `elapsed` is what
		// really happened and is what the readout says; `step` is that
		// with MAX_FRAME_SECONDS on it and is all the orbit, the spin
		// and the sun are advanced by. The first tick has no interval
		// behind it and says so, and recording it would make the
		// readout's first line claim four million frames a second.
		if (!opened.tick.first)
			voe_base_samples_add(&timing.frame, opened.tick.elapsed);
		step = opened.tick.step;

		// The frame opened with a poll in it, so what follows is this
		// program reading state that is already this frame's. Everything
		// is asked every frame because platform hands out state, not
		// events.
		now_size = opened.size;
		if (now_size.width != size.width ||
		    now_size.height != size.height) {
			size = now_size;
			printf("size       %dx%d\n", size.width, size.height);
			fflush(stdout);
		}

		now_decorated = voe_platform_window_decorated(window);
		if (now_decorated != decorated) {
			decorated = now_decorated;
			printf("decorated  %s\n", decorated ? "yes" : "no");
			fflush(stdout);
		}

		// Tab toggles on the press and not while held, which is what
		// turning state back into an edge means. Escape and P below do
		// the same and for the same reason: `platform` hands out which
		// keys are down, and all three of these are actions rather than
		// things held.
		tab_down = voe_platform_input_key_down(window,
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
			window, VOE_PLATFORM_KEY_ESCAPE);
		if (escape_down && !escape_was_down) {
			if (flying)
				flying = false;
			else
				break;
		}
		escape_was_down = escape_down;

		// P asks for the other present mode, on the press and not while
		// held, exactly as Tab does. What the device does about it is
		// asked for below rather than assumed: a surface with no mailbox
		// stays on fifo however often this is pressed, and that is a
		// measurement of the machine rather than a failure.
		p_down = voe_platform_input_key_down(window, VOE_PLATFORM_KEY_P);
		if (p_down && !p_was_down) {
			mailbox_wanted = !mailbox_wanted;
			voe_render_present_set(gpu,
					       mailbox_wanted ?
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
		voe_platform_input_lock_pointer(window, flying);

		now_locked = voe_platform_input_pointer_locked(window);
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

			// One of the two, never both, and the camera system
			// applies placements before motions — so a frame that
			// submitted both would take the hand's answer, which is
			// exactly what a handover wants.
			if (flying)
				(void)voe_scene_camera_move(
					world,
					camera_motion(window, eye, step));
			else
				(void)voe_scene_camera_place(
					world, orbit(eye, seconds));

			// The sun, as an intent like everything else.
			(void)voe_scene_light_submit(world,
						     sunlight(sun, seconds));

			// The turning cube, as an intent like everything else.
			//
			// THE SPIN. The turning cube is a transform intent
			// submitted every frame. Same reasoning as orbit's: how
			// a transform is written is `scene`'s, what turns and
			// how fast is a scene's own, and there is no scene file
			// yet.
			spinning = voe_scene_transform_get(world, turning);
			if (spinning != NULL) {
				voe_scene_transform moved = *spinning;

				moved.rotation = voe_math_quat_from_axis_angle(
					spin_axis,
					seconds * TURN / SPIN_SECONDS);
				(void)voe_scene_transform_submit(
					world,
					(voe_scene_transform_intent){
						.entity = turning,
						.transform = moved });
			}
		}

		// The systems, in order, and then the draw. Each of them drains
		// what was submitted since it last ran; nothing here calls into
		// one system from another.
		voe_scene_camera_system_run(world);

		// THE HEADS-UP LINE IS PLACED HERE, BETWEEN TWO SYSTEMS, AND
		// THAT POSITION IS THE WHOLE OF WHETHER IT WORKS. It is derived
		// from where the camera is, so it has to be worked out after the
		// camera system has moved it and submitted before the transform
		// system drains — which is exactly this gap, and it costs
		// nothing: both systems still run once.
		//
		// PLACING IT UP WITH THE OTHER INTENTS PUTS IT ONE FRAME BEHIND,
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
			world,
			voe_dev_facing_the_camera(world, eye, hud, hud_size));
		// The panel travels with the line, one frame behind it in
		// exactly the same way and for exactly the same reason.
		(void)voe_scene_transform_submit(
			world,
			voe_dev_behind_the_line(world, eye, panel, hud_size));
		// And the readout, placed from the camera alone — its geometry
		// does not exist yet and its placement does not need it.
		(void)voe_scene_transform_submit(
			world,
			voe_dev_top_left_of_the_view(world, eye, readout,
						     now_size, READOUT_EM));
		// And the two sprites that turn towards the camera, in this
		// same gap and for this same reason. The engine does not
		// billboard, so this is a call site turning them itself — see
		// src/sprites.c.
		voe_dev_sprites_face(world, eye, &sprites);

		voe_scene_transform_system_run(world);
		voe_scene_light_system_run(world);

		// The line between `update` and `draw`, and the reason the two
		// are measured apart: everything above is this program's own
		// work and everything below is the GPU's frame, the wait for it
		// included. One number covering both would not say which of them
		// grew.
		after_update = voe_platform_clock_now();
		voe_base_samples_add(&timing.update, after_update - top);

		// THE FRAME, IN THE ORDER THE HEADER GIVES: the camera and the sun
		// out of the tables, the draw opened, build what changes this
		// frame, a pass onto the monitor's target with the monitor's
		// camera, a pass onto the window with the world's camera, the
		// walk in each of them, and the draw closed. An open that says
		// there is nothing to draw into skips everything in the middle
		// and the loop comes round again — it does not wait, which is
		// what the spin on a minimised window is.
		frame = voe_3d_draw_system_frame(world, now_size);
		if (!voe_app_draw_open(app, now_size, &drawing))
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
			voe_3d_frame monitor_frame =
				voe_dev_monitor_frame(&monitor, world);
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
			readout_ok = build_the_readout(world, gpu, font, arena,
						       readout, window, &timing,
						       &readout_glyphs, &error);

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
			elements_first = voe_render_frame_elements_submitted(gpu);
			elements_ok = voe_dev_elements_submit(gpu, font);
			elements_count =
				voe_render_frame_elements_submitted(gpu) -
				elements_first;
			(void)voe_3d_panel_set_range(world, exhibit_panel,
						     elements_first,
						     elements_count);

			badge_first = voe_render_frame_elements_submitted(gpu);
			elements_ok = voe_dev_elements_badge_submit(gpu) &&
				      elements_ok;
			(void)voe_3d_panel_set_range(
				world, badge_panel, badge_first,
				voe_render_frame_elements_submitted(gpu) -
					badge_first);

			// THE MONITOR'S PASS, AND IT IS FIRST BECAUSE THE
			// WINDOW'S SHOWS WHAT IT DREW. The same world, the same
			// walk and the same tables, from a camera of its own
			// into a target of its own; the screen that shows the
			// result is left out of it by monitor_frame.hidden.
			// Second would be a target read in one pass and written
			// in the next, and what the window showed would be the
			// picture of the frame before.
			if (!voe_render_pass_begin(gpu, monitor.target,
						   &monitor_camera)) {
				(void)voe_app_draw_close(app);
				break;
			}
			voe_3d_draw_system_run(world, gpu, arena,
					       monitor_frame);
			voe_render_pass_end(gpu);

			// The second pass of a frame on a device made with room
			// for two; refused only if that number were too small,
			// which is this file's mistake. The frame is still
			// closed on the way out, so its slot is not left half
			// recorded.
			if (!voe_render_pass_begin(gpu, VOE_RENDER_TARGET_WINDOW,
						   &pass_camera)) {
				(void)voe_app_draw_close(app);
				break;
			}

			// EITHER SIDE OF THE WALK, WHICH IS WHAT THESE TWO
			// BRACKET AND ALL THEY BRACKET. The difference is every
			// mesh drawn plus one per panel — the panels' own draws
			// are issued inside the walk, sorted among the
			// see-through meshes, so there is no moment between
			// them to read a count at and that is card 032 working
			// rather than something missing here.
			draws_before_walk = voe_render_frame_draw_count(gpu);

			voe_3d_draw_system_run(world, gpu, arena, frame);

			draws_after_walk = voe_render_frame_draw_count(gpu);

			// The screen-filling surface, after the walk because it
			// is not in the world and has nothing to sort against,
			// and before the end so that it is in this frame at
			// all. It is the one surface here that is not a panel
			// and the only one a resize changes — see src/surface.h.
			// Its failure is looked at after the frame has been
			// ended, for the reason the readout's is.
			elements_ok = voe_dev_surface_draw(gpu, now_size) &&
				      elements_ok;

			// And the interface, on the same terms and for the
			// same reasons: it is not in the world either. It goes
			// after the surface so that it is painted over it,
			// which is what submission order means on this path.
			elements_ok = voe_dev_interface_draw(
					      gpu, interface, arena, now_size,
					      voe_platform_input_pointer(window),
					      voe_platform_input_button_down(
						      window,
						      VOE_PLATFORM_BUTTON_LEFT),
					      voe_platform_input_key_down(
						      window,
						      VOE_PLATFORM_KEY_SHIFT),
					      &interface_elements) &&
				      elements_ok;

			voe_render_pass_end(gpu);
			if (!voe_app_draw_close(app))
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
				&timing.draws,
				(double)voe_render_frame_draw_count(gpu));
			timing.drawn_elements =
				voe_render_frame_elements_submitted(gpu);
			timing.draws_measured = true;

			// The element capacity is smaller than the three
			// surfaces need, which is this file's mistake in the
			// same way the readout's transient room would be.
			if (!elements_ok) {
				VOE_BASE_ERROR("dev",
					       "could not submit or draw an element surface — see the refusal above");
				break;
			}
			// A readout that could not be built means the transient
			// room above is too small for it, which is this file's
			// mistake and worth stopping over rather than a refusal
			// line on stderr every frame for as long as it runs.
			if (!readout_ok) {
				VOE_BASE_ERROR("dev",
					       "could not build the readout: %s",
					       voe_base_error_string(error));
				break;
			}
			if (!readout_reported) {
				printf("readout    %u glyphs — %u of %u transient vertices, %u of %u indices, 1 of %u ranges\n",
				       readout_glyphs, readout_glyphs * 4u,
				       4u * MAX_TRANSIENT_GLYPHS,
				       readout_glyphs * 6u,
				       6u * MAX_TRANSIENT_GLYPHS,
				       (unsigned)MAX_TRANSIENT_GEOMETRIES);
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
				       voe_render_frame_draw_count(gpu),
				       timing.drawn_elements,
				       draws_after_walk - draws_before_walk);
				fflush(stdout);
				readout_reported = true;
			}
		}

		after_draw = voe_platform_clock_now();
		voe_base_samples_add(&timing.draw, after_draw - after_update);

		// The card's own measurement of a frame two frames back, when
		// there is one. Asked after the draw because that is what moved
		// it on; a card that cannot time never answers and the gpu line
		// says so rather than reading nought.
		if (voe_render_frame_gpu_time(gpu, &gpu_seconds)) {
			voe_base_samples_add(&timing.gpu, gpu_seconds);
			timing.gpu_timed = true;
		}

		// The mode is asked for every period rather than remembered,
		// because a rebuild is what puts a requested mode in force and
		// that happens inside the draw above.
		if (after_draw - timing.started >= REPORT_SECONDS) {
			present = voe_render_present_get(gpu);
			report(&timing, after_draw - timing.started, present);
			timing.started = after_draw;
		}
	}

stop:
	// The font holds GPU resources, so it goes before the device `app`
	// closes; the arena goes last of the three because the app struct is in
	// it.
	voe_text_font_destroy(font);
	voe_app_destroy(app);
	voe_base_arena_destroy(arena);
	printf("closed\n");
	return 0;
}
