// Geometry that lives one frame: built inside a frame, drawn in it, and refused
// by the frame after. Three claims, each of them the one a wrong implementation
// would get wrong quietly.
//
// THE ID DIES WITH ITS FRAME, AND THE SLOT DRAWS SOMETHING ELSE NEXT FRAME. A
// range built in frame one is drawn and lands in the picture; the same id used
// in frame two is refused by the draw, and a range built in frame two at the
// same slot draws its own contents and not frame one's. That second half is the
// part worth reading a picture for: a reset that forgot to empty the pool, or a
// create that wrote at the wrong offset, would draw the right slot with last
// frame's bytes and every bookkeeping check would still pass.
//
// BOTH POOLS IN ONE FRAME, WHICH IS THE REBIND. A static range and a transient
// range are two different vertex buffers, and a frame binds one pair of pools at
// its top. Drawing static, then transient, then static again is what makes the
// draw rebind twice; if either bind is missing, one of the ranges is drawn out of
// the other's buffer and wears its shape — which here means the wrong half of the
// picture is the wrong colour.
//
// OVERRUNNING A TRANSIENT POOL IS A RETURNED FAILURE AND CORRUPTS NOTHING. The
// range that did fit still draws in the frame that refused, and the frame after
// it draws correctly — the specific failure this guards against is a refusal that
// leaves a pool's counters or a slot half written, which is a program that works
// until the frame it first runs out of room.
//
// IDENTITY CAMERA, UNLIT RECORDS. The view and the projection are both the
// identity, so a vertex is already in clip space and a quad from x = -1 to x = 0
// is exactly the left half of the target. The records are unlit so a pixel's
// colour is the record's base colour and nothing about a sun has to be right for
// a count to mean something. Depth is 0.5 everywhere: above the clear of 0,
// under the GREATER test this engine runs.
//
// It includes render's internal header by relative path, as tests/pools.c and
// tests/offscreen.c do: reading a target back is not something the engine does
// and must not become part of its surface so that a test can see it.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SIDE 16
#define IMAGE_BYTES (SIDE * SIDE * 4)

// Room for exactly one static quad and, per frame, one transient quad, so that
// the second transient create in a frame is the overrun.
#define QUAD_VERTICES 4
#define QUAD_INDICES 6

static const voe_render_capacities CAPACITIES = {
	.vertices = QUAD_VERTICES,
	.indices = QUAD_INDICES,
	.geometries = 1,
	.objects = 4,
	.shadings = 2,
	.passes = 1,
	.transient_vertices = QUAD_VERTICES,
	.transient_indices = QUAD_INDICES,
	.transient_geometries = 1,
};

