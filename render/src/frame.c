// One frame, in three calls: begin waits for the slot and opens a recording,
// draw records one object into it, end submits it and puts it on the window.
// The frame's own clears are not draws — they are the load operations dynamic
// rendering performs as it begins — so what is recorded inside the rendering is
// the draws and the one clear below that is not a load operation.
//
// THAT ONE IS voe_render_frame_clear_depth, AND IT IS THE FOURTH CALL. Card 024
// added it: a caller that wants a group of objects to be in front of everything
// it has already drawn clears depth between the two, mid-frame, inside the same
// rendering block. It is the only command in this file recorded between draws
// that is not itself a draw, and its own comment says why it is not a second
// rendering block.
//
// THREE CALLS AND NOT ONE, BECAUSE THE CALLER IS WHAT KNOWS WHAT TO DRAW. Until
// card 018 this was a single function and the scene was two cubes inside this
// folder; now `3d` walks its tables between the begin and the end, and render
// never learns what a mesh is for.
//
// WHAT IS OPEN BETWEEN THEM LIVES IN THE DEVICE. device->recording says a
// recording is open, device->object_count says how many objects have gone into
// it, device->bound says which of the two pipelines it last bound, and
// device->image_index says which swapchain image _end has to blit into. There is
// exactly one frame open at a time, so a token handed to the caller would be a
// second place for that to live and a second thing to get wrong.
//
// THERE ARE TWO DRAW CALLS AND THEY DIFFER IN ONE ARGUMENT. _draw goes through
// the solid pipeline, _draw_blended through the one that tests depth without
// writing it and blends premultiplied; both are draw_with() below. This file
// does not sort and does not know how to: the order the blended draws arrive in
// is the order they are recorded in, and getting that order right is
// voe_3d_draw_system_run's.
//
// THE CLOCK IS GONE FROM THIS FILE AND SO IS THE CAMERA. Both were here while
// render owned the scene: a frame counted a nominal frame's worth of seconds and
// stepped a camera by what the caller said the person did. The camera is
// `scene`'s now and the clock is the frame loop's, so what arrives here is two
// matrices that somebody else already worked out.
//
// THE CAMERA AND THE SUN ARE ONE BLOCK IN ONE BUFFER, AND _begin IS WHERE THEY
// MEET. A caller hands them over separately because they come from two different
// places; they land adjacent because the shader reads them out of one binding.
// See struct voe_render_frame_block in device_internal.h.
//
// NOTHING HERE DRAWS INTO A SWAPCHAIN IMAGE. The scene goes into images the
// engine owns (target.c) and the swapchain image is written once, by a blit, as
// the last thing a frame does. That separation is what a post-process pass, a
// render resolution different from the window's, and an editor viewport all need
// in order to exist at all.
//
// THE COPY IS A BLIT AND THAT IS A STARTING POINT. vkCmdBlitImage is one call
// and it scales, which is everything this needs. A full-screen quad becomes
// necessary the moment anything wants to run a shader between the target and the
// screen — tone mapping first — and that is the card that replaces this.
//
// THE ONE Y FLIP IN THIS ENGINE IS voe_render_frame_viewport BELOW. Vulkan's
// clip space has +Y pointing down the screen and this engine has +Y up, and the
// whole of the reconciliation is a negative viewport height there. Never a
// negated row in a projection matrix — voe_3d_projection deliberately does not
// have one — and never both: flipping twice looks exactly like flipping none
// until something is culled, and then it is a bug nobody can see. The front-face
// constant that goes with this flip is set on the pipeline in device.c, and the
// pair of them is proven by render/tests/offscreen.c.
//
// DEPTH RUNS BACKWARDS AND THE CLEAR IS THE HALF OF IT THAT LIVES HERE. The
// buffer is cleared to VOE_RENDER_DEPTH_CLEAR, which is 0, which is this
// engine's far plane; the comparison is GREATER, set on the pipeline in device.c;
// and the near plane is at 1.0, which comes out of the projection matrix in `3d`.
// Three files, one convention, and clearing to 1 instead — the habit from every
// tutorial — leaves a depth test that rejects everything. Both clears in this
// file read that one constant, and the number never leaves this folder: no
// caller supplies it and none is told it.
//
// THE DEPTH IMAGE IS NEVER STORED AND NEVER COPIED. Its storeOp is DONT_CARE
// because nothing reads it after the rendering ends: it exists to sort fragments
// within one frame and is rebuilt from the clear on the next — and, since card
// 024, possibly more than once within that frame.
//
// TWO INDICES RUN THROUGH THIS FILE AND THEY ARE NOT INTERCHANGEABLE. A frame
// slot counts how far ahead the CPU is allowed to run and is bounded by
// VOE_RENDER_FRAMES_IN_FLIGHT; a swapchain image index is whatever the driver
// hands back from an acquire and is bounded by the image count it chose. They
// are often both 2 or 3 and that means nothing. Each array is reached through one
// accessor below and each accessor asserts its own bound.
//
// THE FENCE WAIT IS FOR THE FRAME VOE_RENDER_FRAMES_IN_FLIGHT AGO, NOT THE LAST
// ONE, AND THAT GAP IS THE WHOLE OF THE OVERLAP. Waiting on this slot's fence
// leaves every frame submitted since it still running on the GPU. What that
// fence makes safe is this slot's own command buffer, its own acquire semaphore,
// its own target, its own uniform buffer and its own object buffer, and nothing
// else.
//
// THE TWO SEMAPHORE KINDS HAVE DIFFERENT LIFETIMES. The acquire semaphore is per
// slot, guarded by the fence beside it. The rendering-finished semaphore is per
// swapchain image, because present is what waits on it and present hands back no
// fence to say when it stopped. Flattening the two is a race the validation
// layers do not reliably catch.
//
// THE ACQUIRE IS WAITED ON AT THE BLIT AND NOT BEFORE. The first thing a frame
// does to a swapchain image is a transfer, not a colour write, and the scene
// does not touch that image at all — so drawing the target can start while the
// presentation engine is still finished with the image, and only the copy has to
// wait.
//
// A HEADLESS DEVICE RUNS EVERY LINE OF THIS EXCEPT THE THREE THAT NEED A WINDOW:
// there is nothing to acquire, nothing to blit into and nothing to present, so
// _end submits and returns. That is what lets a test drive the same recording
// path the window does and then read the target itself.
//
// THE CARD'S OWN CLOCK IS READ HERE AND IT IS READ ONE LAP LATE. Two timestamps
// are written into this slot's query pool, at the top and the bottom of the
// command buffer, and they are read at the top of the next frame that lands on
// this slot — which is the first moment the fence says the card has finished
// writing them. Reading them any sooner means waiting for the GPU, and a program
// that waits for the GPU in order to time the GPU is timing something else. See
// read_gpu_time below and voe_render_frame_gpu_time.
//
// A SWAPCHAIN GOES STALE AND THAT IS ORDINARY. Out-of-date means the surface
// changed under us and the swapchain has to be built again; suboptimal means it
// still works but no longer matches. Both are answered by rebuilding, neither is
// an error to report, and both happen for real on a compositor that resizes the
// client area when the titlebar goes away.
#include "device_internal.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// The colour behind everything drawn. It is deliberately none of the colours a
// test's geometry wears, so that a person looking at the window can tell the
// background from the things in front of it — and so that
// render/tests/offscreen.c can take the clear colour out of a corner of the
// picture and count what differs from it.
//
// IT IS A LINEAR COLOUR AND NOT THE BYTES THAT REACH THE SCREEN. The target is
// an sRGB format, so the hardware encodes whatever is written into it — a clear
// of 0.5 arrives on screen as a byte of about 188 and not 128. These three
// numbers are the linear form of the same slate blue this was before card 019,
// which is why they are not the round numbers they used to be: a clear colour
// written as if it were sRGB comes out of an sRGB target visibly washed out.
#define CLEAR_RED 0.00304f
#define CLEAR_GREEN 0.08438f
#define CLEAR_BLUE 0.11954f

