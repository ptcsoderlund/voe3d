// voe_render_compress_start, _ready and _finish (ADR-0486), held to the bounds
// assets/tests/image_bc7.c and image_blocks.c held the CPU encoder to.
//
// Each result is drawn as texture_blocks.c draws one: a level at a time, as a
// one-level texture through voe_render_texture_create_blocks beside its RGBA8
// source through _create_levels, unlit on a quad exactly the level's size in
// the target's top-left corner, so every pixel is one texel's centre. A COLOUR
// draw is compared with its source's draw, and that draw with the source within
// 1; a DATA draw is read back to linear and compared with the source value,
// 2 more for the sRGB target's step, near 1.6 linear at the values drawn.
//
// gradient: 64×64, R 30 + s, G 200 − s, B 60 + s/2 (s = x + y), box-mipped to
// 1×1, as BC7 COLOUR within 4. noise: 64×64 and every level its own, each block
// two random colours in a checker, as BC7 COLOUR within 3, image_bc7.c's two
// colours. bump: a normal map, R 128 + 36 sin(2πx/64), G the same in y,
// box-mipped, as BC5 DATA within 3 on x and y.
//
// edge: 13×9 greys 100 + 3x + 12y and its four levels as BC7 COLOUR within 3,
// and level 0's blocks byte for byte those of the same picture widened to
// 16×12 by repeating its last column and row: the edge blocks repeat the edge.
//
// two_jobs: the gradient as BC7 and the bump as BC5 started together are both
// ready within ten seconds; the BC5 dropped with NULL, the BC7's blocks are
// those its blocking run gave. No job is live at the end.
//
// It runs headless and includes render's internal header, as texture_blocks.c
// does. No usable Vulkan, or no textureCompressionBC to draw the blocks with,
// skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
// 64×64 down to 1×1: 4096 + 1024 + 256 + 64 + 16 + 4 + 1 texels, and
// 256 + 64 + 16 + 4 + 1 + 1 + 1 blocks.
#define CHAIN_TEXELS 5461
#define CHAIN_BLOCKS 343
#define EDGE_WIDTH 13
#define EDGE_HEIGHT 9

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

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static uint8_t gradient[CHAIN_TEXELS * 4];
static uint8_t gradient_blocks[CHAIN_BLOCKS * 16];
static uint8_t picture[CHAIN_TEXELS * 4];
static uint8_t picture_blocks[CHAIN_BLOCKS * 16];
static uint8_t again[CHAIN_BLOCKS * 16];

// What a level is drawn with, and how near its blocks must come.
struct chain {
	voe_render_texture_kind kind;
	voe_render_blocks blocks;
	uint32_t width;
	uint32_t height;
	uint32_t levels;
	const uint8_t *rgba;
	const uint8_t *bytes;
	int bound;
};

static uint32_t side(uint32_t full, uint32_t level)
{
	return full >> level > 0 ? full >> level : 1;
}

// Level `level`'s first texel, and its first block.
static size_t texels_before(uint32_t w, uint32_t h, uint32_t level)
{
	size_t at = 0;

	for (uint32_t i = 0; i < level; i++)
		at += (size_t)side(w, i) * side(h, i);
	return at;
}

static size_t blocks_before(uint32_t w, uint32_t h, uint32_t level)
{
	size_t at = 0;

	for (uint32_t i = 0; i < level; i++)
		at += (size_t)((side(w, i) + 3) / 4) * ((side(h, i) + 3) / 4);
	return at;
}

// Level 0 already in `chain`, every level below it the 2×2 average of the one
// above, a texel past the edge the edge's; the level count back.
static uint32_t box_chain(uint8_t *chain, uint32_t w, uint32_t h)
{
	const uint32_t levels = voe_render_texture_full_levels(w, h);

	for (uint32_t i = 1; i < levels; i++) {
		const uint8_t *up = chain + texels_before(w, h, i - 1) * 4;
		uint8_t *down = chain + texels_before(w, h, i) * 4;
		const uint32_t uw = side(w, i - 1), uh = side(h, i - 1);

		for (uint32_t y = 0; y < side(h, i); y++)
			for (uint32_t x = 0; x < side(w, i); x++)
				for (uint32_t c = 0; c < 4; c++) {
					const uint32_t x0 = 2 * x < uw ? 2 * x : uw - 1;
					const uint32_t x1 = 2 * x + 1 < uw ? 2 * x + 1 : uw - 1;
					const uint32_t y0 = 2 * y < uh ? 2 * y : uh - 1;
					const uint32_t y1 = 2 * y + 1 < uh ? 2 * y + 1 : uh - 1;
					const uint32_t sum = up[(y0 * uw + x0) * 4 + c] +
							     up[(y0 * uw + x1) * 4 + c] +
							     up[(y1 * uw + x0) * 4 + c] +
							     up[(y1 * uw + x1) * 4 + c];

					down[(y * side(w, i) + x) * 4 + c] = (uint8_t)((sum + 2) / 4);
				}
	}
	return levels;
}

