// A target of one's own, drawn into through a pass and shown on the window
// through its texture id, read back out of the window's picture. Eight claims.
//
// AN ELEMENT SHOWS IT AND A MESH SHOWS IT, THE RIGHT WAY UP. The target's
// picture is red in its top-left quarter and green everywhere else, which is the
// one arrangement a picture turned over on either axis gets wrong: shown over the
// whole window, exactly the window's top-left quarter has to be red. Once through
// an IMAGE element, once through a full-window quad whose shading record carries
// the same texture id in its base colour slot.
//
// EACH FRAME SLOT READS ITS OWN SLOT'S PICTURE. Frame by frame the target is
// drawn red, green, blue, red, … and shown on the window in the same frame, for
// twice as many frames as there are slots. A texture slot that named one frame
// slot's image for every set shows the previous frame's colour in every other
// frame, and the per-frame colours are printed so a reader can see which.
//
// A RESIZE KEEPS THE ID AND CHANGES THE PICTURE'S SIZE. Resized to 32×16, the
// target is drawn with one red texel in its top-left corner and green elsewhere,
// and shown over the 64×64 window through the unchanged id: the red is exactly
// two pixels wide and four tall. At the old size it would be one by one.
//
// A SHEET RECTANGLE PICKS PART OF THE PICTURE. The target is red on its left
// half and green on its right, and an IMAGE element whose sheet covers the left
// half only shows red across the whole window.
//
// THE CAPACITY IS A RETURNED REFUSAL. A device with room for no targets refuses
// the first create, and one with room for one refuses the second.
//
// A TARGET READ BACK IS RGBA8, THE RIGHT WAY UP, AND THE WINDOW'S IS THE SAME
// PICTURE. One frame draws the same three quadrants — red top-left, green
// top-right, blue bottom-left, the fourth left to the clear colour — into the
// target and into the window, and voe_render_target_read brings both back. The
// corner pixels are the colours drawn, so `pixels[0]` really is the red channel
// and row zero really is the top; alpha is 255 where the clear shows, which is
// what says the divide by alpha left an opaque picture opaque; and the two
// pictures are byte for byte identical, which is the whole claim that a picture
// saved with no display is the picture a person would have seen.
//
// A TARGET NO PASS HAS DRAWN INTO STILL READS. Its picture is undefined, which
// is not the same thing as an error, so the call succeeds and reports the size
// asked for. A fresh device of its own, because every target in the scene above
// has been drawn into.
//
// THE SURFACES ARE MILLIMETRES THE SIZE OF THE PICTURES, so an element's bounds
// are pixels of whatever it is drawn into. Identity camera and an unlit record
// for the mesh, as tests/passes.c has them.
//
// IT READS THE WINDOW'S PICTURE BOTH WAYS, AND THAT IS DELIBERATE. Most of the
// claims below copy the window's colour image by hand, through render's internal
// header included by relative path, because they are claims about which frame
// slot holds which picture — something the public call deliberately does not
// expose (ADR-0156). The readback claim uses voe_render_target_read, and the two
// agreeing on the same frame is worth having.
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

#define SIDE 64
#define IMAGE_BYTES (SIDE * SIDE * 4)

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 2,
	.shadings = 1,
	// Six in one frame is the readback claim's: three quadrants into the
	// target and the same three into the window.
	.elements = 8,
	.passes = 2,
	.targets = 1,
};

static const voe_math_float4 RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static const voe_math_float4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
static const voe_math_float4 BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
static const voe_math_float4 WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };

// Resolved by hand for the reason tests/offscreen.c gives.
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	return copy_image_to_buffer != NULL;
}

// Copies the slot's finished window target into `buffer` — the shape
// tests/passes.c uses, for the reasons tests/offscreen.c gives.
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

#define NEITHER 0
#define IS_RED 1
#define IS_GREEN 2
#define IS_BLUE 3

static const char *const NAMES[] = { "neither", "red", "green", "blue" };

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
	if (blue > 128 && blue > red && blue > green)
		return IS_BLUE;
	return NEITHER;
}

// How many pixels in the rectangle [x0, x1) × [y0, y1) of the window are
// `colour`.
static int count_in(const unsigned char *image, int x0, int y0, int x1, int y1,
		    int colour)
{
	int count = 0;

	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++)
			if (colour_of(image + (y * SIDE + x) * 4) == colour)
				count++;
	return count;
}