// The only place a frame slot indexes anything, and the only place an image
// index does. Both asserts are the same mistake read from either end: a slot is
// not an image index and an image index is not a slot, and with arrays this
// short a swap lands in range as often as not.
static struct voe_render_frame *frame_at(voe_render_device *device, uint32_t slot)
{
	VOE_BASE_DEBUG_ASSERT(slot < VOE_RENDER_FRAMES_IN_FLIGHT,
			      "a per-slot array reached with something that is not a frame slot");
	return &device->frames[slot];
}

static struct voe_render_image *image_at(voe_render_device *device,
					 uint32_t index)
{
	VOE_BASE_DEBUG_ASSERT(index < device->image_count,
			      "a per-image array reached with something that is not a swapchain image index");
	return &device->images[index];
}

const struct voe_render_frame *
voe_render_frame_current(const voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "asking no device for its frame slot");

	return frame_at((voe_render_device *)device, device->slot);
}

VkViewport voe_render_frame_viewport(VkExtent2D extent)
{
	// y at the bottom and a negative height: the flip, and the only one.
	//
	// The depth range stays the plain 0..1 identity, and it is not where this
	// engine's reversed depth lives. That comes out of the projection matrix,
	// which puts the near plane at 1.0 and the far plane at 0.0; the viewport
	// maps clip depth to the range a depth buffer stores, and 0..1 is the
	// whole of that range. Reversing it here as well would reverse it twice.
	VkViewport viewport = {
		.y = (float)extent.height,
		.width = (float)extent.width,
		.height = -(float)extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	return viewport;
}

void voe_render_frame_set_viewport(voe_render_device *device,
				   VkViewport viewport)
{
	VOE_BASE_ASSERT(device != NULL, "setting a viewport on no device");
	VOE_BASE_ASSERT(device->recording,
			"setting a viewport with no frame open");

	voe_render_vk.cmd_set_viewport(frame_at(device, device->slot)->commands,
				       0, 1, &viewport);
}

// The pair of timestamps this slot wrote the last time round, turned into
// seconds. Called after the fence wait and before anything overwrites the pool,
// which is the one window in which the numbers are both finished and still
// there.
//
// THE READINGS ARE MASKED BEFORE THEY ARE SUBTRACTED. A queue is allowed to
// report as few as 36 valid bits, and the bits above those hold rubbish rather
// than zeroes — so subtracting two raw readings on such a queue gives a number
// with no relationship to time at all. Masking both down to the bits that carry
// a value is what makes the subtraction mean something.
//
// AN END BELOW A START IS ONE WRAP AND NOT A FAILURE. A counter of 36 bits at a
// nanosecond a tick comes round about once a minute, so a frame that straddles
// the wrap is a thing that really happens rather than a thing to guard against
// on paper. Adding one whole counter back is the only reading of it that is
// right; the alternative is a negative duration, which voe_base_samples asserts
// on and rightly.
static void read_gpu_time(voe_render_device *device,
			  const struct voe_render_frame *frame)
{
	uint64_t stamps[VOE_RENDER_TIMESTAMPS_PER_FRAME];
	uint64_t mask = UINT64_MAX;
	uint64_t ticks;

	if (!device->timestamps || !frame->timed)
		return;

	// No WAIT bit: the fence has already said the card is finished with this
	// slot, so the results are there. VK_NOT_READY would mean they are not,
	// and the honest answer to that is to keep the previous measurement
	// rather than to invent one.
	if (voe_render_vk.get_query_pool_results(device->device,
						 frame->timestamps, 0,
						 VOE_RENDER_TIMESTAMPS_PER_FRAME,
						 sizeof(stamps), stamps,
						 sizeof(stamps[0]),
						 VK_QUERY_RESULT_64_BIT) !=
	    VK_SUCCESS)
		return;

	if (device->timestamp_valid_bits < 64)
		mask = ((uint64_t)1 << device->timestamp_valid_bits) - 1;

	stamps[0] &= mask;
	stamps[1] &= mask;

	if (stamps[1] >= stamps[0])
		ticks = stamps[1] - stamps[0];
	else
		ticks = (mask - stamps[0]) + stamps[1] + 1;

	// The period is nanoseconds per tick, and the engine's unit is the
	// second.
	device->gpu_seconds = (double)ticks *
			      (double)device->timestamp_period / 1e9;
	device->gpu_measured = true;
}

// The barriers, the clears and everything a draw needs bound. Recorded once per
// frame, because none of it differs between two draws in the same frame — the
// object number a draw pushes is the only thing that does.
static void open_rendering(voe_render_device *device,
			   const struct voe_render_frame *frame)
{
	// Two images into the layouts the rendering needs. The colour barrier is
	// index 0 throughout this file, because close_rendering reuses it to
	// move the colour image on again and the depth image needs no second
	// transition.
	VkImageMemoryBarrier2 barriers[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = frame->target.colour.image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		},
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			// EARLY_FRAGMENT_TESTS is where the depth clear and the
			// depth test happen, so it is the stage that has to wait
			// for this transition rather than the colour output one.
			.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = frame->target.depth.image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 2,
		.pImageMemoryBarriers = barriers,
	};
	VkRenderingAttachmentInfo colour = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->target.colour.view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .color = { .float32 = { CLEAR_RED, CLEAR_GREEN,
						       CLEAR_BLUE, 1.0f } } },
	};
	// Cleared to the far plane, which is 0 here, and thrown away afterwards.
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = frame->target.depth.view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = device->resolution },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour,
		.pDepthAttachment = &depth,
	};
	// The scissor is the whole target and takes no part in the flip. It is
	// in framebuffer coordinates, which have no sign to get wrong.
	VkRect2D scissor = { .extent = device->resolution };
	VkViewport viewport = voe_render_frame_viewport(device->resolution);
	VkDeviceSize vertex_offset = 0;

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	// Both clears are load operations, so they have already happened by the
	// time the first command inside is recorded.
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);
	// The solid pipeline, because a frame's opaque and cutout draws come
	// first; a blended draw binds the other one and `bound` is what keeps a
	// run of either kind to a single bind.
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline);
	device->bound = device->pipeline;
	voe_render_vk.cmd_bind_descriptor_sets(frame->commands,
					       VK_PIPELINE_BIND_POINT_GRAPHICS,
					       device->layout, 0, 1,
					       &frame->descriptor, 0, NULL);

	// The two pools, bound once for the whole frame. Every mesh is a range
	// inside them, which is the property that makes one bind enough.
	voe_render_vk.cmd_bind_vertex_buffers(frame->commands, 0, 1,
					      &device->vertices.buffer.buffer,
					      &vertex_offset);
	voe_render_vk.cmd_bind_index_buffer(frame->commands,
					    device->indices.buffer.buffer, 0,
					    VK_INDEX_TYPE_UINT32);
}