static uint32_t next_random(uint32_t *state)
{
	*state ^= *state << 13;
	*state ^= *state >> 17;
	*state ^= *state << 5;
	return *state;
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

// An unlit white record wearing `texture`, on a quad `w` × `h` pixels in the
// target's top-left corner at z = −1, drawn and read back into `buffer` at
// `offset`; the record destroyed after.
static void draw_wearing(voe_render_device *device, voe_render_geometry quad,
			 voe_render_texture texture, uint32_t w, uint32_t h,
			 VkBuffer buffer, VkDeviceSize offset)
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

	// The quad spans ±1, so a scale of w / 64 makes it w of the 64 pixels.
	world.m[0][0] = (float)w / SIDE;
	world.m[1][1] = (float)h / SIDE;
	world.m[0][3] = (float)w / SIDE - 1.0f;
	world.m[1][3] = 1.0f - (float)h / SIDE;
	world.m[2][3] = -1.0f;
	object.world = world;
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));
	object.shading = shading.index;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(device,
						     VOE_RENDER_TARGET_WINDOW,
						     &camera));
		VOE_TEST_CHECK(voe_render_frame_draw(device, quad, object));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	voe_render_vk.device_wait_idle(device->device);
	if (drawing &&
	    voe_render_vk.allocate_command_buffers(device->device, &allocate,
						   &commands) == VK_SUCCESS) {
		voe_render_vk.begin_command_buffer(commands, &begin);
		copy_image_to_buffer(commands, frame->target.colour.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer,
				     1, &region);
		voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
		voe_render_vk.end_command_buffer(commands);
		submit_commands.commandBuffer = commands;
		VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1,
							       &submit,
							       VK_NULL_HANDLE),
				   VK_SUCCESS);
		voe_render_vk.device_wait_idle(device->device);
		voe_render_vk.free_command_buffers(device->device, device->pool,
						   1, &commands);
	}
	VOE_TEST_CHECK(voe_render_shading_destroy(device, shading));
}

// A drawn byte of a DATA texture, back to the linear value it encoded.
static float to_linear(uint8_t drawn)
{
	const float v = drawn / 255.0f;

	return 255.0f * (v <= 0.04045f ? v / 12.92f :
					 powf((v + 0.055f) / 1.055f, 2.4f));
}

static bool near(float a, float b, float bound)
{
	return fabsf(a - b) <= bound;
}

// Every level of `chain` drawn as blocks and as RGBA8; how many channels of
// how many pixels miss their bound.
static uint32_t drawn_misses(voe_render_device *device,
			     voe_render_geometry quad,
			     const struct voe_render_buffer *readback,
			     struct chain chain)
{
	uint32_t misses = 0;

	for (uint32_t i = 0; i < chain.levels; i++) {
		const uint32_t w = side(chain.width, i), h = side(chain.height, i);
		const uint8_t *source =
			chain.rgba + texels_before(chain.width, chain.height, i) * 4;
		voe_base_error error = VOE_BASE_OK;
		voe_render_texture blocks = { 0 }, plain = { 0 };
		void *mapped = NULL;

		VOE_TEST_CHECK(voe_render_texture_create_blocks(
			device, chain.kind, chain.blocks, w, h, 1,
			chain.bytes + blocks_before(chain.width, chain.height, i) * 16,
			&blocks, &error));
		VOE_TEST_CHECK(voe_render_texture_create_levels(
			device, chain.kind, w, h, 1, source, &plain, &error));
		draw_wearing(device, quad, blocks, w, h, readback->buffer, 0);
		draw_wearing(device, quad, plain, w, h, readback->buffer,
			     IMAGE_BYTES);
		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback->memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		for (uint32_t y = 0; mapped != NULL && y < h; y++)
			for (uint32_t x = 0; x < w; x++) {
				// The target is B8G8R8A8; BC5 holds R and G alone.
				const uint8_t *a = (const uint8_t *)mapped +
						   ((size_t)y * SIDE + x) * 4;
				const uint8_t *b = a + IMAGE_BYTES;
				const uint8_t *s = source + ((size_t)y * w + x) * 4;
				const uint32_t channels =
					chain.blocks == VOE_RENDER_BLOCKS_BC5 ? 2 : 3;

				for (uint32_t c = 0; c < channels; c++) {
					if (chain.kind == VOE_RENDER_TEXTURE_DATA) {
						misses += !near(to_linear(b[2 - c]), s[c], 2.0f);
						misses += !near(to_linear(a[2 - c]), s[c],
								(float)chain.bound + 2.0f);
						continue;
					}
					misses += !near(b[2 - c], s[c], 1.0f);
					misses += !near(a[2 - c], b[2 - c],
							(float)chain.bound + 1.0f);
				}
			}
		if (mapped != NULL)
			voe_render_vk.unmap_memory(device->device, readback->memory);
		VOE_TEST_CHECK(voe_render_texture_destroy(device, blocks));
		VOE_TEST_CHECK(voe_render_texture_destroy(device, plain));
	}
	return misses;
}