// Two records: one red, one green, both unlit, both reading the white default
// texture over the whole of itself so the colour on screen is the factor.
static const voe_render_shading_values RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};
static const voe_render_shading_values GREEN = {
	.base_colour = { 0.0f, 1.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// The one function this test needs that render's own code never calls, resolved
// by hand for the reason tests/offscreen.c gives: reading an image back is a
// test's business and not surface to add to the table.
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	return copy_image_to_buffer != NULL;
}

// A quad from x0 to x1 across the whole height, facing the camera, at a depth
// that passes. Corner for corner and index for index the shape dev/src/quad.c
// uses, which is the winding the culling test already proves is a front face.
static void quad(float x0, float x1, voe_render_vertex vertices[4],
		 uint32_t indices[6])
{
	vertices[0] = (voe_render_vertex){ { x0, 1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 0.0f } };
	vertices[1] = (voe_render_vertex){ { x1, 1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 0.0f } };
	vertices[2] = (voe_render_vertex){ { x1, -1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 1.0f } };
	vertices[3] = (voe_render_vertex){ { x0, -1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 1.0f } };
	indices[0] = 3;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 3;
	indices[4] = 1;
	indices[5] = 0;
}

static voe_render_view identity_camera(void)
{
	return (voe_render_view){
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
		.eye = { 0.0f, 0.0f, 0.0f },
	};
}

// Any light: the records are unlit and never read it.
static voe_render_light no_sun(void)
{
	return (voe_render_light){
		.direction = { 0.0f, -1.0f, 0.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
}

static voe_render_object wearing(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// Copies the slot's finished target into `buffer`, by a command buffer of this
// file's own, after waiting for the device to go idle — the shape
// tests/offscreen.c uses, for the reasons its header gives.
static void read_back(voe_render_device *device,
		      const struct voe_render_frame *frame, VkBuffer buffer)
{
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
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { SIDE, SIDE, 1 },
	};
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

	voe_render_vk.device_wait_idle(device->device);

	VOE_TEST_CHECK_INT(voe_render_vk.allocate_command_buffers(device->device,
								  &allocate,
								  &commands),
			   VK_SUCCESS);
	if (commands == VK_NULL_HANDLE)
		return;

	voe_render_vk.begin_command_buffer(commands, &begin);
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

// Which of red and green a pixel mostly is, or neither: the clear colour is a
// dark blue and has no red or green worth counting.
#define NEITHER 0
#define IS_RED 1
#define IS_GREEN 2

// BGRA, which is the headless device's format — main() asserts it.
static int colour_of(const unsigned char *pixel)
{
	unsigned char blue = pixel[0];
	unsigned char green = pixel[1];
	unsigned char red = pixel[2];

	if (red > 128 && red > green && red > blue)
		return IS_RED;
	if (green > 128 && green > red && green > blue)
		return IS_GREEN;
	return NEITHER;
}

// How many pixels of the given colour in the columns [x0, x1).
static int count_in_columns(const unsigned char *image, int x0, int x1,
			    int colour)
{
	int count = 0;

	for (int y = 0; y < SIDE; y++)
		for (int x = x0; x < x1; x++)
			if (colour_of(image + (y * SIDE + x) * 4) == colour)
				count++;
	return count;
}

// The whole left or right half, and nothing in the other. The halves are the
// two quads' exact footprints, so the counts are exact and not a threshold.
#define HALF (SIDE * SIDE / 2)

static void expect_halves(const unsigned char *image, int left, int right)
{
	VOE_TEST_CHECK_INT(count_in_columns(image, 0, SIDE / 2, IS_RED),
			   left == IS_RED ? HALF : 0);
	VOE_TEST_CHECK_INT(count_in_columns(image, 0, SIDE / 2, IS_GREEN),
			   left == IS_GREEN ? HALF : 0);
	VOE_TEST_CHECK_INT(count_in_columns(image, SIDE / 2, SIDE, IS_RED),
			   right == IS_RED ? HALF : 0);
	VOE_TEST_CHECK_INT(count_in_columns(image, SIDE / 2, SIDE, IS_GREEN),
			   right == IS_GREEN ? HALF : 0);
}

struct scene {
	voe_render_device *device;
	voe_render_shading red;
	voe_render_shading green;
	// A static quad over the left half.
	voe_render_geometry left;
	struct voe_render_buffer readback;
	// vkMapMemory hands its pointer back through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *pixels;
};

// A frame and one pass onto the window, which is where every draw here goes.
static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { .view = identity_camera(),
					  .light = no_sun() };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	return true;
}

// The pass open_frame opened, and then the frame.
static bool close_frame(voe_render_device *device)
{
	voe_render_pass_end(device);
	return voe_render_frame_end(device);
}

// A transient quad over the right half, and the id that names it this frame.
static bool build_right(voe_render_device *device, voe_render_geometry *out,
			voe_base_error *error)
{
	voe_render_vertex vertices[4];
	uint32_t indices[6];

	quad(0.0f, 1.0f, vertices, indices);
	return voe_render_geometry_create_transient(device, vertices, 4,
						    indices, 6, out, error);
}

// Frame one: a transient right half, green. Frame two: the same id is refused,
// and a new transient range at the same slot draws red over the right half —
// the picture having changed is what proves the pool was emptied and rewritten.
static void an_id_lives_one_frame(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_geometry first = { 0 };
	voe_render_geometry second = { 0 };
	voe_base_error error = VOE_BASE_OK;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(build_right(device, &first, &error));
	VOE_TEST_CHECK(first.generation > 0);
	VOE_TEST_CHECK(first.index >= CAPACITIES.geometries);
	VOE_TEST_CHECK(voe_render_frame_draw(device, first,
					     wearing(scene->green)));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, NEITHER, IS_GREEN);

	// The next frame's begin is what makes last frame's id stale, and the
	// draw is what refuses it — before anything is built this frame, so
	// nothing new is living in the slot yet when it does.
	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_geometry_at(device, first) == NULL);
	VOE_TEST_CHECK(!voe_render_frame_draw(device, first,
					      wearing(scene->green)));

	VOE_TEST_CHECK(build_right(device, &second, &error));
	VOE_TEST_CHECK_INT(second.index, first.index);
	VOE_TEST_CHECK(second.generation != first.generation);
	VOE_TEST_CHECK(voe_render_frame_draw(device, second,
					     wearing(scene->red)));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, NEITHER, IS_RED);
}

