// voe_render_texture_create_blocks and voe_render_texture_memory_smooth
// (ADR-0477 points 1, 6 and 7).
//
// bc7_is_drawn: a 16×16 chain of hand-built BC7 mode-6 blocks, every one the
// opaque colour (201, 101, 51), as COLOUR, on a quad 16 pixels across facing the
// camera, unlit, as mips.c's given_levels_are_drawn draws. Every pixel inside the
// quad's edge must read what an RGBA8 COLOUR texture of that colour reads, within
// 2, and redder than blue, so two quads that were never drawn do not pass.
//
// bc5_is_made: a 16×16 BC5 DATA chain of mid-grey blocks is made.
//
// memory_counts: an RGBA8 SMOOTH texture and a BC7 one each raise `count` and
// `bytes`, the BC7 one `compressed` too, and destroying both gives the first
// reading back.
//
// The colour's three channels are odd so that mode 6's one shared low bit per
// endpoint, P, can be 1 for all four with alpha 255.
//
// It runs headless and reads the window target back, as mips.c does, and includes
// render's internal header for the same reason. A machine with no usable Vulkan
// skips and says so; a card without textureCompressionBC checks that the upload
// is refused REFUSED and skips the rest.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
#define TEXELS 16
#define LEVELS 5
// 4×4 + 2×2 + 1 + 1 + 1 blocks down the 16×16 chain.
#define CHAIN_BLOCKS 23

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 4,
	.passes = 1,
};

static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
	{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0 } },
	{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0 } },
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

static const uint8_t COLOUR[4] = { 201, 101, 51, 255 };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static uint8_t chain[CHAIN_BLOCKS * 16];
static uint8_t rgba[TEXELS * TEXELS * 4];

// `count` bits of `value` into `block` at bit `at`, least significant first, as
// BC7 lays a block out; the new position back.
static uint32_t put_bits(uint8_t *block, uint32_t at, uint32_t count,
			 uint32_t value)
{
	for (uint32_t i = 0; i < count; i++, at++) {
		if ((value >> i) & 1)
			block[at / 8] |= (uint8_t)(1u << (at % 8));
	}
	return at;
}

// One BC7 mode-6 block of COLOUR: the mode bit, both endpoints the colour's top
// seven bits per channel, both P bits 1, every index 0.
static void bc7_block(uint8_t *block)
{
	uint32_t at = 0;

	memset(block, 0, 16);
	at = put_bits(block, at, 7, 1u << 6);
	for (uint32_t c = 0; c < 4; c++) {
		at = put_bits(block, at, 7, COLOUR[c] >> 1);
		at = put_bits(block, at, 7, COLOUR[c] >> 1);
	}
	at = put_bits(block, at, 2, 3);
	VOE_TEST_CHECK_INT(at, 65);
}

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

// An unlit white record wearing `texture`, on the quad 16 pixels across at
// z = −1, drawn and read back into `buffer` at `offset`.
static void draw_wearing(voe_render_device *device, voe_render_geometry quad,
			 voe_render_texture texture, VkBuffer buffer,
			 VkDeviceSize offset)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
		.unlit = 1,
		.base_colour_texture = texture.index,
	};
	voe_render_shading shading = { 0 };
	// Before the frame begins: the slot this frame draws into.
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_math_float4x4 world = voe_math_float4x4_identity();
	voe_platform_size size = { SIDE, SIDE };
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
	voe_render_object object = {
		.normal = voe_math_float4x4_identity(),
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	bool drawing = false;

	world.m[0][0] = 0.25f;
	world.m[1][1] = 0.25f;
	world.m[2][3] = -1.0f;
	object.world = world;
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));
	object.shading = shading.index;

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

