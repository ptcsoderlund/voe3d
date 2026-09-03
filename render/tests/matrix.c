// THREE CLAIMS ABOUT MATRICES, AND ONLY ONE OF THEM NEEDS A GRAPHICS CARD.
//
// The first is that this engine's depth really does run backwards, and it is
// checked on the CPU: voe_render_cube_projection is arithmetic, so a test can
// read the matrix it returns and multiply points through it without a driver
// anywhere. Near plane at 1.0, far plane approached at 0.0, nearer means
// greater, and no negated Y. That half runs on every machine, build box
// included, and it is the half that catches somebody "correcting" the
// projection to the shape every tutorial writes.
//
// The second is that slangc really was invoked with -matrix-layout-row-major,
// and that one cannot be checked without running a shader. A known matrix goes
// into the engine's own uniform buffer through the engine's own descriptor, and
// matrix_probe.slang reports three of its elements back as colour. With the flag
// the shader reads the elements C wrote at [0][3], [1][3] and [2][3]; without it
// it reads the ones C wrote at [3][0], [3][1] and [3][2], because the same
// sixteen floats mean something else. The matrix below is filled so that all six
// of those are different numbers, far apart, so the answer names the layout
// rather than merely differing from it.
//
// The third is that the scene cube.c describes is the scene it says it is, and
// it is checked on the CPU for the same reason the first is: an orbit and two
// model matrices are arithmetic. The camera keeps its distance and its height
// and comes back round; it looks at what it aims at, so the origin lands in the
// middle of the frame from every point on the orbit; the still cube is still to
// the last bit; and no corner of either cube ever crosses the near plane, which
// is the card's "nothing clipped when the camera passes close" as a number
// instead of a look. That last one is the one a person cannot check by eye
// reliably at all — a clipped corner at the edge of a fast orbit is one frame.
//
// WHY THE FLAG IS WORTH A TEST AT ALL. Remove it and nothing fails to compile,
// nothing warns, and every transform in the engine comes out transposed. That is
// the whole class of bug this file exists for, and CLAUDE.md and float4x4.h both
// say so in words — this is the same statement in a form that runs.
//
// IT INCLUDES render's INTERNAL HEADER BY RELATIVE PATH, exactly as
// tests/offscreen.c and tests/loader.c do and for the same reason: a headless
// device, the probe and the frame slots are not render's public surface and must
// not become part of it so that a test can see them.
//
// A MACHINE WITH NO USABLE VULKAN STILL RUNS THE FIRST HALF AND SKIPS THE
// SECOND, AND SAYS SO. There is no driver on a headless build box and that is
// not a broken checkout.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

// Small and square: every pixel of it is copied to the CPU, and the probe covers
// all of them, so one sample is as good as all of them. Square so that a
// mistake swapping width for height cannot hide.
#define SIDE 16
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)

// The channel order of VK_FORMAT_B8G8R8A8_UNORM, which is the format a headless
// device takes and which this test checks it really got before reading a byte.
#define BLUE 0
#define GREEN 1
#define RED 2

// Element (row, column) of the probe matrix is (row * 4 + column) / 16, so every
// one of the sixteen is a different value and flat memory float k holds k / 16.
// That makes the two layouts' answers arithmetic rather than a lookup:
//
//   row-major     m[0][3] m[1][3] m[2][3] are floats 3, 7, 11
//   column-major  the same expressions are floats 12, 13, 14
//
// Both sets are inside 0..1 so they survive an unorm colour, and the two are a
// hundred byte values apart so no rounding mode can turn one into the other.
#define PROBE_ELEMENT(row, column) ((float)((row) * 4 + (column)) / 16.0f)

// What the three channels must read, as bytes, if the flag was in force. 255 *
// 3/16, 7/16 and 11/16.
#define EXPECTED_RED 48
#define EXPECTED_GREEN 112
#define EXPECTED_BLUE 175

// What they would read without it — never asserted as a pass, only used to say
// so in the failure message, because "48 but got 191" is a diagnosis and "48 but
// got something else" is a puzzle.
#define COLUMN_MAJOR_RED 191

// A byte or two of slack. The float-to-unorm conversion is round-to-nearest but
// the exact tie-breaking is the implementation's, and the two layouts differ by
// far more than this.
#define TOLERANCE 3

// How many points of the orbit the scene checks are sampled. Prime, so that the
// samples do not land on the quarter turns a mistake is most likely to survive:
// a view matrix that is only right on the axes is a view matrix that is wrong,
// and 97 samples of a twelve-second orbit miss every one of them.
#define ORBIT_SAMPLES 97

