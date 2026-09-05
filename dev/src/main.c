// voe_dev — the one program a person runs to see what the engine can currently
// do. Today it opens a window holding a world: two cubes placed by hand, two
// models read out of `.glb` files, one sun going round them, and a camera that
// either orbits them or is flown. There is one of these and it always shows the
// current state, so what is here now is expected to be deleted rather than kept
// behind a flag when the next thing lands.
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING. What is here is which key
// means which direction, where a placeholder cube stands, and the loop that runs
// the systems in order. Anything in it that starts to look worth keeping belongs
// in a folder, with a test — the moment it is worth testing it is in the wrong
// place.
//
// THE CLOCK IS REAL NOW AND CARD 020 IS WHAT MADE IT ONE. Every frame is stepped
// by however long the last one actually took, read from voe_platform_clock_now,
// and not by a nominal sixtieth of a second — so the orbit takes the number of
// seconds it says it does on a display of any refresh rate. The one thing this
// file does to that number is clamp what the scene is stepped by; see
// MAX_FRAME_SECONDS for why, and note that nothing clamps what is reported.
//
// AND IT PRINTS WHAT IT MEASURED. Four numbers every couple of seconds, each an
// average and a worst over exactly that period: the frame, this program's own
// work, the draw, and the graphics card's own clock. say_what_is_measured() is
// the legend and it is printed once at startup, because a number whose meaning
// is ambiguous is worse than no number. P switches between the two present
// modes, which is the measurement the frame-pacing decision is waiting on.
//
// THREE THINGS LIVE HERE THAT WILL NOT LIVE HERE LONG, and each of them is a
// call site's business only until the folder that owns it exists:
//
//   - THE CAMERA PATH. The orbit is a function of that clock and it submits a
//     camera placement every frame. It is a demonstration and not a feature:
//     what a camera does about being moved is `scene`'s, and where a camera
//     should be is whatever is driving it.
//   - THE SPIN. The turning cube is a transform intent submitted every frame.
//     Same reasoning: how a transform is written is `scene`'s, what turns and
//     how fast is a scene's own, and there is no scene file yet.
//   - THE SUN'S PATH. The light circles the scene on the same clock, as a light
//     intent every frame, for one reason: a still light is a light nobody can
//     tell from a wrong one. A scene will say where its sun is the day there is
//     a scene file.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h, platform/input.h, render/device.h or 3d's
// headers has a hole and that is the finding, not a reason to reach for the
// preprocessor.
//
// ---- WHAT IT SHOULD LOOK LIKE ----
//
// A flat blue-green background with four lit things in it, from left to right:
//
//   - A lettered cube with a smaller one attached to its top-right corner. That
//     is `dev/src/model.glb`, this repository's own test model, and the small
//     cube is the big one's child in the file.
//   - A cube standing still at the origin, which is where the camera looks.
//   - A squashed cube turning on a tilted axis, just to its right.
//   - A figure standing on nothing: `dev/src/textured_primitives_human.glb`,
//     three primitives out of Blender sharing one material — a body, a bar of
//     arms and a spherical head, about two metres tall, standing with its feet
//     at y = 0 rather than centred like the cubes.
//
// Every cube wears the same "F", so every face reads as a letter and the letter
// says which way up and which way round the face is. The figure wears its own
// albedo map, which is the thing to look at for whether a real exporter's
// texture coordinates arrive intact.
//
// IT IS LIT BY ONE SUN AND THE SUN GOES ROUND. One directional light, circling
// the scene once every SUN_SECONDS, so the bright side of everything moves and
// the far side of everything is black — there is no ambient light and no bounce,
// so an unlit face really is nothing. The figure's ORM map is read now:
// occlusion, roughness and metalness out of one picture, which is why it does
// not look like plastic in the way the cubes do.
//
// THE TURNING CUBE IS SQUASHED, AND THAT IS THE NORMAL MATRIX ON SCREEN. Its
// scale is not the same on all three axes (CUBE_SCALE_*), which is the one case
// where transforming a normal by the world matrix is visibly wrong: the shading
// would slide across the faces as it turned instead of staying stuck to them.
// 3d/tests/normal_matrix.c is the automated half; this is the half a person can
// see.
//
// NOTHING IS TONE MAPPED, SO THE BRIGHT SIDE CAN CLIP. A highlight that goes
// flat white is expected and is on the later list, not a mistake in the shading.
//
// It prints a line whenever something changes — the window's size, who is
// drawing its frame, whether the camera is being flown, whether the pointer is
// locked — one line per model at startup for what it cost, a block of timings
// every couple of seconds, and exits zero when the window is closed.
//
// TAB FLIES IT AND TAB HANDS IT BACK, AND BOTH STATES ARE WORTH LOOKING AT.
// There are two things to check here and they need different cameras: whether
// the rendering is right, which wants a camera nobody is touching, and whether
// the input is right, which wants a hand on it. Tab switches, Escape always
// hands back, and the orbit is what the program starts in.
//
// ---- ORBITING: THREE MOTIONS, AND ALL THREE HAVE TO BE THERE ----
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
//   - The two models do neither: each sits where its file and one transform
//     intent put it. The lettered model's small cube stays attached to the big
//     one's corner, and that is the flattening — the small cube is the big one's
//     child in the file, and its place in the world is the composition of the
//     two transforms.
//
// WHAT IS WRONG IF IT LOOKS WRONG. Each failure has its own shape:
//
//   - Nothing on screen, or a cube inside out — the depth test or the winding.
//     render/tests/offscreen.c is the automated form of that one.
//   - Everything drifting or growing — the projection or the aspect ratio.
//   - The picture upside down — the one Y flip went the wrong way or happened
//     twice. Every "F" is upright when it is right.
//   - AN "F" THAT READS BACKWARDS — a mirror, and this is the failure worth
//     staring at. A model can come out mirrored from a transposed rotation or a
//     coordinate conversion nobody should have added, and a mirrored cube looks
//     completely normal until you read the letter on it. 3d/tests/import.c is
//     the automated form.
//   - The still cube not still, or not centred — the model matrix or the
//     look-at. scene/tests/transform.c and scene/tests/camera.c check both on
//     the CPU, so this should have failed before it got here.
//   - A model missing while the cubes are there — that import failed and said so
//     on stderr, or the world ran out of room for it. Each model is tried on its
//     own, so one of them can be missing without the other.
//   - The figure's texture smeared or in the wrong place while the cubes' "F"s
//     are right — a real exporter's texture coordinates, which nothing in this
//     repository generated. That is what having a file nobody here wrote is for.
//   - EVERYTHING BLACK — the sun is pointing away from everything, its intensity
//     is nought, or the light never reached the shader. The background is
//     cleared and not lit, so a black scene on a coloured background is a
//     lighting failure and a black window is not.
//   - Everything pale and washed out, or muddy and too dark — a colour space.
//     One of the two sRGB halves (the texture format and the target format) is
//     doing its job without the other; see render/src/texture.c.
//   - The shading sliding across the squashed cube as it turns rather than
//     staying on its faces — the normal matrix, and the one thing that cube is
//     there to show.
//   - The bright side of the still cube not moving as the sun goes round — the
//     light intent is not landing, or the light system is not being run.
//
// ---- FLYING: W A S D, Q E, SPACE, CTRL, SHIFT, AND THE MOUSE ----
//
// Tab, then: W and S forwards and back along where the camera is looking, A and
// D left and right, E or Space up and Q or Ctrl down — straight up and down
// whatever the camera is looking at — Shift to go four times as fast, and the
// mouse to look around. Tab again or Escape to give it back. E and Q are there
// so that the whole of flying is reachable from the left hand alone.
//
// P is the one key that is not about the camera: it switches the present mode,
// in either camera state, and is nowhere near the movement keys for that reason.
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
// WHAT THERE IS TO TRY:
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
//   - WATCH THE TIMING BLOCKS, AND WATCH THEM MOVE. On fifo, `frame` should sit
//     within a few tenths of a millisecond of the display's refresh interval —
//     16.7 ms at sixty hertz — and the reciprocal printed beside it should be
//     the refresh rate. `draw` should be nearly all of it and `update` almost
//     none: the program is waiting for the display, which is what fifo means.
//     Then make something happen: drag the window bigger and `gpu` should go up
//     with the pixel count, minimise it and the rate should go through the roof.
//     A number that never moves is a number that is not being measured.
//   - PRESS P AND COMPARE. It asks for fifo, and the `present` word on every
//     block says which is actually in force — a machine that has no mailbox was
//     already saying fifo and will go on saying it, and that is an answer.
//     What to look for is `frame` dropping onto the display's refresh interval
//     and the rate pinned to the refresh rate, in exchange for not drawing
//     frames nobody ever sees. Press it again to come back.
//   - Hold the window still and read the `worst` column. It should be close to
//     the average. A worst several times the average is stutter, and stutter is
//     the thing a person actually notices — which is why it is printed at all.
//   - Close it. It should print `closed` and exit zero.
//
// IT WILL SPIN A CORE WHILE IT IS OPEN, AND ON MAILBOX IT WILL SPIN THE GRAPHICS
// CARD TOO. That is the engine's default and it is deliberate — performance by
// default, and nothing waits for anything it does not have to — so this program
// runs as fast as the card and the program between them allow, drawing many
// frames for every one anybody sees. P is how to stop it doing that: fifo waits
// for the display, so a visible window then costs one frame's worth of work per
// refresh. Either way a minimised one presents nothing and _poll returns
// immediately, because platform has no way to wait yet.
#include "cubes.h"