struct scene {
	voe_render_device *device;
	voe_render_target target;
	voe_render_texture picture;
	voe_render_shading shows_picture;
	voe_render_geometry whole_window;
	struct voe_render_buffer readback;
	// vkMapMemory hands its pointer back through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *pixels;
};

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

// A solid element covering `bounds`, clipped to nothing it does not cover.
static voe_render_element solid(voe_math_float4 bounds, voe_math_float4 colour)
{
	return (voe_render_element){
		.bounds = bounds,
		.clip = bounds,
		.colour = colour,
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
}

// A pass onto the target drawing `count` elements over a surface of `size`
// millimetres — the target's own pixel size, so bounds are pixels.
static void draw_into_target(struct scene *scene, voe_math_float2 size,
			     const voe_render_element *elements, uint32_t count)
{
	voe_render_device *device = scene->device;
	uint32_t first = voe_render_frame_elements_submitted(device);

	for (uint32_t i = 0; i < count; i++)
		VOE_TEST_CHECK(voe_render_frame_submit_element(device,
							       elements[i]));

	VOE_TEST_CHECK(voe_render_pass_begin(device, scene->target, NULL));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device, voe_render_element_transform(size), first, count));
	voe_render_pass_end(device);
}

// A pass onto the window showing the target through one IMAGE element over the
// whole window, reading `sheet` of the picture.
static void show_on_window(struct scene *scene, voe_math_float4 sheet)
{
	voe_render_device *device = scene->device;
	voe_math_float2 size = { SIDE, SIDE };
	voe_render_element image = {
		.bounds = { 0.0f, 0.0f, SIDE, SIDE },
		.clip = { 0.0f, 0.0f, SIDE, SIDE },
		.colour = WHITE,
		.kind = VOE_RENDER_ELEMENT_IMAGE,
		.sheet_texture = scene->picture.index,
		.sheet = sheet,
	};
	uint32_t first = voe_render_frame_elements_submitted(device);

	VOE_TEST_CHECK(voe_render_frame_submit_element(device, image));
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device, voe_render_element_transform(size), first, 1));
	voe_render_pass_end(device);
}

// Red in the top-left quarter of a SIDE × SIDE target, green everywhere else.
static void draw_quarter(struct scene *scene)
{
	voe_math_float2 size = { SIDE, SIDE };
	voe_render_element elements[2] = {
		solid((voe_math_float4){ 0.0f, 0.0f, SIDE, SIDE }, GREEN),
		solid((voe_math_float4){ 0.0f, 0.0f, SIDE / 2, SIDE / 2 }, RED),
	};

	draw_into_target(scene, size, elements, 2);
}

#define QUARTER (SIDE / 2 * SIDE / 2)

static void expect_quarter(const unsigned char *image)
{
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE / 2, SIDE / 2, IS_RED),
			   QUARTER);
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_RED), QUARTER);
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_GREEN),
			   SIDE * SIDE - QUARTER);
}

static void an_element_shows_the_target(struct scene *scene)
{
	const struct voe_render_frame *frame =
		voe_render_frame_current(scene->device);

	if (!open_frame(scene->device))
		return;
	draw_quarter(scene);
	show_on_window(scene, (voe_math_float4){ 0.0f, 0.0f, 1.0f, 1.0f });
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));

	read_back(scene->device, frame, scene->readback.buffer);
	expect_quarter(scene->pixels);
}

static void a_mesh_shows_the_target(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_render_pass_camera camera = {
		.view = {
			.view = voe_math_float4x4_identity(),
			.projection = voe_math_float4x4_identity(),
		},
		.light = {
			.direction = { 0.0f, -1.0f, 0.0f },
			.intensity = 1.0f,
			.colour = { 1.0f, 1.0f, 1.0f },
		},
	};

	if (!open_frame(device))
		return;
	draw_quarter(scene);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->whole_window,
					     (voe_render_object){
						     .world = voe_math_float4x4_identity(),
						     .normal = voe_math_float4x4_identity(),
						     .shading = scene->shows_picture.index,
						     .colour = { 1.0f, 1.0f, 1.0f, 1.0f },
					     }));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	read_back(device, frame, scene->readback.buffer);
	expect_quarter(scene->pixels);
}