// The cube's half side, in metres, which is cube.c's HALF and is stated here
// rather than shared. It is the one number this file has to know about the
// geometry, the claim below is what happens at its corners, and a cube that
// stopped being a metre across would want this line looked at rather than
// silently followed.
#define CUBE_HALF 0.5f

// Where the still cube is required to stay, and how far the frame's centre may
// be from the origin's projection. Both are in the units of what they measure —
// metres and normalized device coordinates — and both are tight because they are
// claims about exactness rather than about accuracy: the still cube's matrix is
// the identity, and the look-at points at its target.
#define STILL_TOLERANCE 0.0f
#define CENTRE_TOLERANCE 1e-5f

// ------------------------------------------------ the half with no graphics card

// The depth the rasteriser would see for a point this far in front of the
// camera, which is clip z over clip w. Distance is positive and the camera looks
// along its own -Z, so the point is at view-space z = -distance.
static float depth_at(voe_math_float4x4 projection, float distance)
{
	voe_math_float4 point = { 0.0f, 0.0f, -distance, 1.0f };
	voe_math_float4 clip = voe_math_float4x4_mul_float4(projection, point);

	VOE_TEST_CHECK(clip.w != 0.0f);
	if (clip.w == 0.0f)
		return 0.0f;
	return clip.z / clip.w;
}