#include <3d/draw_system.h>
#include <3d/import.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <assets/image.h>
#include <base/arena.h>
#include <base/error.h>
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
// much more; the rest is headroom for the next thing dropped in here.
#define MAX_VERTICES (64 * 1024)
#define MAX_INDICES (128 * 1024)
#define MAX_MESHES 64
#define MAX_DRAWN_OBJECTS 256
#define MAX_SHADINGS 64

// The longest step the scene is ever advanced by, in seconds, however long the
// frame actually took.
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
// can each be told apart — see the header.
//
// THE RADIUS IS WHATEVER FITS WHAT IS IN THE SCENE, and the scene is about nine
// metres across now that there are two models in it. It has grown twice for that
// reason and it will again; it decides nothing.
#define ORBIT_RADIUS 7.0f
#define ORBIT_HEIGHT 1.8f
#define ORBIT_SECONDS 12.0f

// Where the two placeholder cubes stand, and how fast the second one turns about
// its tilted axis.
#define CUBES_APART 1.6f
#define SPIN_SECONDS 4.0f
#define SPIN_AXIS_X 1.0f
#define SPIN_AXIS_Y 1.0f
#define SPIN_AXIS_Z 0.0f

// The turning cube's scale, and the three numbers are different on purpose: a
// non-uniform scale is the only case where a normal matrix and a world matrix
// disagree, so this is what makes that difference something a person can look
// at. See the header.
#define CUBE_SCALE_X 1.4f
#define CUBE_SCALE_Y 0.6f
#define CUBE_SCALE_Z 1.0f

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