// Ends the rendering and leaves the colour image ready to be copied out of, by
// whoever asked for the drawing. A frame blits it into a swapchain image; a test
// copies it into memory it can read. ALL_TRANSFER and not the blit alone,
// because those are two different stages and this barrier has to cover both —
// naming one of them leaves the other reading an image this dependency does not
// reach, which synchronisation validation reports and nothing else does.
//
// The depth image gets no second barrier: nothing reads it, so there is no later
// access for one to order against.
static void close_rendering(const struct voe_render_frame *frame)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = frame->target.colour.image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};

	voe_render_vk.cmd_end_rendering(frame->commands);
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

// The target onto the screen, and the whole of what the swapchain image is for.
// The two extents are the same number today and the blit still reads both, so
// that the day the target stops being the window's size this needs no edit.
static void blit_to_screen(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   const struct voe_render_image *image)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image->image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrier,
	};
	VkImageBlit region = {
		.srcSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.srcOffsets = { { 0, 0, 0 },
				{ (int32_t)device->resolution.width,
				  (int32_t)device->resolution.height, 1 } },
		.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.dstOffsets = { { 0, 0, 0 },
				{ (int32_t)device->extent.width,
				  (int32_t)device->extent.height, 1 } },
	};

	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);

	// NEAREST, which does nothing at all while the two extents match and is
	// the engine's rule the moment they do not. A linear blit is a smoothing
	// filter over the whole finished frame — the largest piece of
	// antialiasing there could be here — and this engine does not want
	// antialiasing anywhere. A render resolution below the window's will
	// therefore show square pixels rather than a blur, which is the intended
	// picture and not a fault in the blit.
	voe_render_vk.cmd_blit_image(frame->commands, frame->target.colour.image,
				     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     image->image,
				     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				     &region, VK_FILTER_NEAREST);

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	voe_render_vk.cmd_pipeline_barrier2(frame->commands, &dependency);
}