static void check_projection(void)
{
	VkExtent2D extent = { 800, 600 };
	voe_math_float4x4 projection = voe_render_cube_projection(extent);
	// The distance at which depth is exactly 1, which is the near plane.
	// Read out of the matrix rather than assumed, because the near plane is
	// cube.c's constant and not this file's business — what is under test is
	// the shape of the matrix, not the number in it.
	float near_plane = projection.m[2][3];

	// w = -z, which is what makes the perspective divide a divide by
	// distance. Without this the two checks below say nothing.
	VOE_TEST_CHECK_FLOAT(projection.m[3][2], -1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(projection.m[3][3], 0.0f, 0.0f);

	// THE FAR PLANE IS AT INFINITY, AND THIS IS ITS SIGNATURE. clip.z is the
	// near plane for every vertex, with no term in z at all, so depth is
	// near / distance and reaches 0 only in the limit. A finite far plane
	// would put a non-zero number here.
	VOE_TEST_CHECK_FLOAT(projection.m[2][2], 0.0f, 0.0f);
	VOE_TEST_CHECK(near_plane > 0.0f);

	// NOTHING IN THE PROJECTION NEGATES Y. The engine's one Y flip is the
	// negative viewport height in frame.c; a minus sign here as well would
	// flip twice, which is invisible until something is culled. Both scales
	// are positive.
	VOE_TEST_CHECK(projection.m[0][0] > 0.0f);
	VOE_TEST_CHECK(projection.m[1][1] > 0.0f);

	// A wider target than it is tall divides x by the aspect ratio, so the
	// horizontal scale is the smaller of the two. This is what catches an
	// aspect correction applied to the wrong axis, which stretches the
	// picture without ever failing.
	VOE_TEST_CHECK(projection.m[0][0] < projection.m[1][1]);

	// DEPTH RUNS BACKWARDS: the near plane is 1, not 0.
	VOE_TEST_CHECK_FLOAT(depth_at(projection, near_plane), 1.0f, 0.0001f);

	// And nearer is greater, all the way out. Three distances rather than
	// two, because two of them could pass on a matrix that is merely
	// constant.
	VOE_TEST_CHECK(depth_at(projection, near_plane * 2.0f) <
		       depth_at(projection, near_plane));
	VOE_TEST_CHECK(depth_at(projection, near_plane * 100.0f) <
		       depth_at(projection, near_plane * 2.0f));

	// The far plane is approached and never reached, and it is 0 rather than
	// 1. A long way out is nearly zero and still positive.
	VOE_TEST_CHECK(depth_at(projection, near_plane * 100000.0f) > 0.0f);
	VOE_TEST_CHECK(depth_at(projection, near_plane * 100000.0f) < 0.001f);

	// Everything in front of the near plane is inside 0..1, which is the
	// range a depth buffer stores and the range the viewport maps onto.
	VOE_TEST_CHECK(depth_at(projection, near_plane * 3.0f) > 0.0f);
	VOE_TEST_CHECK(depth_at(projection, near_plane * 3.0f) < 1.0f);
}

// ------------------------------------------------- the scene, with no card either

// The eight corners of a cube, in its own space. The order does not matter: what
// is asked below is a minimum over all of them.
static voe_math_float3 cube_corner(int corner)
{
	voe_math_float3 c;

	c.x = (corner & 1) ? CUBE_HALF : -CUBE_HALF;
	c.y = (corner & 2) ? CUBE_HALF : -CUBE_HALF;
	c.z = (corner & 4) ? CUBE_HALF : -CUBE_HALF;
	return c;
}

// How far in front of the camera a point is. The camera looks along its own -Z,
// so a point in view space is in front of it when its z is negative, and the
// distance is minus that z. Anything the near plane clips reads zero or less
// here, which is why one number covers both "behind the camera" and "too close".
static float distance_in_front(voe_math_float4x4 view, voe_math_float4x4 model,
			       voe_math_float3 point)
{
	voe_math_float3 world = voe_math_float4x4_transform_point(model, point);

	return -voe_math_float4x4_transform_point(view, world).z;
}

// THE ORBIT IS A CIRCLE, IT LOOKS AT WHAT IT ORBITS, AND IT NEVER GETS CLOSE
// ENOUGH TO CLIP ANYTHING. Three claims over the whole orbit rather than at a
// handful of angles, because every one of them is exactly true at zero seconds —
// which is the one moment every other test in this folder sees.
static void check_scene(void)
{
	// A window shape rather than a square, so that an aspect ratio applied
	// to the wrong axis would move the centre off in one direction. The
	// numbers below are angles and distances and none of them depends on
	// this.
	VkExtent2D extent = { 1280, 720 };
	voe_math_float4x4 projection = voe_render_cube_projection(extent);
	float near_plane = projection.m[2][3];
	// The orbit's own radius and height, taken from the first sample rather
	// than from a constant this file would then have to keep in step with
	// cube.c. What is under test is that they do not change, not what they
	// are.
	float radius = 0.0f;
	float height = 0.0f;
	float closest = 0.0f;
	voe_math_float3 centre = { 0.0f, 0.0f, 0.0f };

	VOE_TEST_CHECK(near_plane > 0.0f);

	for (int sample = 0; sample < ORBIT_SAMPLES; sample++) {
		// One second per sample, and nothing here knows cube.c's
		// period. Ninety-seven seconds is several turns of any orbit a
		// person would sit and watch, and the claims below hold at every
		// moment rather than at chosen angles — which is what makes not
		// knowing the period the right way round.
		float seconds = (float)sample;
		struct voe_render_uniforms uniforms;
		voe_math_float3 origin = { 0.0f, 0.0f, 0.0f };
		voe_math_float3 eye;
		voe_math_float4 clip;
		voe_math_float4x4 still;
		voe_math_float4x4 identity;
		voe_math_float4x4 turning;

		voe_render_cube_uniforms_fill(&uniforms, extent, seconds);

		// Where the camera is, recovered from the view matrix: the view
		// matrix takes the eye to the origin of view space, so the
		// inverse takes the origin of view space back to the eye.
		eye = voe_math_float4x4_transform_point(
			voe_math_float4x4_inverse(uniforms.view), origin);

		// A CIRCLE ABOUT +Y, WHICH IS TWO NUMBERS THAT DO NOT MOVE. An
		// orbit that drifted in or out, or up and down, would still look
		// like an orbit.
		//
		// Both get a tolerance and neither can be exact, because the eye
		// came back through a general 4x4 inverse: a tenth of a
		// millimetre is a couple of parts in ten million of four metres,
		// which is float arithmetic and not a drift. A real drift over
		// ninety-seven samples is orders of magnitude past this.
		if (sample == 0) {
			radius = sqrtf(eye.x * eye.x + eye.z * eye.z);
			height = eye.y;
			VOE_TEST_CHECK(radius > 0.0f);
		} else {
			VOE_TEST_CHECK_FLOAT(sqrtf(eye.x * eye.x +
						   eye.z * eye.z),
					     radius, 1e-4f);
			VOE_TEST_CHECK_FLOAT(eye.y, height, 1e-4f);
		}

		// IT LOOKS AT WHAT IT ORBITS. The origin goes through the view
		// and the projection and comes out in the middle of the frame,
		// from every angle. This is the one check that would catch a
		// look-at whose up vector or whose sign of forward was wrong
		// without the picture going blank.
		clip = voe_math_float4x4_mul_float4(
			projection,
			voe_math_float4x4_mul_float4(
				uniforms.view,
				(voe_math_float4){ 0.0f, 0.0f, 0.0f, 1.0f }));
		VOE_TEST_CHECK(clip.w > 0.0f);
		if (clip.w > 0.0f) {
			VOE_TEST_CHECK_FLOAT(clip.x / clip.w, 0.0f,
					     CENTRE_TOLERANCE);
			VOE_TEST_CHECK_FLOAT(clip.y / clip.w, 0.0f,
					     CENTRE_TOLERANCE);
		}

		// THE STILL CUBE IS STILL, EXACTLY. Not nearly: cube 0's matrix
		// is the identity at every moment, so a corner of it is where it
		// was, to the bit. The card's whole arrangement rests on one of
		// the two objects not moving, and "nearly still" is a slow drift
		// nobody would see until the cube had left.
		still = voe_render_cube_model(0, seconds);
		identity = voe_math_float4x4_identity();
		for (int row = 0; row < 4; row++) {
			for (int column = 0; column < 4; column++)
				VOE_TEST_CHECK_FLOAT(still.m[row][column],
						     identity.m[row][column],
						     STILL_TOLERANCE);
		}

		// AND THE OTHER ONE TURNS WITHOUT GOING ANYWHERE, WHICH IS WHAT
		// "on its own axis" MEANS. Its translation column never moves,
		// so what changes about it is a rotation and nothing else — the
		// mistake this rules out is composing the translation and the
		// rotation the other way round, which swings the cube round the
		// origin and reads as a second camera.
		turning = voe_render_cube_model(1, seconds);
		if (sample == 0) {
			centre = (voe_math_float3){ turning.m[0][3],
						    turning.m[1][3],
						    turning.m[2][3] };
			// Beside the first cube and not on top of it, or there
			// would be one silhouette and nothing to see.
			VOE_TEST_CHECK(voe_math_float3_length(centre) >
				       CUBE_HALF * 2.0f);
		} else {
			VOE_TEST_CHECK_FLOAT(turning.m[0][3], centre.x, 0.0f);
			VOE_TEST_CHECK_FLOAT(turning.m[1][3], centre.y, 0.0f);
			VOE_TEST_CHECK_FLOAT(turning.m[2][3], centre.z, 0.0f);
		}

		// NOTHING CROSSES THE NEAR PLANE, AND THAT IS EVERY CORNER OF
		// EVERY CUBE. The closest one anywhere on the orbit is kept, so
		// the failure message says how close it actually came rather
		// than only that it was too close.
		for (uint32_t cube = 0; cube < VOE_RENDER_CUBE_COUNT; cube++) {
			voe_math_float4x4 model =
				voe_render_cube_model(cube, seconds);
			int corner;

			for (corner = 0; corner < 8; corner++) {
				float distance = distance_in_front(
					uniforms.view, model,
					cube_corner(corner));

				if ((sample == 0 && cube == 0 && corner == 0) ||
				    distance < closest)
					closest = distance;
			}
		}
	}

	// A whole near plane of margin and not a hair of it: the claim is that
	// the orbit was chosen so that nothing can clip, not that it happens not
	// to on the numbers of the day.
	VOE_TEST_CHECK(closest > near_plane * 2.0f);
}

// --------------------------------------------------------- the half that draws

// The one function this test needs that render's own code never calls. Reading
// an image back into memory is not something the engine does, so it is resolved
// here by hand rather than added to the table — the same reasoning, and the same
// shape, as render/tests/offscreen.c.
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");

	// Core Vulkan 1.0, so a driver without it is broken rather than merely
	// old — but a test that called through a null pointer would die where it
	// is supposed to report, and the analyser is right to say so.
	return copy_image_to_buffer != NULL;
}