static void each_frame_reads_its_own_picture(struct scene *scene)
{
	static const voe_math_float4 *const COLOURS[3] = { &RED, &GREEN, &BLUE };
	static const int EXPECTED[3] = { IS_RED, IS_GREEN, IS_BLUE };
	voe_math_float2 size = { SIDE, SIDE };
	uint32_t frames = 2 * VOE_RENDER_FRAMES_IN_FLIGHT;

	for (uint32_t i = 0; i < frames; i++) {
		const struct voe_render_frame *frame =
			voe_render_frame_current(scene->device);
		voe_render_element fill = solid(
			(voe_math_float4){ 0.0f, 0.0f, SIDE, SIDE },
			*COLOURS[i % 3]);
		int seen;

		if (!open_frame(scene->device))
			return;
		draw_into_target(scene, size, &fill, 1);
		show_on_window(scene,
			       (voe_math_float4){ 0.0f, 0.0f, 1.0f, 1.0f });
		VOE_TEST_CHECK(voe_render_frame_end(scene->device));

		read_back(scene->device, frame, scene->readback.buffer);
		seen = colour_of(scene->pixels);
		printf("frame %u, slot %u: drew %s, window shows %s\n", i,
		       (unsigned)(frame - scene->device->frames),
		       NAMES[EXPECTED[i % 3]], NAMES[seen]);
		VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE,
					    EXPECTED[i % 3]),
				   SIDE * SIDE);
	}
}

static void a_resize_keeps_the_id(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const struct voe_render_target_slot *slot;
	voe_math_float2 size = { 32.0f, 16.0f };
	voe_render_element elements[2] = {
		solid((voe_math_float4){ 0.0f, 0.0f, 32.0f, 16.0f }, GREEN),
		solid((voe_math_float4){ 0.0f, 0.0f, 1.0f, 1.0f }, RED),
	};

	voe_render_target_resize(device, scene->target, 32, 16);
	slot = voe_render_target_at(device, scene->target);
	VOE_TEST_CHECK(slot != NULL);
	if (slot == NULL)
		return;
	// Recorded and not applied until the next frame begins.
	VOE_TEST_CHECK_INT(slot->extent.width, SIDE);

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK_INT(slot->extent.width, 32);
	VOE_TEST_CHECK_INT(slot->extent.height, 16);
	VOE_TEST_CHECK_INT(slot->texture, scene->picture.index);
	VOE_TEST_CHECK_INT(device->textures[scene->picture.index].generation,
			   scene->picture.generation);

	draw_into_target(scene, size, elements, 2);
	show_on_window(scene, (voe_math_float4){ 0.0f, 0.0f, 1.0f, 1.0f });
	VOE_TEST_CHECK(voe_render_frame_end(device));

	read_back(device, frame, scene->readback.buffer);
	// One texel of 32 × 16 over 64 × 64 is two pixels by four, in the
	// top-left corner.
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, 2, 4, IS_RED), 8);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED), 8);

	// Back to the size the other claims were drawn at.
	voe_render_target_resize(device, scene->target, SIDE, SIDE);
}

static void a_sheet_picks_half_the_picture(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_math_float2 size = { SIDE, SIDE };
	voe_render_element elements[2] = {
		solid((voe_math_float4){ 0.0f, 0.0f, SIDE / 2, SIDE }, RED),
		solid((voe_math_float4){ SIDE / 2, 0.0f, SIDE / 2, SIDE }, GREEN),
	};

	if (!open_frame(device))
		return;
	draw_into_target(scene, size, elements, 2);
	show_on_window(scene, (voe_math_float4){ 0.0f, 0.0f, 0.5f, 1.0f });
	VOE_TEST_CHECK(voe_render_frame_end(device));

	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);
}