// Where the two lifetimes meet: this waits on the slot's acquire semaphore,
// signals the image's rendering-finished one, and signals the slot's fence. A
// headless device has neither semaphore and waits for and signals nothing but
// the fence.
static bool submit(voe_render_device *device,
		   const struct voe_render_frame *frame,
		   const struct voe_render_image *image)
{
	VkCommandBufferSubmitInfo commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.commandBuffer = frame->commands,
	};
	// The blit, because that is the first thing in this submit that touches
	// the acquired image, and it is the stage the barriers around it name.
	// Everything before it draws into a target of our own and has no reason
	// to wait for the presentation engine at all.
	VkSemaphoreSubmitInfo wait = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.semaphore = frame->acquired,
		.stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT,
	};
	VkSemaphoreSubmitInfo signal = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	};
	VkSubmitInfo2 info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &commands,
	};
	VkResult result;

	if (image != NULL) {
		signal.semaphore = image->drawn;
		info.waitSemaphoreInfoCount = 1;
		info.pWaitSemaphoreInfos = &wait;
		info.signalSemaphoreInfoCount = 1;
		info.pSignalSemaphoreInfos = &signal;
	}

	result = voe_render_vk.queue_submit2(device->queue, 1, &info,
					     frame->submitted);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkQueueSubmit2 failed (VkResult %d)\n",
			(int)result);
		return false;
	}
	return true;
}