// The matrix under test, written straight into the slot's mapped uniform buffer.
// The projection member is left as it is: the probe reads view and nothing else,
// and filling the other would suggest it mattered.
static void write_probe_matrix(const struct voe_render_frame *frame)
{
	struct voe_render_uniforms *uniforms = frame->uniforms_mapped;

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			uniforms->view.m[row][column] =
				PROBE_ELEMENT(row, column);
	}
}

static void record_probe(voe_render_device *device,
			 const struct voe_render_frame *frame,
			 VkPipeline pipeline, VkBuffer buffer)
{
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { SIDE, SIDE, 1 },
	};
	// What makes the copy visible to a map below. The fence this submit
	// signals is documented to do the same job, and saying it here as well
	// costs one barrier and removes the question.
	VkMemoryBarrier2 visible = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &visible,
	};

	voe_render_vk.begin_command_buffer(frame->commands, &begin);

	// The probe leaves the colour target in TRANSFER_SRC_OPTIMAL, which is
	// exactly the layout this copy wants.
	voe_render_probe_draw(device, frame, pipeline,
			      voe_render_frame_viewport(device->resolution));

	copy_image_to_buffer(frame->commands, frame->target.colour.image,
			     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1,
			     &region);
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	voe_render_vk.end_command_buffer(frame->commands);
}

