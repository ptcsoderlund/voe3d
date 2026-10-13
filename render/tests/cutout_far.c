// A FAR CUT-OUT STAYS SOLID (ADR-0421 point 3). A 256×256 SMOOTH colour texture
// of one-texel vertical stripes, white with alpha 1 and 0 alternating, on an
// unlit cutout record with cutoff 0.5, drawn twice facing the camera:
//
// 1. near: a quad of half-side 8 filling the 64×64 picture, so 32 texels show
//    two pixels each and the texture reads at level 0. About half the pixels
//    are drawn: 2048 of 4096, checked between 40% and 60%.
// 2. far: a quad 16 pixels across, so sixteen texels a pixel and level 4. Its
//    mips average the stripes to alpha 0.5, which the cut alone would decide by
//    rounding; scaled by 1 + 0.25 × 4 the leaves stay. At least a third of its
//    256 pixels are drawn.
//
// A drawn pixel is white on every channel (unlit white, exposure ×1 and Clip);
// an undrawn one is the clear colour, near black. The camera and quad are
// mips.c's: eye at the origin looking down −Z, 90°, so at distance 1 the
// picture is 2 units across.
//
// It runs headless and reads the window target back, as mips.c does, and
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
#define TEXELS 256

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The near case's bounds on drawn pixels: 40% and 60% of the picture.
#define NEAR_LEAST (SIDE * SIDE * 2 / 5)
#define NEAR_MOST (SIDE * SIDE * 3 / 5)
// The far quad's square, pixels 24 .. 39 on both axes, and the least drawn.
#define FAR_LEFT 24
#define FAR_RIGHT 40
#define FAR_PIXELS ((FAR_RIGHT - FAR_LEFT) * (FAR_RIGHT - FAR_LEFT))
#define FAR_LEAST ((FAR_PIXELS + 2) / 3)

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
};

// A unit quad facing +Z, wound counter-clockwise from the camera, (0,0) at its
// top-left.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
	{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0 } },
	{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0 } },
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// The stripes: every texel white, alpha 255 on even columns and 0 on odd.
static uint8_t stripe_pixels[TEXELS * TEXELS * 4];

static const uint8_t *stripes(void)
{
	for (uint32_t y = 0; y < TEXELS; y++) {
		for (uint32_t x = 0; x < TEXELS; x++) {
			uint8_t *p = stripe_pixels + ((size_t)y * TEXELS + x) * 4;

			memset(p, 255, 3);
			p[3] = (x & 1) ? 0 : 255;
		}
	}
	return stripe_pixels;
}

// Eye at the origin looking down −Z, reverse-Z with a 90° field, as mips.c's.
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

// One quad placed by `world`, drawn and read back into `buffer` at `offset`.
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

// How many pixels in columns and rows `first` .. `last` − 1 are drawn: white
// within 2 on every channel.
static uint32_t drawn(const uint8_t *image, uint32_t first, uint32_t last)
{
	uint32_t count = 0;

	for (uint32_t y = first; y < last; y++) {
		for (uint32_t x = first; x < last; x++) {
			const uint8_t *p = image + ((size_t)y * SIDE + x) * 4;

			if (p[0] >= 253 && p[1] >= 253 && p[2] >= 253)
				count++;
		}
	}
	return count;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_texture texture = { 0 };
	voe_render_shading cutout = { 0 };
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_CUTOUT,
		.alpha_cutoff = 0.5f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
		.unlit = 1,
	};
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
		device, &readback, IMAGE_BYTES * 2,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	VOE_TEST_CHECK(voe_render_texture_create(
		device, VOE_RENDER_TEXTURE_COLOUR, VOE_RENDER_SAMPLING_SMOOTH,
		TEXELS, TEXELS, stripes(), &texture, &error));
	values.base_colour_texture = texture.index;
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &cutout,
						 &error));

	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE) {
		draw_case(device, quad, cutout, facing(8.0f), readback.buffer,
			  0);
		draw_case(device, quad, cutout, facing(0.25f), readback.buffer,
			  IMAGE_BYTES);

		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		if (mapped != NULL) {
			const uint8_t *pixels = mapped;
			uint32_t near = drawn(pixels, 0, SIDE);
			uint32_t far = drawn(pixels + IMAGE_BYTES, FAR_LEFT,
					     FAR_RIGHT);

			printf("near %u of %u drawn, far %u of %u\n", near,
			       SIDE * SIDE, far, FAR_PIXELS);
			VOE_TEST_CHECK(near >= NEAR_LEAST);
			VOE_TEST_CHECK(near <= NEAR_MOST);
			VOE_TEST_CHECK(far >= FAR_LEAST);
			voe_render_vk.unmap_memory(device->device,
						   readback.memory);
		}
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
