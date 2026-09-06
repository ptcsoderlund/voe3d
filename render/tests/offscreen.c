// BACK FACES ARE CULLED, AND THE Y FLIP, THE WINDING AND THE FRONT-FACE CONSTANT
// AGREE ABOUT WHICH WAY ROUND THAT IS. Three separate lines in three places say
// one thing between them — the negative viewport height in
// voe_render_frame_viewport, the counter-clockwise faces of the cube below, and
// VK_FRONT_FACE_COUNTER_CLOCKWISE beside VK_CULL_MODE_BACK_BIT in device.c — and
// none of them means anything alone. CLAUDE.md says flipping twice looks exactly
// like flipping none until something is culled; this is the test that notices.
//
// HOW A BACK FACE GETS IN FRONT OF THE RASTERISER WITHOUT A SECOND SHADER. The
// viewport is dynamic state, so the same cube drawn through the mirror of the
// engine's viewport has every face wound the other way round in framebuffer
// space: every face the engine would draw is culled and every face it would cull
// is drawn. That leaves the pipeline — the thing actually under test — untouched,
// and it means the front-facing half of this test runs the engine's own viewport
// function rather than a copy of it.
//
// THE CUBE IS THIS FILE'S OWN NOW, AND THAT IS WHAT CARD 018 CHANGED. It used to
// be render/src/cube.c's, drawn by a function that knew what the scene was; the
// scene left this folder with that card, so the geometry, the camera and the
// matrices are the test's and they go in through the same public calls `3d` uses.
// What is under test is therefore exactly what ships: the pools, the object
// records, the pipeline and the frame.
//
// IT HAS ITS OWN SUN NOW, AND IT IS TURNED ROUND FOR THE SECOND CASE. Card 019
// lights everything, and a surface facing away from the one light in the world
// is black — which would make every colour claim below read a black pixel and
// call it red, because "which channel is largest" has no answer for three zeroes.
// The mirrored case is looking at the far faces of the cube, whose normals point
// the opposite way to the near ones, so it gets the opposite sun. Each case
// therefore lights the faces it can actually see, and a light that failed to
// reach the shader at all fails the counts below rather than moving a centroid.
//
// AND THE MATERIAL IS THE PLAINEST ONE THAT KEEPS THE PICTURE'S COLOURS: no
// metalness, fully rough. A metal has no diffuse response and reflects its own
// colour in a narrow lobe, so a fully metallic cube is nearly black except where
// the highlight is — a perfectly good picture and a useless one to ask "is red
// above blue" of. What is under test here is geometry, not shading.
//
// IT USED TO BE A TRIANGLE AND THE CLAIM USED TO BE "NOTHING WAS DRAWN". A
// single triangle mirrored is culled entirely, so the old test could ask for an
// image identical to the clear. A cube mirrored is not empty — it is the same
// cube seen from the inside — so that claim could not survive the geometry
// changing and was replaced by the ones below rather than weakened.
//
// AND THE DEPTH TEST IS CHECKED BY THE CUBE BEING THERE AT ALL. Depth runs
// backwards here: cleared to 0, compared GREATER. Both of the ways to get that
// wrong reject every fragment rather than sorting them wrongly — LESS against a
// clear of 0, or GREATER against a clear of 1 — so an inverted depth test does
// not draw a confusing picture, it draws nothing. drawn_pixels() below is what
// says so, and it is the automated half of the card's "flip the comparison on
// purpose and confirm it looks wrong".
//
// IT RUNS HEADLESS AND THAT IS WHY IT CAN BE A TEST AT ALL. Drawing into an
// offscreen image needs no window, no compositor and no surface, so this runs
// under ctest on a machine with nothing but a driver.
//
// It includes render's internal header directly, by relative path, exactly as
// tests/loader.c does and for the same reason: reading a target back into memory
// is not something the engine does, so it is not part of render's public surface
// and must not become part of it so that a test can see it.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. There is no driver on a
// headless build box and that is not a broken checkout. The skip is a pass and
// it prints its reason.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

