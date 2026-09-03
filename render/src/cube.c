// The cubes: eight vertices and thirty-six indices shared by both of them, the
// matrices that place them, the camera that looks at them, and the descriptor
// the shader reads it through. The geometry and the descriptor have a startup
// lifetime; the matrices are built fresh every frame from one number, because
// the scene now moves.
//
// TWO CUBES, ONE TURNING AND ONE STANDING STILL, AND THAT IS THE POINT RATHER
// THAN A DEMO. With one object you cannot tell an orbiting camera from a
// rotating cube — the picture is identical. Three independent motions can each
// be told apart, so a mistake in the view matrix, the model matrix or the
// projection shows up as a specific wrong thing instead of "something looks
// off". The still one is at the origin, which is also what the camera looks at,
// so it holds the centre of the frame while its faces turn; the turning one
// stands to one side, so parallax carries it across the frame and in front of
// and behind its neighbour. Between them there is nothing an orbiting camera and
// a rotating object could both explain.
//
// THIS IS STILL A PLACEHOLDER, AND WHAT IS LEFT OF IT IS THE ARRANGEMENT RATHER
// THAN THE CAMERA. render has no scene to ask and must not grow one: geometry
// that is not a cube arrives with the card that loads a file, and many objects
// with the card that needs a table to keep them in. What is worth keeping here
// is the plumbing under it — buffers, an upload, a descriptor, a depth test, a
// push constant per draw — not the two cubes.
//
// THERE ARE TWO CAMERAS AND THE CALLER CHOOSES BETWEEN THEM EVERY FRAME. The
// orbit is a function of the clock and takes no input; the flown one is a
// position and two angles moved by what a person did. Both build a view matrix
// through the same look_at below, so whichever is in use, everything downstream
// of it is the same code — and a bug in the matrix chain shows up in both rather
// than in one.
//
// THE FLOWN CAMERA'S NUMBERS ARE HERE, WITH THE ORBIT'S, BECAUSE THIS FOLDER
// STILL OWNS THE CAMERA. How fast it walks, how far a mouse turns it and how
// close to straight up it may look are #defines below — the same standing
// FIELD_OF_VIEW and ORBIT_RADIUS have. What the caller supplies is which way,
// not how fast; include/render/device.h says why the split falls there.
//
// AND IT KNOWS NOTHING ABOUT A KEYBOARD. voe_render_camera_input is a direction
// and a mouse delta, not a key. Which key means forward is the caller's business
// and it stays out of this folder, which is the same rule that keeps the window's
// size a parameter rather than a question render asks.
//
// THE MODEL MATRIX IS A PUSH CONSTANT AND THE CAMERA IS A UNIFORM BUFFER, AND
// THE SPLIT IS BY HOW OFTEN EACH CHANGES. The camera is written once per frame
// and read by every draw in it, which is what a uniform buffer is; the model
// matrix differs between two draws in the same command buffer, which a single
// uniform buffer cannot express without either a second buffer, a dynamic offset
// or a descriptor per object. A push constant is none of those: sixty-four bytes
// recorded into the command buffer between the two draws, no allocation, no
// descriptor, and nothing per-object to tear down. It is also the option that
// runs out first — see voe_render_push in device_internal.h for the size floor —
// and running out is what the card that introduces many objects has to answer.
//
// EIGHT VERTICES, NOT TWENTY-FOUR, AND THAT IS THE EARLIER CARD'S DECISION. A
// corner shared by three faces can share one position and one colour, but it
// cannot share three different texture coordinates; the moment texture
// coordinates exist this becomes twenty-four vertices and the data below is
// rewritten. That is known and accepted rather than discovered later.
//
// THE WINDING IS COUNTER-CLOCKWISE SEEN FROM OUTSIDE, WHICH IS glTF'S AND SO
// THIS ENGINE'S. Every one of the twelve triangles below is wound so that its
// cross product points out of the cube. The pipeline in device.c culls back
// faces and names counter-clockwise as the front, and the viewport in frame.c
// flips Y — three facts that only agree by construction, which is what
// render/tests/offscreen.c is for.
//
// THE CAMERA LOOKS ALONG ITS OWN -Z AND THE VIEW MATRIX IS BUILT HERE BECAUSE
// math WILL NOT BUILD IT. A look-at is a camera, math is pure value types with
// no opinion about cameras, and a projection matrix encodes a clip-space
// convention that math is explicitly not allowed to know — see float4x4.h. So
// both are assembled here out of math's vectors, and this file is where this
// engine's conventions turn into sixteen floats. The rotation is the exception:
// a quaternion is a value type with no clip space in it, so it is math's, and
// voe_math_float4x4_from_quat is what this file calls.
//
// THE ORBIT AND THE SPIN TURN THE SAME WAY, AND THAT WAY IS THE ENGINE'S. A
// positive angle about +Y takes +Z towards +X, which is what math/tests/quat.c
// proves and what the sines below are written to match. Two conventions here
// that disagreed would be invisible: both would still turn.
#include "device_internal.h"