// The targets and the swapchain are built from the same window size and rebuilt
// together, because the target is the resolution and the swapchain is where it
// lands. Order matters only in that a target that cannot be made is a device
// that cannot draw, and there is no point building a swapchain for it. On a
// headless device the second call does nothing.
static bool rebuild(voe_render_device *device, voe_platform_size size)
{
	if (!voe_render_target_build(device, size))
		return false;
	return voe_render_swapchain_build(device, size);
}

bool voe_render_frame_begin(voe_render_device *device, voe_platform_size size,
			    voe_render_view view, voe_render_light light,
			    bool *drawing)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { .camera = view,
					       .light = light };
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkResult result;

	VOE_BASE_ASSERT(device != NULL, "beginning a frame on no device");
	VOE_BASE_ASSERT(drawing != NULL, "beginning a frame with nowhere to say so");
	VOE_BASE_ASSERT(!device->recording,
			"beginning a frame while one is already open — every begin needs its end");

	*drawing = false;

	// A window with no area has no images and nothing to present. Skipped,
	// not failed — it comes back the moment the window does.
	if (size.width <= 0 || size.height <= 0)
		return true;

	if (device->rebuild ||
	    (!device->headless && device->swapchain == VK_NULL_HANDLE) ||
	    size.width != device->built.width ||
	    size.height != device->built.height) {
		if (!rebuild(device, size))
			return false;
	}
	if (!device->headless && device->swapchain == VK_NULL_HANDLE)
		return true;

	// The slot this frame is, and the last thing indexed until the acquire
	// below hands back an image.
	frame = frame_at(device, device->slot);

	// The fence is waited on before the acquire and reset after it, so that
	// an acquire that fails leaves the fence signalled. Resetting first
	// would leave a fence nothing will ever signal, and the next frame would
	// wait on it forever.
	//
	// This is the wait for the frame VOE_RENDER_FRAMES_IN_FLIGHT ago. Every
	// frame submitted since is still running, and that is where the overlap
	// this file exists for comes from.
	voe_render_vk.wait_for_fences(device->device, 1, &frame->submitted,
				      VK_TRUE, UINT64_MAX);

	// The fence just waited on and the semaphore about to be handed to the
	// acquire have to belong to the same slot, or the semaphore is being
	// reused while a submit may still be waiting on it. Asking the driver
	// says so where reading the code only claims it.
	VOE_BASE_DEBUG_ASSERT(voe_render_vk.get_fence_status(device->device,
							     frame->submitted) ==
			      VK_SUCCESS,
			      "beginning a frame on a slot whose last submit has not finished");

	// Before the reset and the recording below, because both of those are
	// what will overwrite what is being read. The fence above is what makes
	// it safe to read at all.
	read_gpu_time(device, frame);

	if (!device->headless) {
		result = voe_render_vk.acquire_next_image(device->device,
							  device->swapchain,
							  UINT64_MAX,
							  frame->acquired,
							  VK_NULL_HANDLE,
							  &device->image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			device->rebuild = true;
			return true;
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			fprintf(stderr,
				"render: vkAcquireNextImageKHR failed (VkResult %d)\n",
				(int)result);
			return false;
		}
		// Suboptimal is still a usable image, so this frame is drawn and
		// the rebuild waits until it has been presented.
		if (result == VK_SUBOPTIMAL_KHR)
			device->rebuild = true;
	}

	voe_render_vk.reset_fences(device->device, 1, &frame->submitted);
	voe_render_vk.reset_command_buffer(frame->commands, 0);

	// The camera and the sun, into this slot's own buffer. Safe because the
	// fence above says the GPU has finished reading what was in here two
	// frames ago.
	VOE_BASE_DEBUG_ASSERT(frame->uniforms_mapped != NULL,
			      "beginning a frame whose uniform buffer is not mapped");
	memcpy(frame->uniforms_mapped, &block, sizeof(block));

	voe_render_vk.begin_command_buffer(frame->commands, &begin);

	// The reset has to be outside a rendering and it has to come before the
	// writes, because a query pool is created and left in an undefined state
	// rather than an empty one and a query written twice without a reset
	// between is undefined. Here is the only place both are true.
	if (device->timestamps) {
		voe_render_vk.cmd_reset_query_pool(frame->commands,
						   frame->timestamps, 0,
						   VOE_RENDER_TIMESTAMPS_PER_FRAME);
		// TOP_OF_PIPE, which for a timestamp means as soon as this
		// command is reached in submission order — the earliest point in
		// this frame's work that the card can name.
		voe_render_vk.cmd_write_timestamp2(frame->commands,
						   VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
						   frame->timestamps, 0);
	}

	open_rendering(device, frame);

	device->recording = true;
	device->object_count = 0;
	*drawing = true;
	return true;
}