// Red top-left, green top-right, blue bottom-left, and nothing at all in the
// fourth quarter, so the clear colour shows there. Drawn into whichever target
// it is given, which is what makes the two pictures comparable.
static void draw_quadrants(struct scene *scene, voe_render_target target)
{
	voe_render_device *device = scene->device;
	voe_math_float2 size = { SIDE, SIDE };
	const voe_render_element elements[3] = {
		solid((voe_math_float4){ 0.0f, 0.0f, SIDE / 2, SIDE / 2 }, RED),
		solid((voe_math_float4){ SIDE / 2, 0.0f, SIDE / 2, SIDE / 2 },
		      GREEN),
		solid((voe_math_float4){ 0.0f, SIDE / 2, SIDE / 2, SIDE / 2 },
		      BLUE),
	};
	uint32_t first = voe_render_frame_elements_submitted(device);

	for (uint32_t i = 0; i < 3; i++)
		VOE_TEST_CHECK(voe_render_frame_submit_element(device,
							       elements[i]));

	VOE_TEST_CHECK(voe_render_pass_begin(device, target, NULL));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device, voe_render_element_transform(size), first, 3));
	voe_render_pass_end(device);
}

static const uint8_t *pixel_of(const voe_render_picture *picture, uint32_t x,
			       uint32_t y)
{
	return picture->pixels + ((size_t)y * picture->width + x) * 4;
}

// colour_of's counterpart for what voe_render_target_read hands back, which is
// RGBA and not the card's own order. The difference between the two functions is
// the whole of what the channel swap is claimed to do.
static int rgba_colour_of(const uint8_t *pixel)
{
	unsigned red = pixel[0];
	unsigned green = pixel[1];
	unsigned blue = pixel[2];

	if (red > 128 && red > green && red > blue)
		return IS_RED;
	if (green > 128 && green > red && green > blue)
		return IS_GREEN;
	if (blue > 128 && blue > red && blue > green)
		return IS_BLUE;
	return NEITHER;
}

static void a_target_is_read_back(struct scene *scene, voe_base_arena *arena)
{
	voe_render_device *device = scene->device;
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_render_picture own = { 0 };
	voe_render_picture window = { 0 };
	voe_base_error error = VOE_BASE_OK;

	if (!open_frame(device))
		return;
	draw_quadrants(scene, scene->target);
	draw_quadrants(scene, VOE_RENDER_TARGET_WINDOW);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	VOE_TEST_CHECK(voe_render_target_read(device, scene->target, arena,
					      &own, &error));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &window, &error));
	if (own.pixels == NULL || window.pixels == NULL) {
		voe_base_arena_rewind(arena, mark);
		return;
	}

	VOE_TEST_CHECK_INT(own.width, SIDE);
	VOE_TEST_CHECK_INT(own.height, SIDE);
	VOE_TEST_CHECK_INT(window.width, SIDE);
	VOE_TEST_CHECK_INT(window.height, SIDE);

	// BYTE ORDER, SPELLED OUT AT THE FIRST PIXEL. It is inside the red
	// quadrant, so pixels[0] is the largest of the three and the two after
	// it are not — which is false the moment the swap goes the other way.
	VOE_TEST_CHECK(own.pixels[0] > 128);
	VOE_TEST_CHECK(own.pixels[1] < 128);
	VOE_TEST_CHECK(own.pixels[2] < 128);

	// ORIENTATION: the quadrant drawn at the top of the surface is at row
	// zero, and the one drawn at the bottom is at the last row.
	VOE_TEST_CHECK_INT(rgba_colour_of(pixel_of(&own, 1, 1)), IS_RED);
	VOE_TEST_CHECK_INT(rgba_colour_of(pixel_of(&own, SIDE - 2, 1)),
			   IS_GREEN);
	VOE_TEST_CHECK_INT(rgba_colour_of(pixel_of(&own, 1, SIDE - 2)),
			   IS_BLUE);

	// The fourth quarter is the clear colour, which is none of the three,
	// and it is opaque — the divide by alpha left it exactly as it was.
	VOE_TEST_CHECK_INT(rgba_colour_of(pixel_of(&own, SIDE - 2, SIDE - 2)),
			   NEITHER);
	VOE_TEST_CHECK_INT(pixel_of(&own, SIDE - 2, SIDE - 2)[3], 255);
	VOE_TEST_CHECK_INT(own.pixels[3], 255);

	// ACCEPTANCE CRITERION 6, AS ONE COMPARISON. The same three elements
	// drawn into a target of its own and into the window, on a device with
	// no display at all, come back byte for byte the same.
	VOE_TEST_CHECK_INT(memcmp(own.pixels, window.pixels, IMAGE_BYTES), 0);

	voe_base_arena_rewind(arena, mark);
}