#include <base/assert.h>
#include <math/quat.h>

#include <math.h>
#include <stdio.h>

// Half the side, so a cube spans one metre. Units are metres (CLAUDE.md).
#define HALF 0.5f

// A whole turn, in radians. Every period below is a number of seconds for one
// of these, which is the unit a person can check with a stopwatch — radians per
// second is not.
#define TURN 6.2831853f

// 60 degrees of vertical field of view, in radians because every angle in this
// engine is. Vertical and not horizontal, because the projection below divides
// by the aspect ratio rather than multiplying, so a wider window shows more
// rather than the same amount squashed.
#define FIELD_OF_VIEW 1.0471976f

// The near plane, in metres. There is deliberately no far plane: see
// voe_render_cube_projection.
#define NEAR_PLANE 0.1f

// The camera's orbit: a circle about +Y through the origin, at a fixed height
// above it, always looking at it. Above and not level, so that a top face is
// visible and three faces of a cube are on screen at once — head on, a cube is a
// square and a broken depth test looks fine.
//
// THE RADIUS IS WHAT KEEPS THE NEAR PLANE OUT OF IT. The nearest the eye ever
// gets to a cube's centre is the radius minus how far that cube stands from the
// origin, and the nearest corner is another half diagonal in from there; with
// the numbers below that is over two metres against a near plane of ten
// centimetres. render/tests/matrix.c does that arithmetic over the whole orbit
// rather than leaving it to this comment.
#define ORBIT_RADIUS 4.0f
#define ORBIT_HEIGHT 1.8f
#define ORBIT_SECONDS 12.0f

// The flown camera's three numbers.
//
// Metres per second, and metres per second with `fast` held. Three is a brisk
// walk, which is the right order of magnitude for a scene one metre across —
// something the size of a building wants a different number and it wants it from
// outside this folder, which is the card that gives the camera a home.
#define FLY_METRES_PER_SECOND 3.0f
#define FLY_FAST_MULTIPLIER 4.0f

// Radians per unit of whatever the window system called mouse movement. There is
// no principled value for this: the unit is the compositor's on one platform and
// the mouse's own counts on the other — see voe_platform_input_motion — so this
// is a number picked by moving a mouse and looking, and it is the one line to
// change when it feels wrong. A quarter of a degree per unit, roughly, which
// puts a half-turn at about a hand's width of desk.
#define FLY_RADIANS_PER_UNIT 0.004f

// How close to straight up or straight down the camera may look, in radians.
//
// IT IS NOT π/2 AND THE CLAMP IS NOT COSMETIC. look_at builds the camera's right
// axis by crossing the world's up vector with the direction of view; looking
// exactly along up makes those two parallel, the cross product zero, and the
// normalize that follows a division by zero — so the view matrix fills with NaN
// and the whole frame disappears. A degree short of it is enough and it is what
// every camera like this does.
#define FLY_PITCH_LIMIT 1.5533431f