// Where each model is put, once, after it is imported. A file places its
// contents wherever its author left them — a model should not have an opinion
// about what else is in the scene — so this is the call site moving each one out
// of the cubes' way, and it moves them the way anything moves anything: by
// submitting a transform intent.
#define LETTERED_X (-3.5f)
#define HUMAN_X 3.5f

// The picture on the placeholder cubes, embedded at build time.
//
// IT IS AN "F" BECAUSE AN "F" HAS NO SYMMETRY LEFT TO HIDE BEHIND. A checker
// board looks right upside down, a mirrored one looks right too, and both are
// mistakes this engine can make. An F read the wrong way round is obvious across
// the room. The four corner blocks say which corner is which: red is top-left,
// green top-right, blue bottom-left, yellow bottom-right.
static const uint8_t TEXTURE_PNG[] = {
#embed "texture.png"
};

// Two models, embedded the same way as the picture and for the same reason:
// `platform` has no file API yet, so nothing here opens a file. The card that
// gives it one is the card that makes these paths.
//
// THE FIRST ONE IS THE ENGINE'S OWN TEST MODEL: one lettered cube with a second,
// half-sized one as its child, so that the import has a tree to flatten rather
// than a list to copy — and so that a person can see whether the child ended up
// where the composition of the two transforms says it should.
static const uint8_t LETTERED_GLB[] = {
#embed "model.glb"
};