// The whole of both draw calls; `pipeline` is the only thing that differs
// between them.
//
// THE BIND IS CONDITIONAL AND THAT IS THE ONLY REASON `bound` EXISTS. A frame is
// a run of solid draws and then a run of blended ones, so this costs one bind at
// the boundary rather than one per draw — and it is still correct if a caller
// ever interleaves them, which is what makes it a condition and not an
// assumption about the caller's order.
static bool draw_with(voe_render_device *device, voe_render_geometry geometry,
		      voe_render_object object, VkPipeline pipeline)
{
	const struct voe_render_geometry_slot *slot;
	struct voe_render_frame *frame;

	VOE_BASE_ASSERT(device != NULL, "drawing on no device");
	VOE_BASE_ASSERT(device->recording,
			"drawing with no frame open — voe_render_frame_begin said there was nothing to draw into, or _end has already run");

	slot = voe_render_geometry_at(device, geometry);
	if (slot == NULL) {
		fprintf(stderr,
			"render: a draw named mesh %u generation %u, which is not a mesh this device handed out\n",
			geometry.index, geometry.generation);
		return false;
	}

	if (device->object_count >= device->capacities.objects) {
		fprintf(stderr,
			"render: this frame already holds %u objects, which is what the device was made for\n",
			device->capacities.objects);
		return false;
	}

	frame = frame_at(device, device->slot);

	if (device->bound != pipeline) {
		voe_render_vk.cmd_bind_pipeline(frame->commands,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline);
		device->bound = pipeline;
	}

	// The record, into this slot's own object buffer at this object's
	// number. Written rather than staged because the buffer is host-visible
	// and this slot's; the fence at the top of the frame is what makes that
	// safe.
	memcpy((unsigned char *)frame->objects_mapped +
		       (size_t)device->object_count * sizeof(object),
	       &object, sizeof(object));

	// The object's number, and the only push constant left in this engine.
	// The shader reads its own record out of the buffer with it — which is
	// also why this stops being a push constant the day the draws become
	// indirect: an indirect draw's shader reads the same number out of its
	// instance index instead.
	voe_render_vk.cmd_push_constants(frame->commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(device->object_count),
					 &device->object_count);

	// first_vertex is the vertexOffset rather than something added to the
	// indices on the way in, which is what lets a mesh keep the numbering
	// its file gave it.
	voe_render_vk.cmd_draw_indexed(frame->commands, slot->index_count, 1,
				       slot->first_index,
				       (int32_t)slot->first_vertex, 0);

	device->object_count++;
	return true;
}

bool voe_render_frame_draw(voe_render_device *device,
			   voe_render_geometry geometry,
			   voe_render_object object)
{
	VOE_BASE_ASSERT(device != NULL, "drawing on no device");
	return draw_with(device, geometry, object, device->pipeline);
}

bool voe_render_frame_draw_blended(voe_render_device *device,
				   voe_render_geometry geometry,
				   voe_render_object object)
{
	VOE_BASE_ASSERT(device != NULL, "drawing on no device");
	return draw_with(device, geometry, object, device->pipeline_blended);
}