// Start then finish: the blocking caller.
static void encode(voe_render_device *device, voe_render_blocks blocks,
		   uint32_t w, uint32_t h, uint32_t levels, const uint8_t *rgba,
		   uint8_t *out)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_compressing *job = voe_render_compress_start(
		device, blocks, w, h, levels, rgba, &error);

	VOE_TEST_CHECK(job != NULL);
	if (job != NULL)
		voe_render_compress_finish(device, job, out);
}

static void make_gradient(uint8_t *chain)
{
	for (uint32_t y = 0; y < SIDE; y++)
		for (uint32_t x = 0; x < SIDE; x++) {
			uint8_t *t = chain + ((size_t)y * SIDE + x) * 4;
			const uint32_t s = x + y;

			t[0] = (uint8_t)(30 + s);
			t[1] = (uint8_t)(200 - s);
			t[2] = (uint8_t)(60 + s / 2);
			t[3] = 255;
		}
	box_chain(chain, SIDE, SIDE);
}

static void make_bump(uint8_t *chain)
{
	const double tau = 6.283185307179586;

	for (uint32_t y = 0; y < SIDE; y++)
		for (uint32_t x = 0; x < SIDE; x++) {
			uint8_t *t = chain + ((size_t)y * SIDE + x) * 4;

			t[0] = (uint8_t)(128 + lround(36.0 * sin(tau * x / SIDE)));
			t[1] = (uint8_t)(128 + lround(36.0 * sin(tau * y / SIDE)));
			t[2] = 230;
			t[3] = 255;
		}
	box_chain(chain, SIDE, SIDE);
}

// Every level its own: each block two random opaque colours in a checker.
static void make_noise(uint8_t *chain)
{
	uint32_t state = 0x9e3779b9u;

	for (uint32_t i = 0; i < voe_render_texture_full_levels(SIDE, SIDE); i++) {
		const uint32_t w = side(SIDE, i);
		uint8_t *level = chain + texels_before(SIDE, SIDE, i) * 4;

		for (uint32_t by = 0; by < (w + 3) / 4; by++)
			for (uint32_t bx = 0; bx < (w + 3) / 4; bx++) {
				uint8_t colour[2][4] = { { 0, 0, 0, 255 },
							 { 0, 0, 0, 255 } };

				for (uint32_t k = 0; k < 2; k++)
					for (uint32_t c = 0; c < 3; c++)
						colour[k][c] = (uint8_t)next_random(&state);
				for (uint32_t y = by * 4; y < by * 4 + 4 && y < w; y++)
					for (uint32_t x = bx * 4; x < bx * 4 + 4 && x < w; x++)
						memcpy(level + ((size_t)y * w + x) * 4,
						       colour[(x + y) & 1], 4);
			}
	}
}

static void colour_chains(voe_render_device *device, voe_render_geometry quad,
			  const struct voe_render_buffer *readback)
{
	const uint32_t levels = voe_render_texture_full_levels(SIDE, SIDE);
	struct chain chain = {
		.kind = VOE_RENDER_TEXTURE_COLOUR,
		.blocks = VOE_RENDER_BLOCKS_BC7,
		.width = SIDE,
		.height = SIDE,
		.levels = levels,
		.rgba = gradient,
		.bytes = gradient_blocks,
		.bound = 4,
	};

	make_gradient(gradient);
	encode(device, VOE_RENDER_BLOCKS_BC7, SIDE, SIDE, levels, gradient,
	       gradient_blocks);
	VOE_TEST_CHECK_INT(drawn_misses(device, quad, readback, chain), 0);

	make_noise(picture);
	encode(device, VOE_RENDER_BLOCKS_BC7, SIDE, SIDE, levels, picture,
	       picture_blocks);
	chain.rgba = picture;
	chain.bytes = picture_blocks;
	chain.bound = 3;
	VOE_TEST_CHECK_INT(drawn_misses(device, quad, readback, chain), 0);
}

