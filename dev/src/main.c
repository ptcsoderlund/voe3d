// voe_dev — the one program a person runs to see what the engine can currently
// do. Today it opens a window holding a world: two cubes placed by hand, a model
// read out of a `.glb`, and a camera that either orbits them or is flown. There
// is one of these and it always shows the current state, so what is here now is
// expected to be deleted rather than kept behind a flag when the next thing
// lands.
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING. What is here is which key
// means which direction, where a placeholder cube stands, and the loop that runs
// the systems in order. Anything in it that starts to look worth keeping belongs
// in a folder, with a test — the moment it is worth testing it is in the wrong
// place.
//
// THREE THINGS LIVE HERE THAT WILL NOT LIVE HERE LONG, and each of them is a
// call site's business only until the folder that owns it exists:
//
//   - THE CLOCK. Every frame claims a nominal frame's worth of seconds, because
//     nothing in the engine can measure one yet: `platform` will own a clock and
//     card 020 is the card that brings it. Everything downstream of
//     NOMINAL_FRAME_SECONDS is already in seconds, so replacing it is one line.
//   - THE CAMERA PATH. The orbit is a function of that clock and it submits a
//     camera placement every frame. It is a demonstration and not a feature:
//     what a camera does about being moved is `scene`'s, and where a camera
//     should be is whatever is driving it.
//   - THE SPIN. The turning cube is a transform intent submitted every frame.
//     Same reasoning: how a transform is written is `scene`'s, what turns and
//     how fast is a scene's own, and there is no scene file yet.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h, platform/input.h, render/device.h or 3d's
// headers has a hole and that is the finding, not a reason to reach for the
// preprocessor.
//
// ---- WHAT IT SHOULD LOOK LIKE ----
//
// A flat blue-green background with four things in it, from left to right:
//
//   - A lettered cube with a smaller one attached to its top-right corner. That
//     is `dev/src/model.glb`, this repository's own test model, and the small
//     cube is the big one's child in the file.
//   - A cube standing still at the origin, which is where the camera looks.
//   - A cube turning on a tilted axis, just to its right.
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
// NOTHING IS LIT AND THAT IS NOT A BUG YET. Every surface is its base colour
// times its albedo map, so the figure looks flat and its ORM map — occlusion,
// roughness and metalness, which the import reads and stores — changes nothing
// on screen. Card 019 is the card that adds a light and starts reading those
// channels.
//
// It prints a line whenever something changes — the window's size, who is
// drawing its frame, whether the camera is being flown, whether the pointer is
// locked — one line per model at startup for what it cost, and exits zero when
// the window is closed.
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
//
// ---- FLYING: W A S D, Q E, SPACE, CTRL, SHIFT, AND THE MOUSE ----
//
// Tab, then: W and S forwards and back along where the camera is looking, A and
// D left and right, E or Space up and Q or Ctrl down — straight up and down
// whatever the camera is looking at — Shift to go four times as fast, and the
// mouse to look around. Tab again or Escape to give it back. E and Q are there
// so that the whole of flying is reachable from the left hand alone.
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
//     advance while it is away, because the clock counts frames drawn.
//   - Close it. It should print `closed` and exit zero.
//
// It will spin a core while it is open. Presenting waits for the display, so a
// visible window costs one frame's worth of work per refresh — but a minimised
// one presents nothing, and _poll returns immediately because platform has no
// way to wait yet. That is also why every speed here follows the refresh rate.
#include "cubes.h"

#include <3d/draw_system.h>
#include <3d/import.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <assets/image.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <platform/input.h>
#include <platform/window.h>
#include <render/device.h>
#include <scene/camera_system.h>
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

// How much time a frame claims to have taken, in seconds.
//
// IT IS A GUESS AND THIS FILE KNOWS IT. There is nothing here that can ask how
// long the last frame took: time belongs in `platform` and `platform` does not
// have it yet. Sixty is the refresh rate a desktop most often has, so on such a
// display the orbit takes the number of seconds it says it does and on anything
// else it is off by the ratio of the refresh rates. Card 020 measures a frame,
// and this line is the whole of what it replaces.
#define NOMINAL_FRAME_SECONDS (1.0f / 60.0f)

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
// ITS ALBEDO MAP IS WHAT SHOWS TODAY. The ORM map — occlusion, roughness and
// metalness in the red, green and blue channels of one picture — is read,
// uploaded and stored in the material component, and nothing multiplies by it
// yet: there is no light to be rough or metallic in front of. Card 019 is the
// card that lights anything, and it is the one that starts reading those
// channels.
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
					     voe_ecs_entity eye)
{
	voe_scene_camera_motion motion = {
		.entity = eye,
		.seconds = NOMINAL_FRAME_SECONDS,
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

// One cube, one entity: geometry it shares with its neighbour, a material it
// shares with its neighbour, and a transform of its own.
static bool add_cube(voe_ecs_world *world, voe_render_geometry geometry,
		     voe_3d_material material, voe_math_float3 position,
		     voe_ecs_entity *out)
{
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
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
	if (!voe_render_texture_create(gpu, picture.width, picture.height,
				       picture.pixels, &texture, error)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}
	voe_base_arena_rewind(arena, mark);

	material.base_colour_texture = texture;
	if (!voe_3d_material_upload(gpu, &material, error))
		return false;

	// One record and one texture id, two entities: sharing a material is
	// two components holding the same numbers.
	return add_cube(world, geometry, material,
			(voe_math_float3){ 0.0f, 0.0f, 0.0f }, &still) &&
	       add_cube(world, geometry, material,
			(voe_math_float3){ CUBES_APART, 0.0f, 0.0f }, turning);
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

	printf("model      %-9s %u entities, %u meshes, %u materials, %u pictures\n",
	       name, imported.entity_count, imported.geometry_count,
	       imported.material_count, imported.texture_count);

	// The intents carry the transforms by value, so nothing above is read
	// again and the whole import's working memory goes back here.
	voe_base_arena_rewind(arena, mark);
	return true;
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
	// The clock. Counted and not measured — see NOMINAL_FRAME_SECONDS.
	float seconds = 0.0f;

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
	voe_3d_mesh_register(world, MAX_ENTITIES);
	voe_3d_material_register(world, MAX_ENTITIES);

	if (!voe_ecs_entity_create(world, &eye) ||
	    !voe_scene_camera_add(world, eye, camera)) {
		fprintf(stderr, "could not make a camera\n");
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
	fflush(stdout);

	while (!voe_platform_window_should_close(window)) {
		voe_platform_size now_size;
		bool now_decorated;
		bool now_locked;
		bool tab_down;
		const voe_scene_transform *spinning;

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
			seconds += NOMINAL_FRAME_SECONDS;

			// One of the two, never both, and the camera system
			// applies placements before motions — so a frame that
			// submitted both would take the hand's answer, which is
			// exactly what a handover wants.
			if (flying)
				(void)voe_scene_camera_move(
					world, camera_motion(window, eye));
			else
				(void)voe_scene_camera_place(
					world, orbit(eye, seconds));

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

		if (!voe_3d_draw_system_run(world, gpu, now_size)) {
			fprintf(stderr, "the GPU stopped answering\n");
			break;
		}
	}

stop:
	voe_render_device_destroy(gpu);
	voe_base_arena_destroy(arena);
	voe_platform_window_destroy(window);
	printf("closed\n");
	return 0;
}