// The second cube's spin, and how far along +X it stands from the first.
//
// THE SPIN AXIS IS TILTED ON PURPOSE, AND +Y WOULD HAVE BEEN THE AMBIGUOUS
// CHOICE. The camera orbits about +Y, so a cube spinning about +Y as well would
// look like the same motion at a different speed, which is the one thing this
// arrangement exists to rule out. A tilted axis cannot be confused with an orbit
// and it brings the top and bottom faces round as well. It need not be unit:
// voe_math_quat_from_axis_angle normalizes it.
#define APART 1.6f
#define SPIN_SECONDS 4.0f
#define SPIN_AXIS_X 1.0f
#define SPIN_AXIS_Y 1.0f
#define SPIN_AXIS_Z 0.0f

// The eight corners, and a colour per corner rather than per face. The colour is
// the position moved into 0..1, so opposite corners are opposite colours and no
// two faces read the same — which is the whole of how a person checks by eye
// that they are looking at the near face and not through it. Both cubes are this
// one buffer: what differs between them is one matrix.
static const struct voe_render_vertex CUBE_VERTICES[8] = {
	{ { -HALF, -HALF, -HALF }, { 0.0f, 0.0f, 0.0f } },
	{ { HALF, -HALF, -HALF }, { 1.0f, 0.0f, 0.0f } },
	{ { HALF, HALF, -HALF }, { 1.0f, 1.0f, 0.0f } },
	{ { -HALF, HALF, -HALF }, { 0.0f, 1.0f, 0.0f } },
	{ { -HALF, -HALF, HALF }, { 0.0f, 0.0f, 1.0f } },
	{ { HALF, -HALF, HALF }, { 1.0f, 0.0f, 1.0f } },
	{ { HALF, HALF, HALF }, { 1.0f, 1.0f, 1.0f } },
	{ { -HALF, HALF, HALF }, { 0.0f, 1.0f, 1.0f } },
};

// Two triangles per face, six faces, counter-clockwise seen from outside. The
// faces are in the order +Z -Z +X -X +Y -Y so that a reader can check one
// against the corner table above without hunting.
static const uint16_t CUBE_INDICES[36] = {
	4, 5, 6, 4, 6, 7, // +Z
	1, 0, 3, 1, 3, 2, // -Z
	5, 1, 2, 5, 2, 6, // +X
	0, 4, 7, 0, 7, 3, // -X
	7, 6, 2, 7, 2, 3, // +Y
	0, 1, 5, 0, 5, 4, // -Y
};

// ------------------------------------------------------------- the matrices

