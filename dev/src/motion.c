// How the eye and the sun move each frame. See motion.h for what each function
// answers and who calls it.
//
// EVERY NUMBER IN HERE IS A CALL SITE'S OPINION: which key means forward, how
// wide the orbit is, how long a lap takes and how bright the sun is. What a
// camera does about being moved is `scene`'s; this only asks.
#include "motion.h"

#include <math/float3.h>
#include <platform/input.h>

#include <math.h>

// A full turn, radians. main.c keeps its own for the turning cube's spin.
#define TURN 6.2831853f

// The orbit: how far out, how high, and how long a lap takes. Three motions that
// can each be told apart — see voe_dev_orbit().
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
voe_scene_camera_motion voe_dev_camera_motion(voe_platform_window *window,
					      voe_ecs_entity eye,
					      float seconds)
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
voe_scene_camera_placement voe_dev_orbit(voe_ecs_entity eye, float seconds)
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
voe_scene_light_intent voe_dev_sunlight(voe_ecs_entity sun, float seconds)
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
