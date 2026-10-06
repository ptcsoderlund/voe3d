// A SMOOTH texture's mip chain (ADR-0359), checked in the picture: hard texels
// when it is magnified, a blend of black and white when it is minified far.
//
// The first two cases are each one quad facing the camera wearing a black and white one-texel
// checker, unlit so no light moves the numbers. Up close a 4×4 checker fills the
// whole 64×64 picture, sixteen pixels a texel, and every pixel must be within 2
// of pure black or pure white: magnification is NEAREST. Far, a 256×256 checker
// on a quad 16 pixels across, so a pixel spans sixteen texels; every pixel inside
// its edge must be between 64 and 220, which one level read NEAREST never gives.
// The linear-light average of black and white is 188 in the sRGB target.
//
// A third case is low-angle (ADR-0369): the same checker tiled 64×16 times over
// a plane 200 wide and 90 deep, one unit below the eye, from 1 to 91 ahead. It
// fills the picture's lower half; its far half, the 16 rows by the horizon, is
// minified two texels or more on both axes and must be the same blend, which is
// anisotropic filtering with its mips and not a smear or a crawl.
//
// The camera sits at the origin looking down −Z with a 90° field of view, so
// at a distance of 1 the picture is exactly 2 units across: a quad of half-side
// 1 fills it and one of half-side 0.25 is 16 pixels.
//
// It runs headless and reads the window target back, as offscreen.c does, and
// includes render's internal header for the same reason. A machine with no
// usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
#define FAR_TEXELS 256

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 3,
	.passes = 1,
};

// A unit quad facing +Z, wound counter-clockwise from the camera, (0,0) at its
// top-left.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// The checker, one texel black and the next white, in a static buffer big
// enough for the far case.
static uint8_t checker_pixels[FAR_TEXELS * FAR_TEXELS * 4];

static const uint8_t *checker(uint32_t side)
{
	for (uint32_t y = 0; y < side; y++) {
		for (uint32_t x = 0; x < side; x++) {
			uint8_t v = ((x + y) & 1) ? 255 : 0;
			uint8_t *p = checker_pixels + ((size_t)y * side + x) * 4;

			p[0] = v;
			p[1] = v;
			p[2] = v;
			p[3] = 255;
		}
	}
	return checker_pixels;
}

// Eye at the origin looking down −Z: the view is the identity, and the
// projection is reverse-Z with a 90° field of view, as offscreen.c's is.
static voe_render_view the_camera(void)
{
	voe_render_view view = { 0 };
	float span = FAR_PLANE - NEAR_PLANE;

	view.view = voe_math_float4x4_identity();
	view.projection.m[0][0] = 1.0f;
	view.projection.m[1][1] = 1.0f;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The quad facing the camera at z = −1, of the given half-side.
static voe_math_float4x4 facing(float half)
{
	voe_math_float4x4 world = voe_math_float4x4_identity();

	world.m[0][0] = half;
	world.m[1][1] = half;
	world.m[2][3] = -1.0f;
	return world;
}

// The quad laid flat one unit below the eye, its top side up: x stays x, its y
// runs away down −Z from z = −1 to −91, and 200 wide so it spans the picture.
static voe_math_float4x4 low_angle(void)
{
	voe_math_float4x4 world = { 0 };

	world.m[0][0] = 100.0f;
	world.m[1][2] = 1.0f;
	world.m[1][3] = -1.0f;
	world.m[2][1] = -45.0f;
	world.m[2][3] = -46.0f;
	world.m[3][3] = 1.0f;
	return world;
}

// One texture, one record, one quad placed by `world`, drawn and read back
// into `buffer` at `offset`.
static void draw_case(voe_render_device *device, voe_render_geometry quad,
		      voe_render_shading shading, voe_math_float4x4 world,
		      VkBuffer buffer, VkDeviceSize offset)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_object object = {
		.world = world,
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_pass_camera camera = { .view = the_camera() };
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

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, quad, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
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

// Every pixel within 2 of pure black or pure white on every channel, and both
// present, so a picture of one colour does not pass.
static void check_hard(const uint8_t *image)
{
	uint32_t blacks = 0;
	uint32_t whites = 0;
	uint32_t others = 0;

	for (uint32_t i = 0; i < SIDE * SIDE; i++) {
		const uint8_t *p = image + (size_t)i * 4;

		if (p[0] <= 2 && p[1] <= 2 && p[2] <= 2)
			blacks++;
		else if (p[0] >= 253 && p[1] >= 253 && p[2] >= 253)
			whites++;
		else
			others++;
	}
	VOE_TEST_CHECK_INT(others, 0);
	VOE_TEST_CHECK(blacks > 0);
	VOE_TEST_CHECK(whites > 0);
}

// Every pixel in rows `top` .. `bottom` − 1 and columns `left` .. `right` − 1
// must be a blend on every channel.
static void check_blend(const uint8_t *image, uint32_t left, uint32_t top,
			uint32_t right, uint32_t bottom)
{
	uint32_t outside = 0;

	for (uint32_t y = top; y < bottom; y++) {
		for (uint32_t x = left; x < right; x++) {
			const uint8_t *p = image + ((size_t)y * SIDE + x) * 4;

			for (uint32_t c = 0; c < 3; c++) {
				if (p[c] < 64 || p[c] > 220)
					outside++;
			}
		}
	}
	VOE_TEST_CHECK_INT(outside, 0);
}

// A checker of `side` texels tiled `across` × `along` times over the quad.
static bool make_case(voe_render_device *device, uint32_t side, float across,
		      float along, voe_render_shading *out,
		      voe_base_error *error)
{
	voe_render_texture texture = { 0 };
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, across, along },
		.unlit = 1,
	};

	if (!voe_render_texture_create(device, VOE_RENDER_TEXTURE_COLOUR,
				       VOE_RENDER_SAMPLING_SMOOTH, side, side,
				       checker(side), &texture, error))
		return false;
	values.base_colour_texture = texture.index;
	return voe_render_shading_create(device, values, out, error);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_shading near_case = { 0 };
	voe_render_shading far_case = { 0 };
	voe_render_shading low_case = { 0 };
	void *mapped = NULL;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
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

	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	VOE_TEST_CHECK(copy_image_to_buffer != NULL);
	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, IMAGE_BYTES * 3,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	VOE_TEST_CHECK(make_case(device, 4, 1.0f, 1.0f, &near_case, &error));
	VOE_TEST_CHECK(make_case(device, FAR_TEXELS, 1.0f, 1.0f, &far_case,
				 &error));
	VOE_TEST_CHECK(make_case(device, FAR_TEXELS, 64.0f, 16.0f, &low_case,
				 &error));

	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE) {
		draw_case(device, quad, near_case, facing(1.0f),
			  readback.buffer, 0);
		draw_case(device, quad, far_case, facing(0.25f),
			  readback.buffer, IMAGE_BYTES);
		draw_case(device, quad, low_case, low_angle(), readback.buffer,
			  IMAGE_BYTES * 2);

		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		if (mapped != NULL) {
			check_hard(mapped);
			// The far quad's inside, one pixel in from its edges.
			check_blend((const uint8_t *)mapped + IMAGE_BYTES, 25,
				    25, 39, 39);
			// The plane's far half: the 16 rows below the horizon.
			check_blend((const uint8_t *)mapped + IMAGE_BYTES * 2,
				    0, SIDE / 2, SIDE, SIDE * 3 / 4);			voe_render_vk.unmap_memory(device->device,
						   readback.memory);
		}
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