// THE SECOND ONE CAME OUT OF BLENDER, WHICH IS THE POINT OF IT. Everything else
// here was built by this repository and agrees with this repository by
// construction; this is a file a real exporter wrote, with three primitives
// sharing one material, an albedo map and an ORM map, both a thousand pixels
// square. What it is really testing is that the reader survives a file nobody
// here wrote.
//
// BOTH ITS MAPS SHOW NOW. The albedo map is the texture coordinates' half and
// the ORM map — occlusion, roughness and metalness in the red, green and blue
// channels of one picture — is the shading's: card 019 lit the engine and reads
// all three of those channels. Its own export wires that picture into glTF's
// metallic-roughness slot and its normal slot rather than its occlusion slot, so
// the occlusion channel of it is not read as occlusion; that is the file's
// arrangement and assets/include/assets/model.h says why it is taken at its
// word.
static const uint8_t HUMAN_GLB[] = {
#embed "textured_primitives_human.glb"
};

// The bindings, and the only thing in this file that decides anything. Which key
// means forward is a call site's business — the engine's job is to know what
// forward does, not which finger asks for it.
//
// OPPOSITE KEYS HELD TOGETHER CANCEL, WHICH IS WHAT ADDING AND SUBTRACTING GIVES
// FOR FREE. W and S together is nought and not "whichever was pressed last",
// which needs an order this file does not keep.
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

// One cube, one entity: geometry it shares with its neighbour, a material it
// shares with its neighbour, and a transform of its own.
static bool add_cube(voe_ecs_world *world, voe_render_geometry geometry,
		     voe_3d_material material, voe_math_float3 position,
		     voe_math_float3 scale, voe_ecs_entity *out)
{
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = scale,
	};

	if (!voe_ecs_entity_create(world, out))
		return false;
	if (!voe_scene_transform_add(world, *out, transform))
		return false;
	if (!voe_3d_mesh_add(world, *out,
			     (voe_3d_mesh){ .geometry = geometry }))
		return false;
	return voe_3d_material_add(world, *out, material);
}

// The two placeholder cubes: their geometry into the pools, the "F" into a
// texture slot, one shading record for both of them, and two entities.
static bool add_the_cubes(voe_ecs_world *world, voe_render_device *gpu,
			  voe_base_arena *arena, voe_ecs_entity *turning,
			  voe_base_error *error)
{
	voe_render_geometry geometry = { 0 };
	voe_render_texture texture = { 0 };
	voe_3d_material material = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 0.8f,
	};
	voe_assets_image picture;
	voe_ecs_entity still = { 0 };
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

	if (!voe_render_geometry_create(gpu, voe_dev_cube_vertices,
					VOE_DEV_CUBE_VERTEX_COUNT,
					voe_dev_cube_indices,
					VOE_DEV_CUBE_INDEX_COUNT, &geometry,
					error))
		return false;

	// The picture arrives the way the shaders do: `#embed`ded at build time,
	// which is also what keeps "nothing is read from disk at run time" true.
	// The decoded pixels are scratch — `render` has taken its own copy by
	// the time the upload returns — so the arena goes back afterwards.
	if (!voe_assets_png_decode(TEXTURE_PNG, sizeof(TEXTURE_PNG), arena,
				   &picture, error))
		return false;
	// A colour, so it goes up in the sRGB format and the hardware decodes it
	// before the shading multiplies by it. An ORM map would be the other kind
	// — see voe_render_texture_kind.
	if (!voe_render_texture_create(gpu, VOE_RENDER_TEXTURE_COLOUR,
				       picture.width, picture.height,
				       picture.pixels, &texture, error)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}
	voe_base_arena_rewind(arena, mark);

	material.base_colour_texture = texture;
	if (!voe_3d_material_upload(gpu, &material, error))
		return false;

	// One record and one texture id, two entities: sharing a material is
	// two components holding the same numbers. The second one is squashed,
	// which is what makes the normal matrix visible — see the header.
	return add_cube(world, geometry, material,
			(voe_math_float3){ 0.0f, 0.0f, 0.0f },
			(voe_math_float3){ 1.0f, 1.0f, 1.0f }, &still) &&
	       add_cube(world, geometry, material,
			(voe_math_float3){ CUBES_APART, 0.0f, 0.0f },
			(voe_math_float3){ CUBE_SCALE_X, CUBE_SCALE_Y,
					   CUBE_SCALE_Z },
			turning);
}