// The camera, as a view matrix. Right-handed, +Y up, and the camera looks along
// its own -Z, so the third row is the direction pointing *back* at the viewer
// and not the direction of view. Getting that sign wrong puts the world behind
// the camera, which looks exactly like nothing being drawn.
//
// The rotation is the transpose of the camera's axes — the inverse of a rotation
// — and the translation column is minus each axis projected onto the eye, which
// is the inverse of the translation expressed in those axes. Written out rather
// than composed from two matrices and an inverse, because the closed form is
// three dot products and the general inverse is not free.
static voe_math_float4x4 look_at(voe_math_float3 eye, voe_math_float3 target,
				 voe_math_float3 up)
{
	voe_math_float3 forward = voe_math_float3_normalize(
		voe_math_float3_sub(target, eye));
	// The camera's own +Z, which points from the target back to the eye.
	voe_math_float3 z = voe_math_float3_neg(forward);
	voe_math_float3 x = voe_math_float3_normalize(
		voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_math_float4x4 view = { 0 };

	view.m[0][0] = x.x;
	view.m[0][1] = x.y;
	view.m[0][2] = x.z;
	view.m[0][3] = -voe_math_float3_dot(x, eye);

	view.m[1][0] = y.x;
	view.m[1][1] = y.y;
	view.m[1][2] = y.z;
	view.m[1][3] = -voe_math_float3_dot(y, eye);

	view.m[2][0] = z.x;
	view.m[2][1] = z.y;
	view.m[2][2] = z.z;
	view.m[2][3] = -voe_math_float3_dot(z, eye);

	view.m[3][3] = 1.0f;
	return view;
}

voe_math_float4x4 voe_render_cube_projection(VkExtent2D extent)
{
	// cot(fov/2): how far the near plane is from the eye in units of half
	// its own height.
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float aspect;
	voe_math_float4x4 projection = { 0 };

	VOE_BASE_DEBUG_ASSERT(extent.width > 0 && extent.height > 0,
			      "a projection for a target with no area");

	aspect = (float)extent.width / (float)extent.height;

	// DEPTH RUNS BACKWARDS AND THE FAR PLANE IS AT INFINITY. Read the third
	// row as arithmetic: clip.z is NEAR_PLANE for every vertex and clip.w is
	// -z, so the depth the rasteriser sees is NEAR_PLANE / -z. At the near
	// plane that is 1.0; as z goes to negative infinity it approaches 0.0
	// and never reaches it. That is this engine's convention — near 1, far
	// 0, cleared to 0, compared GREATER — and it is why there is no far
	// plane to pass in and nothing to clip against at the back.
	//
	// Every tutorial writes the reciprocal of this and CLAUDE.md says not to
	// correct it. A float depth buffer has its precision bunched near zero,
	// which is where the distance is now, so this arrangement spends
	// precision where the geometry is instead of where it is not.
	//
	// NOTHING HERE NEGATES Y. Vulkan's clip space is Y-down and the whole of
	// the reconciliation is the negative viewport height in frame.c. A minus
	// sign on m[1][1] as well would flip twice, and flipping twice is
	// invisible until something is culled.
	projection.m[0][0] = focal / aspect;
	projection.m[1][1] = focal;
	projection.m[2][3] = NEAR_PLANE;
	projection.m[3][2] = -1.0f;
	return projection;
}

// Where the orbiting camera's eye is at this many seconds in.
//
// sine on x and cosine on z, so that a growing angle carries the eye from +Z
// towards +X — the same direction a positive rotation about +Y turns, which is
// the engine's handedness and what math/tests/quat.c proves. Cosine on x instead
// would orbit the other way and look exactly as convincing.
//
// Zero seconds therefore puts the eye straight above and behind the origin on
// the +Z axis, pitched down at it. That is also the only moment every test in
// this folder that draws ever sees, because nothing but a real frame advances
// the clock.
//
// It is a function rather than four lines inside the fill below because two
// things ask where the orbit is: the orbit, and the flown camera taking over
// from it.
static voe_math_float3 orbit_eye(float seconds)
{
	float angle = seconds * TURN / ORBIT_SECONDS;

	return (voe_math_float3){ sinf(angle) * ORBIT_RADIUS, ORBIT_HEIGHT,
				  cosf(angle) * ORBIT_RADIUS };
}

// Which way the flown camera is looking, from its two angles. Unit by
// construction, so nothing normalizes it.
//
// READ IT AGAINST yaw's COMMENT IN device_internal.h AND THE SIGNS ARE THE WHOLE
// OF THIS FUNCTION. At yaw and pitch zero this is (0, 0, -1), which is where a
// camera in this engine looks; a positive yaw swings it towards -X, which is a
// left turn, because a positive rotation about +Y takes +Z to +X and the
// direction of view is the negative of that. Both cosines carry the pitch, so
// the vector stays unit without a normalize.
static voe_math_float3 fly_forward(float yaw, float pitch)
{
	float flat = cosf(pitch);

	return (voe_math_float3){ -sinf(yaw) * flat, sinf(pitch),
				  -cosf(yaw) * flat };
}

// The flown camera's right, and it is horizontal whatever the pitch is.
//
// THAT IS THE WHOLE REASON IT IS NOT cross(forward, up). The cross product is
// this vector multiplied by cos(pitch), so normalizing it gives exactly what is
// written here — and written here it costs no normalize and it does not divide
// by zero when the pitch is straight up. Horizontal is also what strafing should
// be: looking at the floor and stepping right should step right, not into the
// floor.
static voe_math_float3 fly_right(float yaw)
{
	return (voe_math_float3){ cosf(yaw), 0.0f, -sinf(yaw) };
}

void voe_render_cube_camera_step(struct voe_render_camera *camera,
				 voe_render_camera_input input, float seconds,
				 float dt)
{
	voe_math_float3 world_up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 direction = { 0.0f, 0.0f, 0.0f };
	float length;
	float speed = FLY_METRES_PER_SECOND;

	VOE_BASE_DEBUG_ASSERT(camera != NULL, "flying nothing");

	// Not flying, and nothing to remember: the orbit is a function of the
	// clock and keeps no state at all.
	if (!input.fly) {
		camera->flying = false;
		return;
	}

	// The frame control is taken. Put where the orbit had reached and
	// pointed the way the orbit was pointing — which is at the origin — so
	// that the picture does not move on the frame a person takes over.
	//
	// The two angles come back out of the direction with the inverses of
	// fly_forward: pitch is the arcsine of the y it built, and yaw is the
	// arctangent of the other two with both signs put back. Seeding a
	// position and leaving the angles at zero would have the camera in the
	// right place looking the wrong way, which reads as a jump.
	if (!camera->flying) {
		voe_math_float3 eye = orbit_eye(seconds);
		voe_math_float3 view = voe_math_float3_normalize(
			voe_math_float3_neg(eye));

		camera->eye = eye;
		camera->pitch = asinf(view.y);
		camera->yaw = atan2f(-view.x, -view.z);
		camera->flying = true;
	}

	// The mouse. Right and down are both positive out of platform, and both
	// turn the camera that way: yaw comes down because a positive yaw is a
	// left turn, and pitch comes down because looking down is a smaller
	// pitch. Neither is scaled by dt — a mouse reports how far it moved, not
	// how fast, so a frame that took twice as long has twice the movement in
	// it already and multiplying again would make looking around depend on
	// the frame rate.
	camera->yaw -= input.look_x * FLY_RADIANS_PER_UNIT;
	camera->pitch -= input.look_y * FLY_RADIANS_PER_UNIT;

	// Kept inside one turn. sinf and cosf do not care how large the angle
	// is, but the subtraction above does: adding four thousandths to a yaw
	// of ten thousand radians loses most of it, and ten thousand radians is
	// twenty minutes of spinning.
	camera->yaw = fmodf(camera->yaw, TURN);

	// The clamp, and it is not cosmetic — see FLY_PITCH_LIMIT.
	camera->pitch = fmaxf(-FLY_PITCH_LIMIT,
			      fminf(FLY_PITCH_LIMIT, camera->pitch));

	// Where a step goes, in the camera's frame for two of the three axes and
	// the world's for the third.
	//
	// UP IS THE WORLD'S AND NOT THE CAMERA'S, ON PURPOSE. A camera looking
	// at the floor should still rise when asked to rise; using its own up
	// would send it forwards instead, which is correct for a spacecraft and
	// wrong for anything a person is trying to fly around a scene with.
	direction = voe_math_float3_add(
		direction, voe_math_float3_scale(
				   fly_forward(camera->yaw, camera->pitch),
				   input.forward));
	direction = voe_math_float3_add(
		direction,
		voe_math_float3_scale(fly_right(camera->yaw), input.right));
	direction = voe_math_float3_add(
		direction, voe_math_float3_scale(world_up, input.up));

	if (input.fast)
		speed *= FLY_FAST_MULTIPLIER;

	// NORMALIZED, SO THAT TWO KEYS ARE NOT FASTER THAN ONE. Forward and
	// right together are a vector of length root two, and moving along it
	// unscaled is the oldest bug in this kind of camera: a diagonal is forty
	// per cent quicker than a straight line and it is invisible until
	// someone races along one. The length is tested rather than normalized
	// blind, because normalizing a vector of no length is a division by zero
	// and standing still is the commonest case there is.
	length = voe_math_float3_length(direction);
	if (length > 0.0f)
		camera->eye = voe_math_float3_add(
			camera->eye,
			voe_math_float3_scale(direction, speed * dt / length));
}

void voe_render_cube_uniforms_fill(struct voe_render_uniforms *uniforms,
				   VkExtent2D extent,
				   const struct voe_render_camera *camera,
				   float seconds)
{
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 eye;
	voe_math_float3 target;

	VOE_BASE_DEBUG_ASSERT(uniforms != NULL, "filling nothing with matrices");
	VOE_BASE_DEBUG_ASSERT(camera != NULL, "a frame with no camera in it");

	// The two cameras, and this is the only place either of them turns into
	// a view matrix. A flown camera is a point and a direction, so its
	// target is one step along where it looks; the orbit is a point and the
	// origin, which is what makes the still cube hold the centre of the
	// frame.
	if (camera->flying) {
		eye = camera->eye;
		target = voe_math_float3_add(
			eye, fly_forward(camera->yaw, camera->pitch));
	} else {
		eye = orbit_eye(seconds);
		target = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
	}

	uniforms->view = look_at(eye, target, up);
	uniforms->projection = voe_render_cube_projection(extent);
}

voe_math_float4x4 voe_render_cube_model(uint32_t index, float seconds)
{
	voe_math_float3 axis = { SPIN_AXIS_X, SPIN_AXIS_Y, SPIN_AXIS_Z };
	voe_math_float3 offset = { APART, 0.0f, 0.0f };

	VOE_BASE_DEBUG_ASSERT(index < VOE_RENDER_CUBE_COUNT,
			      "asking where a cube is that there is not");

	// Cube 0 is the still one, it is at the origin, and its matrix is the
	// identity rather than a translation by nothing — so that a still cube
	// is still by construction and not by arithmetic that happens to come
	// out to zero. It is what the camera looks at.
	if (index == 0)
		return voe_math_float4x4_identity();

	// Cube 1 turns on its own axis and then stands aside: composition reads
	// right to left, so the rotation is applied first and the translation
	// carries the already-turned cube out to +X. The other order would swing
	// it round the origin instead, which is an orbit and not a spin — and it
	// would look like a second camera motion, which is the one thing this
	// scene is arranged to avoid.
	return voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(offset),
		voe_math_float4x4_from_quat(voe_math_quat_from_axis_angle(
			axis, seconds * TURN / SPIN_SECONDS)));
}