// The middle of the image. The probe covers every pixel, so this is a sample and
// not a search.
static void check_reported_matrix(const unsigned char *pixels)
{
	const unsigned char *pixel =
		pixels + ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;

	if (pixel[RED] > COLUMN_MAJOR_RED - TOLERANCE &&
	    pixel[RED] < COLUMN_MAJOR_RED + TOLERANCE) {
		// Not a second assertion, a diagnosis: this is the exact value
		// the other layout produces, so the flag is the thing to look
		// at and not the shader.
		printf("the shader read this matrix column-major: slangc was invoked without -matrix-layout-row-major\n");
	}

	// Through the float check because voe::testing has no near-integer one
	// and adding it would be editing another folder. The values are byte
	// counts and the tolerance is in bytes; the message reads correctly
	// either way, which is what that macro is for.
	VOE_TEST_CHECK_FLOAT((float)pixel[RED], (float)EXPECTED_RED,
			     (float)TOLERANCE);
	VOE_TEST_CHECK_FLOAT((float)pixel[GREEN], (float)EXPECTED_GREEN,
			     (float)TOLERANCE);
	VOE_TEST_CHECK_FLOAT((float)pixel[BLUE], (float)EXPECTED_BLUE,
			     (float)TOLERANCE);
}

int main(void)
{
	voe_base_arena *arena;
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_frame *frame;
	struct voe_render_buffer readback = { 0 };
	VkPipeline pipeline;
	VkCommandBufferSubmitInfo commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commands,
	};
	// vkMapMemory hands back its pointer through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *mapped = NULL;

	// First, and with no device at all: the projection is arithmetic.
	check_projection();
	check_scene();

	arena = voe_base_arena_new(64 * 1024);
	device = voe_render_device_new_headless(arena, size, &error);
	if (device == NULL) {
		// No Vulkan on the machine, or no card that meets what the
		// engine requires. Both are the build box and neither is this
		// engine being wrong, so they are a skip — and the projection
		// half above has already run and counted.
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: the shader half needs a driver: %s\n",
			       voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// Everything below reads bytes in this order. A headless device asks no
	// surface and takes this format outright, so this is a claim about
	// device.c and not about the machine.
	VOE_TEST_CHECK_INT(device->format.format, VK_FORMAT_B8G8R8A8_UNORM);
	VOE_TEST_CHECK_INT(device->resolution.width, SIDE);

	frame = &device->frames[0];

	if (!resolve_readback(device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// render's own buffer helper, host-visible and coherent so that reading
	// it after the fence needs no invalidate.
	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, IMAGE_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	pipeline = voe_render_probe_pipeline_new(device);
	VOE_TEST_CHECK(pipeline != VK_NULL_HANDLE);

	if (readback.buffer != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE) {
		write_probe_matrix(frame);
		record_probe(device, frame, pipeline, readback.buffer);

		commands.commandBuffer = frame->commands;

		// The slot's own fence, reset first because device.c makes them
		// signalled so that a first frame has something to wait on.
		voe_render_vk.reset_fences(device->device, 1,
					   &frame->submitted);
		VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1,
							       &submit,
							       frame->submitted),
				   VK_SUCCESS);
		VOE_TEST_CHECK_INT(voe_render_vk.wait_for_fences(device->device,
								 1,
								 &frame->submitted,
								 VK_TRUE,
								 UINT64_MAX),
				   VK_SUCCESS);

		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		if (mapped != NULL) {
			check_reported_matrix(mapped);
			voe_render_vk.unmap_memory(device->device,
						   readback.memory);
		} else {
			VOE_TEST_CHECK(mapped != NULL);
		}
	}

	// Idle before anything the GPU touched goes away. The device's own
	// teardown does this for what it owns; these two are this file's.
	voe_render_vk.device_wait_idle(device->device);
	if (pipeline != VK_NULL_HANDLE)
		voe_render_vk.destroy_pipeline(device->device, pipeline, NULL);
	voe_render_buffer_teardown(device, &readback);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