static void bc7_is_drawn(voe_render_device *device, voe_render_geometry quad,
			 const struct voe_render_buffer *readback)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture blocks = { 0 };
	voe_render_texture plain = { 0 };
	uint32_t off = 0;
	void *mapped = NULL;

	for (uint32_t i = 0; i < CHAIN_BLOCKS; i++)
		bc7_block(chain + (size_t)i * 16);
	for (uint32_t i = 0; i < TEXELS * TEXELS; i++)
		memcpy(rgba + (size_t)i * 4, COLOUR, 4);
	VOE_TEST_CHECK(voe_render_texture_create_blocks(
		device, VOE_RENDER_TEXTURE_COLOUR, VOE_RENDER_BLOCKS_BC7, TEXELS,
		TEXELS, LEVELS, chain, &blocks, &error));
	VOE_TEST_CHECK(voe_render_texture_create(device,
						 VOE_RENDER_TEXTURE_COLOUR,
						 VOE_RENDER_SAMPLING_SMOOTH,
						 TEXELS, TEXELS, rgba, &plain,
						 &error));

	draw_wearing(device, quad, blocks, readback->buffer, 0);
	draw_wearing(device, quad, plain, readback->buffer, IMAGE_BYTES);
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback->memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped == NULL)
		return;
	// One pixel in from the quad's edges; the target is B8G8R8A8.
	for (uint32_t y = 25; y < 39; y++) {
		for (uint32_t x = 25; x < 39; x++) {
			size_t at = ((size_t)y * SIDE + x) * 4;
			const uint8_t *a = (const uint8_t *)mapped + at;
			const uint8_t *b = a + IMAGE_BYTES;

			for (uint32_t c = 0; c < 3; c++) {
				if (a[c] > b[c] + 2 || b[c] > a[c] + 2)
					off++;
			}
			if (b[2] <= b[0] + 32)
				off++;
		}
	}
	voe_render_vk.unmap_memory(device->device, readback->memory);
	VOE_TEST_CHECK_INT(off, 0);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, blocks));
	VOE_TEST_CHECK(voe_render_texture_destroy(device, plain));
}

// Each of the two BC4 halves: both endpoints 128, every index 0.
static void bc5_is_made(voe_render_device *device)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture texture = { 0 };

	memset(chain, 0, sizeof chain);
	for (uint32_t i = 0; i < CHAIN_BLOCKS * 2; i++) {
		chain[i * 8] = 128;
		chain[i * 8 + 1] = 128;
	}
	VOE_TEST_CHECK(voe_render_texture_create_blocks(
		device, VOE_RENDER_TEXTURE_DATA, VOE_RENDER_BLOCKS_BC5, TEXELS,
		TEXELS, LEVELS, chain, &texture, &error));
	VOE_TEST_CHECK(texture.index != VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, texture));
}

static void memory_counts(voe_render_device *device)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture plain = { 0 };
	voe_render_texture blocks = { 0 };
	const voe_render_texture_memory start =
		voe_render_texture_memory_smooth(device);
	voe_render_texture_memory first = start;
	voe_render_texture_memory reading;

	for (uint32_t i = 0; i < CHAIN_BLOCKS; i++)
		bc7_block(chain + (size_t)i * 16);
	VOE_TEST_CHECK(voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_SMOOTH,
						 TEXELS, TEXELS, rgba, &plain,
						 &error));
	reading = voe_render_texture_memory_smooth(device);
	VOE_TEST_CHECK_INT(reading.count, first.count + 1);
	VOE_TEST_CHECK(reading.bytes > first.bytes);
	VOE_TEST_CHECK_INT(reading.compressed, first.compressed);

	first = reading;
	VOE_TEST_CHECK(voe_render_texture_create_blocks(
		device, VOE_RENDER_TEXTURE_COLOUR, VOE_RENDER_BLOCKS_BC7, TEXELS,
		TEXELS, LEVELS, chain, &blocks, &error));
	reading = voe_render_texture_memory_smooth(device);
	VOE_TEST_CHECK_INT(reading.count, first.count + 1);
	VOE_TEST_CHECK(reading.bytes > first.bytes);
	VOE_TEST_CHECK_INT(reading.compressed, first.compressed + 1);

	VOE_TEST_CHECK(voe_render_texture_destroy(device, plain));
	VOE_TEST_CHECK(voe_render_texture_destroy(device, blocks));
	reading = voe_render_texture_memory_smooth(device);
	VOE_TEST_CHECK(reading.bytes == start.bytes);
	VOE_TEST_CHECK_INT(reading.count, start.count);
	VOE_TEST_CHECK_INT(reading.compressed, start.compressed);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };

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

	if (!device->bc) {
		voe_render_texture texture = { 0 };

		VOE_TEST_CHECK(!voe_render_texture_create_blocks(
			device, VOE_RENDER_TEXTURE_COLOUR, VOE_RENDER_BLOCKS_BC7,
			TEXELS, TEXELS, LEVELS, chain, &texture, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
		printf("skip: no textureCompressionBC on this card\n");
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	memory_counts(device);
	bc5_is_made(device);

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
	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE)
		bc7_is_drawn(device, quad, &readback);

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