// Square, so that a mistake that swaps width for height cannot hide, and small,
// because every pixel of it is copied to the CPU and read.
#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)

// The two images land one after the other in one buffer. Front first, because
// the clear colour every other check is measured against is read out of it.
#define FRONT_OFFSET ((VkDeviceSize)0)
#define BACK_OFFSET IMAGE_BYTES

// The channel order of VK_FORMAT_B8G8R8A8_SRGB, which is the format a headless
// device takes and which the test checks it really got before reading a byte.
// Only the order matters here: every claim below is about which channel is
// largest, and an sRGB encoding does not turn red into green.
#define BLUE 0
#define GREEN 1
#define RED 2

// One cube of twenty-four vertices, one shading record, one drawn object per
// frame. Two frames in flight, so the second case draws into the other slot's
// target and neither has to wait for the other's readback.
static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
};

// The camera, written out here because the scene is this file's. The eye is
// above and behind the origin, which is where card 015's orbit started, so the
// picture is the one every sample point below was chosen against.
#define EYE_Y 1.8f
#define EYE_Z 4.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The one function this test needs that render's own code never calls. Reading
// an image back into memory is not something the engine does, so it is resolved
// here by hand rather than added to the table — putting it there would be adding
// surface for a test, which rule 10 is there to stop.
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

// THE PICTURE THIS FILE PUTS ON THE CUBE, AND EVERY PART OF IT IS LOAD-BEARING.
//
// RED TOP-LEFT, BLUE BOTTOM-LEFT, GREEN DOWN THE WHOLE RIGHT-HAND SIDE. Red
// against blue is the vertical question — which way up — and green against both
// is the horizontal one, which is what says whether the near face or the far one
// was drawn. There is no fourth colour, deliberately: yellow in the last
// quadrant would have a largest channel of red, and two quadrants that both read
// as red is a test that cannot tell them apart.
//
// It is four by four rather than two by two so that a blend between two texels
// never straddles the middle of the picture. Row zero is the top, which is what
// assets decodes and what Vulkan and glTF both mean by v = 0.
#define T_R { 220, 40, 40, 255 }
#define T_G { 40, 200, 40, 255 }
#define T_B { 40, 60, 220, 255 }
static const unsigned char QUADRANT_TEXTURE[4][4][4] = {
	{ T_R, T_R, T_G, T_G },
	{ T_R, T_R, T_G, T_G },
	{ T_B, T_B, T_G, T_G },
	{ T_B, T_B, T_G, T_G },
};

// THE CUBE: TWENTY-FOUR VERTICES, FOUR PER FACE, BECAUSE A CORNER SHARED BY
// THREE FACES NEEDS THREE TEXTURE COORDINATES. The same point is the top-left of
// one face and the bottom-right of another, and a vertex carries one of each
// attribute.
//
// EVERY FACE IS WOUND COUNTER-CLOCKWISE SEEN FROM OUTSIDE, which is what the
// pipeline's front-face constant means and half of what this file is testing.
//
// (0,0) IS THE TOP-LEFT OF THE PICTURE, so the first vertex of each face is its
// top-left corner and +u runs to the face's right.
#define H 0.5f
static const voe_render_vertex CUBE_VERTICES[24] = {
	// +Z
	{ { -H, H, H }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { H, H, H }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { H, -H, H }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, H }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	// -Z
	{ { H, H, -H }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
	{ { -H, H, -H }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, -H }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { H, -H, -H }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
	// +X
	{ { H, H, H }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, H, -H }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, -H, -H }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { H, -H, H }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	// -X
	{ { -H, H, -H }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { -H, H, H }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, H }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, -H }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	// +Y
	{ { -H, H, -H }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, H, -H }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, H, H }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, H, H }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
	// -Y
	{ { -H, -H, H }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, -H, H }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, -H, -H }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, -H }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
};