// A target nothing has drawn into is undefined, not an error. Its own device,
// because every target in the scene above has been drawn into by the time the
// claims are made.
static void an_undrawn_target_still_reads(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_target target;
	voe_render_texture texture;
	voe_render_picture picture = { 0 };
	voe_render_device *device;
	voe_base_error error = VOE_BASE_OK;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;

	VOE_TEST_CHECK(voe_render_target_create(device, 8, 8, &target, &texture,
						&error));
	VOE_TEST_CHECK(voe_render_target_read(device, target, arena, &picture,
					      &error));
	VOE_TEST_CHECK_INT(picture.width, 8);
	VOE_TEST_CHECK_INT(picture.height, 8);
	VOE_TEST_CHECK(picture.pixels != NULL);

	// And so is the window's before any frame has ended, which is the other
	// half of the same rule.
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK_INT(picture.width, SIDE);
	VOE_TEST_CHECK_INT(picture.height, SIDE);

	voe_render_device_destroy(device);
}

static void the_capacity_is_a_refusal(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_target target;
	voe_render_texture texture;
	voe_render_device *device;
	voe_base_error error = VOE_BASE_OK;

	room.targets = 0;
	device = voe_render_device_new_headless(arena, size, room, &error);
	VOE_TEST_CHECK(device != NULL);
	if (device != NULL) {
		VOE_TEST_CHECK(!voe_render_target_create(device, 8, 8, &target,
							 &texture, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
		voe_render_device_destroy(device);
	}

	room.targets = 1;
	error = VOE_BASE_OK;
	device = voe_render_device_new_headless(arena, size, room, &error);
	VOE_TEST_CHECK(device != NULL);
	if (device != NULL) {
		VOE_TEST_CHECK(voe_render_target_create(device, 8, 8, &target,
							&texture, &error));
		VOE_TEST_CHECK(!voe_render_target_create(device, 8, 8, &target,
							 &texture, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
		voe_render_device_destroy(device);
	}
}

static bool build_scene(struct scene *scene)
{
	voe_base_error error = VOE_BASE_OK;
	// The whole window at a depth anything passes, corner for corner the
	// front face tests/passes.c draws, with (0, 0) at the top-left.
	voe_render_vertex vertices[4] = {
		{ { -1.0f, 1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	};
	uint32_t indices[6] = { 3, 2, 1, 3, 1, 0 };
	voe_render_shading_values shows = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_render_target_create(scene->device, SIDE, SIDE,
						&scene->target, &scene->picture,
						&error));
	VOE_TEST_CHECK(scene->target.index != VOE_RENDER_TARGET_WINDOW.index);
	VOE_TEST_CHECK(scene->picture.index != VOE_RENDER_NO_TEXTURE);

	shows.base_colour_texture = scene->picture.index;
	VOE_TEST_CHECK(voe_render_shading_create(scene->device, shows,
						 &scene->shows_picture, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(scene->device, vertices, 4,
						  indices, 6,
						  &scene->whole_window, &error));
	return error == VOE_BASE_OK;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

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
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	if (scene.readback.buffer != VK_NULL_HANDLE)
		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene.device->device,
							    scene.readback.memory,
							    0, VK_WHOLE_SIZE, 0,
							    &scene.pixels),
				   VK_SUCCESS);

	if (build_scene(&scene) && scene.pixels != NULL) {
		an_element_shows_the_target(&scene);
		a_mesh_shows_the_target(&scene);
		each_frame_reads_its_own_picture(&scene);
		a_resize_keeps_the_id(&scene);
		a_sheet_picks_half_the_picture(&scene);
		a_target_is_read_back(&scene, arena);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	voe_render_vk.device_wait_idle(scene.device->device);
	if (scene.pixels != NULL)
		voe_render_vk.unmap_memory(scene.device->device,
					   scene.readback.memory);
	voe_render_buffer_teardown(scene.device, &scene.readback);
	voe_render_device_destroy(scene.device);

	an_undrawn_target_still_reads(arena);
	the_capacity_is_a_refusal(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