// ------------------------------------------------------- the descriptor and

// One binding and one push constant range, and both stages named on the binding.
// The vertex stage is the one that transforms a position; the fragment stage is
// there because matrix_probe.slang reads this same buffer from a fragment
// shader, which is the only way a shader in this engine can report a value back
// to the CPU. A layout that named the vertex stage alone would make
// render/tests/matrix.c invalid rather than failing.
//
// The push constant range is the pipeline layout's and so it lives in device.c
// beside the layout it belongs to, not here.
static bool build_descriptor_layout(voe_render_device *device)
{
	VkDescriptorSetLayoutBinding binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
			      VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	VkDescriptorSetLayoutCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &binding,
	};
	VkDescriptorPoolSize size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = VOE_RENDER_FRAMES_IN_FLIGHT,
	};
	// Sized for exactly the sets that will ever be asked for, and without
	// FREE_DESCRIPTOR_SET: nothing frees one, the pool goes away whole at
	// shutdown, and a pool that cannot free is a pool that cannot be asked
	// to free by mistake.
	VkDescriptorPoolCreateInfo pool = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = VOE_RENDER_FRAMES_IN_FLIGHT,
		.poolSizeCount = 1,
		.pPoolSizes = &size,
	};
	VkResult result;

	result = voe_render_vk.create_descriptor_set_layout(
		device->device, &info, NULL, &device->descriptor_layout);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateDescriptorSetLayout failed (VkResult %d)\n",
			(int)result);
		device->descriptor_layout = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.create_descriptor_pool(device->device, &pool,
						      NULL,
						      &device->descriptor_pool);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateDescriptorPool failed (VkResult %d)\n",
			(int)result);
		device->descriptor_pool = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