// The overlay's depth clear: one command into the rendering block that is
// already open, and the whole of what card 024 needed from this folder.
//
// vkCmdClearAttachments AND NOT A SECOND RENDERING BLOCK. Ending the rendering
// and beginning it again would work and would cost a second set of load and
// store operations, a second transition of both images, and a colour attachment
// that has to be reloaded rather than kept — for a clear that this one command
// performs inside the block the frame already has open. The shape ADR-0051 chose
// is one rendering block per frame and this does not change it.
//
// IT IS ORDERED AGAINST THE DRAWS AROUND IT AND THAT IS WHY THIS WORKS AT ALL. A
// clear inside a rendering block executes in command order like a draw, so
// everything recorded before this sees the depth buffer it wrote and everything
// recorded after it sees an empty one. It is not a load operation and does not
// happen at the top of the frame.
//
// NO PIPELINE STATE REACHES IT. It clears the attachment directly rather than
// through a pipeline, so the blended pipeline's depth write being off does not
// hold it back — which is what lets it be called after a run of blended draws.
//
// THE DEPTH ASPECT ONLY, AND THE SAME VALUE THE LOAD OP USES. Naming the colour
// attachment here would throw away the world's picture, which is the one way
// this call can be badly wrong, and VOE_RENDER_DEPTH_CLEAR is read from the same
// constant open_rendering reads so the two cannot drift apart.
void voe_render_frame_clear_depth(voe_render_device *device)
{
	VkClearAttachment attachment = {
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	// The whole target, which is the rect the scissor is already set to.
	// Framebuffer coordinates, so the viewport's Y flip takes no part in it.
	// Its extent is filled in below rather than here, because reading it is
	// already a dereference and the assert has not run yet.
	VkClearRect rect = { .baseArrayLayer = 0, .layerCount = 1 };

	VOE_BASE_ASSERT(device != NULL, "clearing depth on no device");
	VOE_BASE_ASSERT(device->recording,
			"clearing depth with no frame open — voe_render_frame_begin said there was nothing to draw into, or _end has already run");

	rect.rect.extent = device->resolution;

	voe_render_vk.cmd_clear_attachments(frame_at(device, device->slot)->commands,
					    1, &attachment, 1, &rect);
}

bool voe_render_frame_end(voe_render_device *device)
{
	struct voe_render_frame *frame;
	struct voe_render_image *image = NULL;
	VkResult result;
	VkPresentInfoKHR present = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.swapchainCount = 1,
		.pImageIndices = &device->image_index,
	};

	VOE_BASE_ASSERT(device != NULL, "ending a frame on no device");
	VOE_BASE_ASSERT(device->recording,
			"ending a frame that was never begun");

	frame = frame_at(device, device->slot);
	if (!device->headless)
		image = image_at(device, device->image_index);

	close_rendering(frame);
	if (image != NULL)
		blit_to_screen(device, frame, image);

	// ALL_COMMANDS, which for a timestamp means once everything submitted
	// before it has finished — so what lies between this and the one in
	// _begin is the whole of this frame's work on the card, the blit
	// included. After the blit and not before it, for that reason.
	if (device->timestamps)
		voe_render_vk.cmd_write_timestamp2(frame->commands,
						   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
						   frame->timestamps, 1);

	voe_render_vk.end_command_buffer(frame->commands);

	// Closed before the submit, so that a submit that fails does not leave a
	// recording open for the next frame to assert on.
	device->recording = false;

	if (!submit(device, frame, image))
		return false;

	// The pool holds a pair worth reading from here on, and not before: a
	// frame whose submit failed recorded the writes and never ran them, so a
	// read would report VK_NOT_READY for the rest of the program's life.
	frame->timed = device->timestamps;

	// The slot is spent the moment the submit lands, and not before: a frame
	// that turned back in _begin never put this slot in flight and has to
	// come back to it with its fence still signalled.
	device->slot = (device->slot + 1) % VOE_RENDER_FRAMES_IN_FLIGHT;

	if (image == NULL)
		return true;

	present.pWaitSemaphores = &image->drawn;
	present.pSwapchains = &device->swapchain;
	result = voe_render_vk.queue_present(device->queue, &present);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		device->rebuild = true;
		return true;
	}
	if (result != VK_SUCCESS) {
		fprintf(stderr, "render: vkQueuePresentKHR failed (VkResult %d)\n",
			(int)result);
		return false;
	}

	return true;
}