// Two triangles a face, and the order is what makes each one counter-clockwise
// from outside. Thirty-two bits, which is what the pool holds.
static const uint32_t CUBE_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	    // +Z
	7, 6, 5, 7, 5, 4,	    // -Z
	11, 10, 9, 11, 9, 8,	    // +X
	15, 14, 13, 15, 13, 12,	    // -X
	19, 18, 17, 19, 17, 16,	    // +Y
	23, 22, 21, 23, 21, 20,	    // -Y
};

// Which channel is the largest, which is all this file asks of a colour: the
// filtering, the format and the card all move the exact numbers about, and none
// of them turns red into green.
static uint32_t dominant(const unsigned char *pixel)
{
	if (pixel[RED] >= pixel[GREEN] && pixel[RED] >= pixel[BLUE])
		return RED;
	if (pixel[GREEN] >= pixel[BLUE])
		return GREEN;
	return BLUE;
}

// The mirror of voe_render_frame_viewport: y at the top and a positive height,
// which is what the engine would have if it did not flip. Written out rather
// than derived, so that it stays the mirror even if the real one changes shape.
static VkViewport mirrored_viewport(VkExtent2D extent)
{
	VkViewport viewport = {
		.y = 0.0f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	return viewport;
}

// The camera's two matrices, this file's own. They are written out rather than
// taken from `3d`, because `render` may not depend on `3d` and because what is
// under test here is the viewport and the pipeline — the matrices only have to
// put the cube in front of the camera.
static voe_render_view the_camera(VkExtent2D extent)
{
	voe_math_float3 eye = { 0.0f, EYE_Y, EYE_Z };
	voe_math_float3 forward = voe_math_float3_normalize(
		voe_math_float3_neg(eye));
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 z = voe_math_float3_neg(forward);
	voe_math_float3 x = voe_math_float3_normalize(
		voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_render_view view = { .eye = eye };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float aspect = (float)extent.width / (float)extent.height;
	float span = FAR_PLANE - NEAR_PLANE;

	view.view.m[0][0] = x.x;
	view.view.m[0][1] = x.y;
	view.view.m[0][2] = x.z;
	view.view.m[0][3] = -voe_math_float3_dot(x, eye);
	view.view.m[1][0] = y.x;
	view.view.m[1][1] = y.y;
	view.view.m[1][2] = y.z;
	view.view.m[1][3] = -voe_math_float3_dot(y, eye);
	view.view.m[2][0] = z.x;
	view.view.m[2][1] = z.y;
	view.view.m[2][2] = z.z;
	view.view.m[2][3] = -voe_math_float3_dot(z, eye);
	view.view.m[3][3] = 1.0f;

	// Reverse-Z, the same shape voe_3d_projection builds: near at 1.0, far
	// at 0.0, and no negated row anywhere — the viewport owns the flip.
	view.projection.m[0][0] = focal / aspect;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The sun, for one case. Its direction is the diagonal, so it lights the three
// faces whose normals point along +X, +Y and +Z — which are the ones the front
// case sees — and the mirrored case takes the opposite, which lights the three
// the far side of the cube shows it. A white light of one, because what is being
// measured is which channel is largest and any positive strength keeps that.
static voe_render_light the_sun(bool mirrored)
{
	// A third each, normalized: the light system does this for a world's
	// light and this file has no world.
	float d = mirrored ? 0.57735027f : -0.57735027f;
	voe_render_light sun = {
		.direction = { d, d, d },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};

	return sun;
}

// One case: a frame through the engine's own public calls, then the target
// copied into the buffer by a command buffer of this file's own.
//
// TWO SUBMITS AND AN IDLE BETWEEN THEM, WHICH IS THE SIMPLEST CORRECT THING. The
// frame's own submit is `render`'s and this file cannot add to it; waiting for
// the device to go idle before recording the copy is what makes the target
// finished and readable, and a test is the one place where waiting for idle
// costs nothing.
static void draw_case(voe_render_device *device, voe_render_geometry cube,
		      voe_render_shading shading, bool mirrored,
		      VkBuffer buffer, VkDeviceSize offset)
{
	const struct voe_render_frame *frame;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		// The cube is not scaled, so its normal matrix is the identity
		// as well — which is exactly the case where forgetting one
		// looks right. 3d/tests/normal_matrix.c is where the difference
		// is the claim.
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
	};
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commands = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkBufferImageCopy region = {
		.bufferOffset = offset,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { SIDE, SIDE, 1 },
	};
	// What makes the copy visible to a map below. The idle wait afterwards
	// is documented to do the same job, and saying it here as well costs one
	// barrier and removes the question.
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
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};
	bool drawing = false;

	// The slot being recorded, taken before _end because _end spends it and
	// moves on to the next one.
	frame = voe_render_frame_current(device);

	VOE_TEST_CHECK(voe_render_frame_begin(device, size,
					      the_camera(device->resolution),
					      the_sun(mirrored), &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;

	// The engine's own viewport was set by _begin; this replaces it with its
	// mirror for the second case, which is how a back face gets drawn.
	if (mirrored)
		voe_render_frame_set_viewport(device,
					      mirrored_viewport(device->resolution));

	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
	VOE_TEST_CHECK(voe_render_frame_end(device));

	voe_render_vk.device_wait_idle(device->device);

	VOE_TEST_CHECK_INT(voe_render_vk.allocate_command_buffers(device->device,
								  &allocate,
								  &commands),
			   VK_SUCCESS);
	if (commands == VK_NULL_HANDLE)
		return;

	voe_render_vk.begin_command_buffer(commands, &begin);
	// _end leaves the colour target in TRANSFER_SRC_OPTIMAL, which is
	// exactly the layout this copy wants.
	copy_image_to_buffer(commands, frame->target.colour.image,
			     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1,
			     &region);
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1, &submit,
						       VK_NULL_HANDLE),
			   VK_SUCCESS);
	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
}

static const unsigned char *pixel_at(const unsigned char *image, uint32_t x,
				     uint32_t y)
{
	return image + ((size_t)y * SIDE + (size_t)x) * 4;
}

static bool same_colour(const unsigned char *a, const unsigned char *b)
{
	return memcmp(a, b, 4) == 0;
}

// Where the pixels of one colour sit, on average, in the drawn part of an image.
//
// A CENTROID AND NOT A PIXEL AT A CHOSEN PLACE, BECAUSE THE CAMERA IS NOT SQUARE
// TO THE CUBE. The eye is above and behind the origin, so the near face is not a
// rectangle in the middle of the picture and the top face is visible above it —
// which means any fixed coordinate this file could name is one camera change
// away from landing on a different face and testing nothing. An average over
// every pixel of a colour has no such coordinate in it, and every visible face
// carries the picture the same way up, so the answer does not depend on which
// faces happen to be in view.
struct centroid {
	uint64_t x;
	uint64_t y;
	uint64_t count;
};

static struct centroid centroid_of(const unsigned char *image,
				   const unsigned char *clear, uint32_t channel)
{
	struct centroid found = { 0, 0, 0 };

	for (uint32_t y = 0; y < SIDE; y++) {
		for (uint32_t x = 0; x < SIDE; x++) {
			const unsigned char *pixel = pixel_at(image, x, y);

			if (same_colour(pixel, clear))
				continue;
			if (dominant(pixel) != channel)
				continue;

			found.x += x;
			found.y += y;
			found.count++;
		}
	}

	return found;
}

// Is a's mean smaller than b's? Cross-multiplied rather than divided, so that
// there is no rounding to argue about and no division by a count of zero — the
// callers check the counts separately, because "there were no red pixels at all"
// is a different failure from "they were in the wrong place".
static bool mean_less(uint64_t a_sum, uint64_t a_count, uint64_t b_sum,
		      uint64_t b_count)
{
	return a_sum * b_count < b_sum * a_count;
}

// How many pixels of an image are something other than the colour it was cleared
// to. What it is really measuring is that anything was drawn at all, which is
// what an inverted depth test takes away — see the note at the top.
static int drawn_pixels(const unsigned char *image, const unsigned char *clear)
{
	int count = 0;

	for (uint32_t y = 0; y < SIDE; y++) {
		for (uint32_t x = 0; x < SIDE; x++) {
			if (!same_colour(pixel_at(image, x, y), clear))
				count++;
		}
	}
	return count;
}

static void check_the_pictures(const unsigned char *pixels)
{
	// A corner of the front image, which is outside the cube whichever way
	// up it is. Taking the clear colour from the picture rather than from a
	// constant means frame.c is free to change it without touching this
	// file.
	const unsigned char *clear = pixel_at(pixels + FRONT_OFFSET, 1, 1);
	const unsigned char *front_centre = pixel_at(pixels + FRONT_OFFSET,
						     SIDE / 2, SIDE / 2);
	const unsigned char *back_centre = pixel_at(pixels + BACK_OFFSET,
						    SIDE / 2, SIDE / 2);
	struct centroid front_red = centroid_of(pixels + FRONT_OFFSET, clear,
						RED);
	struct centroid front_blue = centroid_of(pixels + FRONT_OFFSET, clear,
						 BLUE);
	struct centroid front_green = centroid_of(pixels + FRONT_OFFSET, clear,
						  GREEN);
	struct centroid back_red = centroid_of(pixels + BACK_OFFSET, clear, RED);
	struct centroid back_blue = centroid_of(pixels + BACK_OFFSET, clear,
						BLUE);
	struct centroid back_green = centroid_of(pixels + BACK_OFFSET, clear,
						 GREEN);

	// Both cases drew the cube over the centre. If either of these is the
	// clear colour then nothing was drawn there and every claim below would
	// pass or fail for the wrong reason.
	VOE_TEST_CHECK(!same_colour(front_centre, clear));
	VOE_TEST_CHECK(!same_colour(back_centre, clear));

	// All three colours reached the screen in both cases. Without this a
	// texture that failed to bind — every pixel the same — would pass every
	// comparison below by comparing nothing with nothing.
	VOE_TEST_CHECK(front_red.count > 0);
	VOE_TEST_CHECK(front_blue.count > 0);
	VOE_TEST_CHECK(front_green.count > 0);
	VOE_TEST_CHECK(back_red.count > 0);
	VOE_TEST_CHECK(back_blue.count > 0);
	VOE_TEST_CHECK(back_green.count > 0);

	// THE WHOLE CLAIM, AS TWO COMPARISONS PER IMAGE.
	//
	// Drawn the engine's way: the picture is the right way up, so its red
	// half is above its blue half on the screen; and the near face is the
	// one that survived culling, so the picture's green side is to the
	// right, which is where +u points on the +Z face.
	//
	// Drawn through the mirrored viewport, both answers invert, and they
	// invert for two different reasons that this file is deliberately
	// checking together. Red falls below blue because the viewport put
	// screen y the other way up — that is the flip itself. Green moves to
	// the left because the faces pointing at us are now wound the other way
	// and culled, so what is seen is the far face, whose +u points the
	// opposite way in the world. A mistake in only one of the two would move
	// one of these and not the other.
	VOE_TEST_CHECK(mean_less(front_red.y, front_red.count, front_blue.y,
				 front_blue.count));
	VOE_TEST_CHECK(mean_less(back_blue.y, back_blue.count, back_red.y,
				 back_red.count));
	VOE_TEST_CHECK(mean_less(front_red.x, front_red.count, front_green.x,
				 front_green.count));
	VOE_TEST_CHECK(mean_less(back_green.x, back_green.count, back_red.x,
				 back_red.count));

	// Anything at all was drawn — which is what an inverted depth
	// comparison takes away, because a clear of 0 with GREATER admits every
	// fragment and either mistake admits none.
	VOE_TEST_CHECK(drawn_pixels(pixels + FRONT_OFFSET, clear) > 0);
	VOE_TEST_CHECK(drawn_pixels(pixels + BACK_OFFSET, clear) > 0);

	// And it is a cube and not the whole image: back-face culling leaves the
	// clear colour everywhere outside the silhouette.
	VOE_TEST_CHECK(drawn_pixels(pixels + FRONT_OFFSET, clear) <
		       SIDE * SIDE);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry cube = { 0 };
	voe_render_texture texture = { 0 };
	voe_render_shading shading = { 0 };
	// No metalness and fully rough: the plainest surface there is, and the
	// only one whose colour on screen is still the picture's colour. See the
	// header.
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 1.0f,
	};
	// vkMapMemory hands back its pointer through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *mapped = NULL;

	// Two slots, two targets, so each case draws into its own image and
	// neither has to wait for the other's readback.
	VOE_TEST_CHECK(VOE_RENDER_FRAMES_IN_FLIGHT >= 2);
	if (VOE_RENDER_FRAMES_IN_FLIGHT < 2) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		// No Vulkan on the machine, or no card that meets what the
		// engine requires. Both are the build box and neither is this
		// engine being wrong, so they are a skip. Anything else is a
		// driver that refused something, and that is a failure.
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
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
	VOE_TEST_CHECK_INT(device->format.format, VK_FORMAT_B8G8R8A8_SRGB);
	VOE_TEST_CHECK_INT(device->resolution.width, SIDE);
	VOE_TEST_CHECK_INT(device->resolution.height, SIDE);

	if (!resolve_readback(device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// render's own buffer helper. Coherent as well as visible, so that
	// reading it after the idle wait needs no invalidate call.
	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, IMAGE_BYTES * 2,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// The cube, the picture and the record that ties them together, all
	// through the same public calls `3d` uses.
	VOE_TEST_CHECK(voe_render_geometry_create(device, CUBE_VERTICES, 24,
						  CUBE_INDICES, 36, &cube,
						  &error));
	VOE_TEST_CHECK(voe_render_texture_create(device,
						 VOE_RENDER_TEXTURE_COLOUR,
						 VOE_RENDER_SAMPLING_SMOOTH, 4,
						 4, &QUADRANT_TEXTURE[0][0][0],
						 &texture, &error));

	// The id is a real one and not slot zero's, which is the white default
	// nothing hands out. See ADR-0018 and voe_render_texture in
	// render/device.h.
	VOE_TEST_CHECK(texture.index > VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(texture.generation > 0);

	values.base_colour_texture = texture.index;
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));

	// A geometry id whose generation was never issued names nothing, and a
	// draw with it is refused rather than drawing whatever lives in the
	// slot. This is the whole reason an id has two halves and it costs one
	// line to prove — checked before any frame is open, because a refused
	// draw must not be the thing that leaves one half-recorded.
	{
		voe_render_geometry stale = {
			.index = cube.index,
			.generation = cube.generation + 1,
		};
		bool drawing = false;

		VOE_TEST_CHECK(voe_render_frame_begin(device, size,
						      the_camera(device->resolution),
						      the_sun(false), &drawing));
		VOE_TEST_CHECK(drawing);
		if (drawing) {
			VOE_TEST_CHECK(!voe_render_frame_draw(
				device, stale,
				(voe_render_object){
					.world = voe_math_float4x4_identity(),
					.normal = voe_math_float4x4_identity() }));
			VOE_TEST_CHECK(voe_render_frame_end(device));
			voe_render_vk.device_wait_idle(device->device);
		}
	}

	draw_case(device, cube, shading, false, readback.buffer, FRONT_OFFSET);
	draw_case(device, cube, shading, true, readback.buffer, BACK_OFFSET);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_the_pictures(mapped);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	} else {
		VOE_TEST_CHECK(mapped != NULL);
	}

	// Idle before anything the GPU touched goes away. The device's own
	// teardown does this for what it owns; the buffer is this file's.
	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