// One uniform buffer and one descriptor set per frame slot, mapped for good and
// pointed at each other. Every slot's set is allocated in one call because the
// pool is sized for exactly this many and a partial allocation would leave it in
// a state nothing here knows how to describe.
static bool build_slots(voe_render_device *device)
{
	VkDescriptorSetLayout layouts[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkDescriptorSet sets[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = VOE_RENDER_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts,
	};
	VkResult result;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		layouts[i] = device->descriptor_layout;

	allocate.descriptorPool = device->descriptor_pool;
	result = voe_render_vk.allocate_descriptor_sets(device->device,
						       &allocate, sets);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkAllocateDescriptorSets failed (VkResult %d)\n",
			(int)result);
		return false;
	}

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];
		VkDescriptorBufferInfo buffer_info = {
			.offset = 0,
			.range = sizeof(struct voe_render_uniforms),
		};
		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &buffer_info,
		};

		if (!voe_render_buffer_build(
			    device, &frame->uniforms,
			    sizeof(struct voe_render_uniforms),
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		// Mapped once and never unmapped until teardown: written every
		// frame, so there is nothing to be gained by asking twice.
		result = voe_render_vk.map_memory(device->device,
						  frame->uniforms.memory, 0,
						  VK_WHOLE_SIZE, 0,
						  &frame->uniforms_mapped);
		if (result != VK_SUCCESS || frame->uniforms_mapped == NULL) {
			fprintf(stderr,
				"render: vkMapMemory failed on a uniform buffer (VkResult %d)\n",
				(int)result);
			return false;
		}

		frame->descriptor = sets[i];
		buffer_info.buffer = frame->uniforms.buffer;
		write.dstSet = frame->descriptor;
		voe_render_vk.update_descriptor_sets(device->device, 1, &write,
						     0, NULL);
	}

	return true;
}