// One model, then one transform intent per entity to move the whole thing aside.
//
// EVERY ENTITY HAS TO BE MOVED AND NOT JUST THE FIRST, WHICH IS THE FLATTENING
// SHOWING THROUGH. There is no parent component: the import composed the file's
// tree into world transforms, so moving a model means moving each of the things
// it turned into. The card that adds a hierarchy is the card that makes this one
// intent.
static bool add_a_model(voe_ecs_world *world, voe_render_device *gpu,
			voe_base_arena *arena, const char *name,
			const uint8_t *bytes, size_t size, float offset_x,
			voe_base_error *error)
{
	voe_3d_import imported = { 0 };
	// Everything the import builds on the way through — the parsed model,
	// the decoded picture, the JSON, the list of entities — is scratch: the
	// GPU has taken its own copy of the uploads and the components hold the
	// ids, so none of it is read after this function returns. It is a few
	// megabytes and this is the mark that gives them back.
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

	if (!voe_3d_import_glb(world, gpu, arena, bytes, size, &imported,
			       error)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}

	for (uint32_t i = 0; i < imported.entity_count; i++) {
		const voe_scene_transform *placed =
			voe_scene_transform_get(world, imported.entities[i]);
		voe_scene_transform moved;

		if (placed == NULL)
			continue;

		// Read, change, submit: an intent carries the whole transform,
		// so a submitter reads the current one first. Reading is
		// anybody's; writing is the transform system's.
		moved = *placed;
		moved.position.x += offset_x;
		if (!voe_scene_transform_submit(
			    world, (voe_scene_transform_intent){
					   .entity = imported.entities[i],
					   .transform = moved })) {
			voe_base_arena_rewind(arena, mark);
			return false;
		}
	}

	// Textures and not pictures: a picture wanted as both a colour and a data
	// map is uploaded twice, so the two numbers are not always the same. See
	// 3d/import.h.
	printf("model      %-9s %u entities, %u meshes, %u materials, %u textures\n",
	       name, imported.entity_count, imported.geometry_count,
	       imported.material_count, imported.texture_count);

	// The intents carry the transforms by value, so nothing above is read
	// again and the whole import's working memory goes back here.
	voe_base_arena_rewind(arena, mark);
	return true;
}

// The four numbers the loop measures, and the clock reading that says when a
// period ends. One struct because they are gathered together, reported together
// and reset together, and four loose pairs at the top of main() would be twelve
// variables to keep in step.
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
	// Inside voe_3d_draw_system_run: waiting for the frame slot, taking a
	// swapchain image, recording every draw, submitting and presenting.
	voe_base_samples draw;
	// The graphics card's own clock, over that frame's commands only.
	voe_base_samples gpu;
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
	printf("           draw    inside the draw system: the wait for the frame slot,\n");
	printf("                   the acquire, the recording, the submit and the present.\n");
	printf("                   On fifo the wait for the display is in here and is most of it\n");
	printf("           gpu     the graphics card's own clock, over that frame's commands\n");
	printf("                   only — the wait for the display is not in it. It runs two\n");
	printf("                   frames behind, and is absent on a card that cannot time\n");
	printf("           P switches between fifo and mailbox. Numbers from both are what\n");
	printf("           the frame-pacing decision wants; the mode is on every block below\n");
}

