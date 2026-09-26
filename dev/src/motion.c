// How the eye and the sun move each frame. See motion.h for what each function
// answers and who calls it.
//
// EVERY NUMBER IN HERE IS A CALL SITE'S OPINION: which key means forward, how
// wide the orbit is, how long a lap takes and how bright the sun is. What a
// camera does about being moved is the transform system's; this only asks.
//
// THE FLIGHT'S CONSTANTS ARE WHAT "FASTER" AND "MORE SENSITIVE" MEAN. They were
// `scene`'s camera system's until the camera became a lens (0222) and moved
// here unchanged, so flying feels exactly as it did.
//
// PITCH IS CLAMPED SHORT OF STRAIGHT UP, AND YAW IS WRAPPED AND PITCH IS NOT.
// Past straight up the view flips over; a yaw that grows without bound loses
// precision, and a clamped pitch cannot go round at all.
#include "motion.h"

#include <base/assert.h>
#include <math/double3.h>
#include <math/quat.h>
#include <platform/input.h>

#include <math.h>

// A full turn, radians. main.c keeps its own for the turning cube's spin.
#define TURN 6.2831853f

// Metres a second, and what Shift multiplies it by.
#define METRES_PER_SECOND 3.0f
#define FAST_MULTIPLIER 4.0f

// Radians per unit of whatever the window system calls a mouse delta. Small
// because the numbers platform hands out are large.
#define RADIANS_PER_UNIT 0.004f

// Just under a right angle: 89 degrees in radians.
#define PITCH_LIMIT 1.5533431f

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
//   - The view jumps the moment Tab is pressed — the handover. main.c stops
//     orbiting its flight and starts flying it, so the hand starts from the
//     last place the orbit reached. A jump means the flight was replaced
//     rather than carried over.
//   - The mouse turns the wrong way, or up looks down — a sign in
//     voe_dev_fly below.
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
//
// FORWARD IS WHERE THE EYE LOOKS, RIGHT IS KEPT HORIZONTAL AND UP IS THE
// WORLD'S: looking at the floor and asking to rise still rises.
voe_dev_flight voe_dev_fly(voe_platform_window *window, voe_dev_flight flight,
			   float seconds)
{
	float forward = 0.0f;
	float right = 0.0f;
	float up = 0.0f;
	float speed = METRES_PER_SECOND;
	float flat;
	float length;
	voe_math_float3 direction;
	voe_platform_motion mouse;

	VOE_BASE_ASSERT(window != NULL, "flying with no window's keys");
	VOE_BASE_ASSERT(seconds >= 0.0f, "flying backwards through time");

	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_W))
		forward += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_S))
		forward -= 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_D))
		right += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_A))
		right -= 1.0f;
	// Space and E are the same instruction, and so are Ctrl and Q, which is
	// why each pair is one test and not two. Two tests adding a step each
	// would make Space and E held together a rise of two: normalizing later
	// fixes the speed but not the direction.
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_E))
		up += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_CONTROL) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_Q))
		up -= 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SHIFT))
		speed *= FAST_MULTIPLIER;

	// The mouse turns it. Both signs are subtractions: moving the mouse
	// right turns right, which is a smaller yaw because a positive yaw turns
	// left; moving it down looks down, and platform reports +y as down.
	mouse = voe_platform_input_motion(window);
	flight.yaw = fmodf(flight.yaw - mouse.x * RADIANS_PER_UNIT, TURN);
	flight.pitch = fmaxf(-PITCH_LIMIT,
			     fminf(PITCH_LIMIT,
				   flight.pitch - mouse.y * RADIANS_PER_UNIT));

	flat = cosf(flight.pitch);
	direction = (voe_math_float3){
		-sinf(flight.yaw) * flat * forward + cosf(flight.yaw) * right,
		sinf(flight.pitch) * forward + up,
		-cosf(flight.yaw) * flat * forward - sinf(flight.yaw) * right,
	};

	// Normalized, so holding two keys is not faster than holding one — and
	// guarded, because asking for nothing is the ordinary case.
	length = voe_math_float3_length(direction);
	if (length > 0.0f)
		flight.eye = voe_math_double3_add(
			flight.eye,
			voe_math_double3_from_float3(voe_math_float3_scale(
				direction, speed * seconds / length)));
	return flight;
}

// Where the orbit is at this many seconds in: an eye and the two angles that
// look at the origin from it.
//
// THE CAMERA PATH. The orbit is a function of the loop's clock, and main.c
// submits its pose every frame. It is a demonstration and not a feature: how a
// transform is written is `scene`'s, and where a camera should be is whatever
// is driving it.
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
// THE ANGLES ARE WORKED OUT HERE AND KEPT, because a flight is an absolute
// answer and not a direction to guess the angles back out of. It is also what
// makes the handover to flying seamless: the last flight the orbit answered is
// exactly where the hand takes over from.
voe_dev_flight voe_dev_orbit(float seconds)
{
	float angle = seconds * TURN / ORBIT_SECONDS;
	voe_math_float3 position = { sinf(angle) * ORBIT_RADIUS, ORBIT_HEIGHT,
				     cosf(angle) * ORBIT_RADIUS };
	voe_math_float3 towards = voe_math_float3_normalize(
		voe_math_float3_neg(position));
	voe_dev_flight flight = {
		.eye = voe_math_double3_from_float3(position),
		.pitch = asinf(towards.y),
		// Zero yaw looks along -Z and a positive yaw turns towards -X,
		// which is what this pair of arguments says.
		.yaw = atan2f(-towards.x, -towards.z),
	};

	VOE_BASE_ASSERT(seconds >= 0.0f, "an orbit before the clock started");
	VOE_BASE_ASSERT(isfinite(flight.yaw) && isfinite(flight.pitch),
			"an orbit that looks nowhere");
	return flight;
}

// The flight as a transform. _mul reads right to left, so the pitch is the one
// applied first, about the eye's own X, and the yaw then turns the result about
// +Y: yaw about +Y and then pitch about the turned X (0223). Swap them and the
// view rolls as it looks up.
voe_scene_transform voe_dev_flight_pose(voe_dev_flight flight)
{
	static const voe_math_float3 UP = { 0.0f, 1.0f, 0.0f };
	static const voe_math_float3 SIDE = { 1.0f, 0.0f, 0.0f };
	voe_scene_transform pose = {
		.position = flight.eye,
		.rotation = voe_math_quat_mul(
			voe_math_quat_from_axis_angle(UP, flight.yaw),
			voe_math_quat_from_axis_angle(SIDE, flight.pitch)),
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	VOE_BASE_ASSERT(isfinite(flight.yaw) && isfinite(flight.pitch),
			"a pose from angles that are not numbers");
	VOE_BASE_ASSERT(fabsf(voe_math_quat_length(pose.rotation) - 1.0f) <
				VOE_SCENE_TRANSFORM_ROTATION_TOLERANCE,
			"a pose whose rotation is not one");
	return pose;
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