// The geometry, in device-local memory, through a staging buffer. TRANSFER_DST
// as well as its real usage, because that is what the copy writes into.
static bool build_geometry(voe_render_device *device)
{
	if (!voe_render_buffer_build(device, &device->vertices,
				     sizeof(CUBE_VERTICES),
				     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
					     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;
	if (!voe_render_buffer_upload(device, &device->vertices, CUBE_VERTICES,
				      sizeof(CUBE_VERTICES)))
		return false;

	if (!voe_render_buffer_build(device, &device->indices,
				     sizeof(CUBE_INDICES),
				     VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;
	if (!voe_render_buffer_upload(device, &device->indices, CUBE_INDICES,
				      sizeof(CUBE_INDICES)))
		return false;

	// The draw's count, not the buffer's size. Set last, so that a device
	// whose geometry failed to build has a zero here rather than a count
	// pointing at a buffer that does not exist.
	device->index_count =
		(uint32_t)(sizeof(CUBE_INDICES) / sizeof(CUBE_INDICES[0]));
	return true;
}

bool voe_render_cube_build(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a cube for nothing");

	return build_descriptor_layout(device) && build_slots(device) &&
	       build_geometry(device);
}

void voe_render_cube_teardown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down a cube on nothing");

	if (device->device == VK_NULL_HANDLE)
		return;

	voe_render_buffer_teardown(device, &device->indices);
	voe_render_buffer_teardown(device, &device->vertices);
	device->index_count = 0;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];

		if (frame->uniforms_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->uniforms.memory);
			frame->uniforms_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->uniforms);
		// Not freed on its own: the pool below owns every set and was
		// created without FREE_DESCRIPTOR_SET.
		frame->descriptor = VK_NULL_HANDLE;
	}

	if (device->descriptor_pool != VK_NULL_HANDLE) {
		voe_render_vk.destroy_descriptor_pool(device->device,
						      device->descriptor_pool,
						      NULL);
		device->descriptor_pool = VK_NULL_HANDLE;
	}
	if (device->descriptor_layout != VK_NULL_HANDLE) {
		voe_render_vk.destroy_descriptor_set_layout(
			device->device, device->descriptor_layout, NULL);
		device->descriptor_layout = VK_NULL_HANDLE;
	}
}