// One block, and then the period starts again. `seconds` is how long the period
// really lasted rather than REPORT_SECONDS, because a frame straddles the end of
// one and the rate has to be over the time actually covered.
//
// THE GPU LINE IS ABSENT RATHER THAN NOUGHT WHEN THERE IS NO MEASUREMENT. A card
// that cannot write timestamps would otherwise report a graphics card that takes
// no time at all, which is the most misleading thing this could print.
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
	fflush(stdout);

	voe_base_samples_reset(&timing->frame);
	voe_base_samples_reset(&timing->update);
	voe_base_samples_reset(&timing->draw);
	voe_base_samples_reset(&timing->gpu);
}

int main(void)
{
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
	voe_math_float3 spin_axis = { SPIN_AXIS_X, SPIN_AXIS_Y, SPIN_AXIS_Z };
	voe_render_capacities capacities = {
		.vertices = MAX_VERTICES,
		.indices = MAX_INDICES,
		.geometries = MAX_MESHES,
		.objects = MAX_DRAWN_OBJECTS,
		.shadings = MAX_SHADINGS,
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
	// Last frame's P, for the same reason, and the mode it asks for. It
	// starts true because a device opens wanting mailbox — this is what the
	// engine already asked for and not a second opinion, so the first press
	// of P asks for fifo rather than for what is already happening.
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
	// reading, `previous` the last one, and the difference between them is
	// the frame.
	struct timing timing = { 0 };
	double previous;
	double top;
	double after_update;
	double after_draw;
	double step;

	window = voe_platform_window_new(960, 540,
					 "voe3d — a model, two cubes, one camera");
	if (window == NULL) {
		fprintf(stderr, "could not open a window\n");
		return 1;
	}

	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	gpu = voe_render_device_new(scratch, voe_platform_window_native(window),
				    voe_platform_window_size(window),
				    capacities, &error);
	voe_base_arena_destroy(scratch);
	if (gpu == NULL) {
		fprintf(stderr, "could not start the GPU: %s\n",
			voe_base_error_string(error));
		voe_platform_window_destroy(window);
		return 1;
	}

	// The world and everything read into it live here, and it is destroyed
	// at the end: the world is the arena's, which is what rule 11 asks for.
	arena = voe_base_arena_new(WORLD_ARENA);
	world = voe_ecs_world_new(arena, limits);

	// Registration, once, and this is the whole of what a call site has to
	// know about which components exist. Each folder says what one of its
	// components is; nothing here does.
	voe_scene_transform_register(world, MAX_ENTITIES);
	voe_scene_camera_register(world, 4);
	voe_scene_light_register(world, 4);
	voe_3d_mesh_register(world, MAX_ENTITIES);
	voe_3d_material_register(world, MAX_ENTITIES);

	if (!voe_ecs_entity_create(world, &eye) ||
	    !voe_scene_camera_add(world, eye, camera)) {
		fprintf(stderr, "could not make a camera\n");
		goto stop;
	}

	// The sun, at wherever its lap starts. The draw system needs exactly one
	// light in the world, so this is not optional wiring — a world without it
	// asserts rather than drawing something black.
	if (!voe_ecs_entity_create(world, &sun) ||
	    !voe_scene_light_add(world, sun, sunlight(sun, 0.0f).light)) {
		fprintf(stderr, "could not make a sun\n");
		goto stop;
	}

	if (!add_the_cubes(world, gpu, arena, &turning, &error)) {
		fprintf(stderr, "could not build the two cubes: %s\n",
			voe_base_error_string(error));
		goto stop;
	}

	// A model is the one thing here that is allowed to fail without stopping
	// the program: the cubes are what says the renderer works, and a person
	// looking at a window is better served by seeing them and a message than
	// by seeing nothing. Each one is tried on its own, so a file that cannot
	// be read does not take the other one with it.
	if (!add_a_model(world, gpu, arena, "lettered", LETTERED_GLB,
			 sizeof(LETTERED_GLB), LETTERED_X, &error))
		fprintf(stderr, "could not read the lettered model: %s\n",
			voe_base_error_string(error));
	if (!add_a_model(world, gpu, arena, "human", HUMAN_GLB,
			 sizeof(HUMAN_GLB), HUMAN_X, &error))
		fprintf(stderr, "could not read the human model: %s\n",
			voe_base_error_string(error));

	size = voe_platform_window_size(window);
	decorated = voe_platform_window_decorated(window);
	printf("opened     %dx%d\n", size.width, size.height);
	printf("decorated  %s\n", decorated ? "yes" : "no");
	printf("camera     orbit — Tab to fly, Escape to hand it back\n");
	present = voe_render_present_get(gpu);
	printf("present    %s\n",
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	say_what_is_measured();
	fflush(stdout);

	// The first reading, before the loop, so that the first frame's interval
	// is measured from here rather than from a zero that would report the
	// whole of startup as one very slow frame.
	previous = voe_platform_clock_now();
	timing.started = previous;

	while (!voe_platform_window_should_close(window)) {
		voe_platform_size now_size;
		bool now_decorated;
		bool now_locked;
		bool tab_down;
		bool p_down;
		const voe_scene_transform *spinning;
		double elapsed;
		double gpu_seconds;

		// The frame's interval, measured before anything in it happens,
		// so that everything below is inside it.
		top = voe_platform_clock_now();
		elapsed = top - previous;
		previous = top;
		voe_base_samples_add(&timing.frame, elapsed);

		// What the scene is advanced by: the same number, clamped. The
		// sample above got the unclamped one, because what is reported
		// is what happened — see MAX_FRAME_SECONDS.
		step = elapsed > MAX_FRAME_SECONDS ? MAX_FRAME_SECONDS : elapsed;

		voe_platform_window_poll(window);

		// Poll, then report what changed. Everything is asked every
		// frame because platform hands out state, not events.
		now_size = voe_platform_window_size(window);
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

		// Tab toggles on the press and not while held, which is the one
		// place this file has to turn state back into an edge. Escape
		// only ever hands control back.
		tab_down = voe_platform_input_key_down(window,
						       VOE_PLATFORM_KEY_TAB);
		if (tab_down && !tab_was_down)
			flying = !flying;
		tab_was_down = tab_down;

		if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_ESCAPE))
			flying = false;

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
		// below advances a scene nobody is looking at.
		if (now_size.width > 0 && now_size.height > 0) {
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
		voe_scene_transform_system_run(world);
		voe_scene_light_system_run(world);

		// The line between `update` and `draw`, and the reason the two
		// are measured apart: everything above is this program's own
		// work and everything below is the GPU's frame, the wait for it
		// included. One number covering both would not say which of them
		// grew.
		after_update = voe_platform_clock_now();
		voe_base_samples_add(&timing.update, after_update - top);

		if (!voe_3d_draw_system_run(world, gpu, now_size)) {
			fprintf(stderr, "the GPU stopped answering\n");
			break;
		}

		after_draw = voe_platform_clock_now();
		voe_base_samples_add(&timing.draw, after_draw - after_update);

		// The card's own measurement of a frame two frames back, when
		// there is one. Asked after the draw because that is what moved
		// it on; a card that cannot time never answers and the gpu line
		// says so rather than reading nought.
		if (voe_render_frame_gpu_time(gpu, &gpu_seconds))
			voe_base_samples_add(&timing.gpu, gpu_seconds);

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
	voe_render_device_destroy(gpu);
	voe_base_arena_destroy(arena);
	voe_platform_window_destroy(window);
	printf("closed\n");
	return 0;
}