// Static left half in red, transient right half in green, drawn static then
// transient then static again — two rebinds in one frame, and each half has to
// come out its own colour.
static void both_pools_draw_in_one_frame(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_geometry right = { 0 };
	voe_base_error error = VOE_BASE_OK;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->left,
					     wearing(scene->red)));
	VOE_TEST_CHECK(build_right(device, &right, &error));
	VOE_TEST_CHECK(voe_render_frame_draw(device, right,
					     wearing(scene->green)));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->left,
					     wearing(scene->red)));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, IS_RED, IS_GREEN);
}

// The pool holds one quad per frame. The second create is refused with the
// error the static create uses, the first still draws, and the frame after it
// is an ordinary frame again.
static void overrunning_is_refused_and_the_next_frame_is_fine(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_geometry right = { 0 };
	voe_render_geometry refused = { 0 };
	voe_base_error error = VOE_BASE_OK;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(build_right(device, &right, &error));
	VOE_TEST_CHECK(!build_right(device, &refused, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
	// The range that fitted is still there and still draws.
	VOE_TEST_CHECK(voe_render_geometry_at(device, right) != NULL);
	VOE_TEST_CHECK(voe_render_frame_draw(device, right,
					     wearing(scene->green)));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, NEITHER, IS_GREEN);

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(build_right(device, &right, &error));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->left,
					     wearing(scene->red)));
	VOE_TEST_CHECK(voe_render_frame_draw(device, right,
					     wearing(scene->red)));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, IS_RED, IS_RED);
}

// A device opened with no transient room refuses the create rather than
// asserting, and says which capacity was nought.
static void no_transient_room_is_a_refusal(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_device *device;
	voe_render_geometry geometry = { 0 };
	voe_base_error error = VOE_BASE_OK;

	room.transient_vertices = 0;
	room.transient_indices = 0;
	room.transient_geometries = 0;

	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL)
		return;

	if (open_frame(device)) {
		VOE_TEST_CHECK(!build_right(device, &geometry, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
		VOE_TEST_CHECK(close_frame(device));
	}

	voe_render_device_destroy(device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };
	voe_render_vertex vertices[4];
	uint32_t indices[6];

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// Every count below reads bytes as BGRA; see colour_of.
	VOE_TEST_CHECK_INT(scene.device->format.format, VK_FORMAT_B8G8R8A8_SRGB);

	if (!resolve_readback(scene.device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(scene.device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_buffer_build(
		scene.device, &scene.readback, IMAGE_BYTES,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (scene.readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(scene.device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	// Mapped once for the whole run: every read_back below waits for idle
	// before the pixels are looked at, and the memory is coherent.
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene.device->device,
						    scene.readback.memory, 0,
						    VK_WHOLE_SIZE, 0,
						    &scene.pixels),
			   VK_SUCCESS);

	VOE_TEST_CHECK(voe_render_shading_create(scene.device, RED, &scene.red,
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, GREEN,
						 &scene.green, &error));

	// The static half, uploaded before any frame as a static range is.
	quad(-1.0f, 0.0f, vertices, indices);
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, vertices, 4,
						  indices, 6, &scene.left,
						  &error));
	VOE_TEST_CHECK(scene.left.index < CAPACITIES.geometries);

	if (scene.pixels != NULL) {
		an_id_lives_one_frame(&scene);
		both_pools_draw_in_one_frame(&scene);
		overrunning_is_refused_and_the_next_frame_is_fine(&scene);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	voe_render_vk.device_wait_idle(scene.device->device);
	if (scene.pixels != NULL)
		voe_render_vk.unmap_memory(scene.device->device,
					   scene.readback.memory);
	voe_render_buffer_teardown(scene.device, &scene.readback);
	voe_render_device_destroy(scene.device);

	no_transient_room_is_a_refusal(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