static void bump_chain(voe_render_device *device, voe_render_geometry quad,
		       const struct voe_render_buffer *readback)
{
	const uint32_t levels = voe_render_texture_full_levels(SIDE, SIDE);
	const struct chain chain = {
		.kind = VOE_RENDER_TEXTURE_DATA,
		.blocks = VOE_RENDER_BLOCKS_BC5,
		.width = SIDE,
		.height = SIDE,
		.levels = levels,
		.rgba = picture,
		.bytes = picture_blocks,
		.bound = 3,
	};

	make_bump(picture);
	encode(device, VOE_RENDER_BLOCKS_BC5, SIDE, SIDE, levels, picture,
	       picture_blocks);
	VOE_TEST_CHECK_INT(drawn_misses(device, quad, readback, chain), 0);
}

static void edge(voe_render_device *device, voe_render_geometry quad,
		 const struct voe_render_buffer *readback)
{
	// Level 0 of the 13×9 and the 16×12 alike are 4 × 3 blocks.
	static uint8_t widened[16 * 12 * 4];
	static uint8_t widened_blocks[12 * 16];
	struct chain chain = {
		.kind = VOE_RENDER_TEXTURE_COLOUR,
		.blocks = VOE_RENDER_BLOCKS_BC7,
		.width = EDGE_WIDTH,
		.height = EDGE_HEIGHT,
		.rgba = picture,
		.bytes = picture_blocks,
		.bound = 3,
	};

	for (uint32_t y = 0; y < 12; y++)
		for (uint32_t x = 0; x < 16; x++) {
			const uint32_t ex = x < EDGE_WIDTH ? x : EDGE_WIDTH - 1;
			const uint32_t ey = y < EDGE_HEIGHT ? y : EDGE_HEIGHT - 1;
			const uint8_t grey = (uint8_t)(100 + 3 * ex + 12 * ey);
			const uint8_t texel[4] = { grey, grey, grey, 255 };

			memcpy(widened + ((size_t)y * 16 + x) * 4, texel, 4);
			if (x == ex && y == ey)
				memcpy(picture + ((size_t)y * EDGE_WIDTH + x) * 4,
				       texel, 4);
		}
	chain.levels = box_chain(picture, EDGE_WIDTH, EDGE_HEIGHT);
	VOE_TEST_CHECK_INT(chain.levels, 4);
	encode(device, VOE_RENDER_BLOCKS_BC7, EDGE_WIDTH, EDGE_HEIGHT,
	       chain.levels, picture, picture_blocks);
	encode(device, VOE_RENDER_BLOCKS_BC7, 16, 12, 1, widened,
	       widened_blocks);
	VOE_TEST_CHECK(memcmp(picture_blocks, widened_blocks,
			      sizeof widened_blocks) == 0);
	VOE_TEST_CHECK_INT(drawn_misses(device, quad, readback, chain), 0);
}

static void two_jobs(voe_render_device *device)
{
	const uint32_t levels = voe_render_texture_full_levels(SIDE, SIDE);
	const struct timespec millisecond = { .tv_nsec = 1000000 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_compressing *colour, *bump;
	uint32_t waited = 0;

	make_bump(picture);
	colour = voe_render_compress_start(device, VOE_RENDER_BLOCKS_BC7, SIDE,
					   SIDE, levels, gradient, &error);
	bump = voe_render_compress_start(device, VOE_RENDER_BLOCKS_BC5, SIDE,
					 SIDE, levels, picture, &error);
	VOE_TEST_CHECK(colour != NULL && bump != NULL);
	if (colour == NULL || bump == NULL)
		return;
	VOE_TEST_CHECK_INT(device->compress.live, 2);
	while (waited < 10000 && !(voe_render_compress_ready(device, colour) &&
				   voe_render_compress_ready(device, bump))) {
		thrd_sleep(&millisecond, NULL);
		waited++;
	}
	VOE_TEST_CHECK(voe_render_compress_ready(device, colour));
	VOE_TEST_CHECK(voe_render_compress_ready(device, bump));
	voe_render_compress_finish(device, bump, NULL);
	memset(again, 0, sizeof again);
	voe_render_compress_finish(device, colour, again);
	VOE_TEST_CHECK(memcmp(again, gradient_blocks, sizeof again) == 0);
	VOE_TEST_CHECK_INT(device->compress.live, 0);
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
		printf("skip: no textureCompressionBC to draw the blocks with\n");
		voe_render_device_destroy(device);
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
	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE) {
		colour_chains(device, quad, &readback);
		bump_chain(device, quad, &readback);
		edge(device, quad, &readback);
		two_jobs(device);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
