// One frame: voe_render_frame_begin waits for the slot and opens a recording,
// the caller opens passes into it (pass.c) and draws into those (draw.c), and
// voe_render_frame_end closes the recording, puts the picture on the window
// (present.c), submits and presents. This file also reads the GPU time a slot
// measured, and rebuilds the targets and the swapchain when the window resizes.
//
// THREE CALLS AND NOT ONE, BECAUSE THE CALLER IS WHAT KNOWS WHAT TO DRAW. Until
// card 018 this was a single function and the scene was two cubes inside this
// folder; now `3d` walks its tables between the begin and the end, and render
// never learns what a mesh is for.
//
// WHAT IS OPEN BETWEEN THEM LIVES IN THE DEVICE. device->recording says a
// recording is open, device->pass_open that a pass is, device->pass_count how
// many passes the frame has opened, device->window_cleared whether one of them
// cleared the window, device->object_count says how many objects have gone into
// it, device->element_count says how many elements have been submitted to it,
// device->draw_commands says how many draw commands it holds, device->bound says
// which pipeline it last bound, device->bound_transient says which of the two
// geometry pool pairs it last bound, and device->image_index says which swapchain
// image _end has to blit into. _begin resets them; pass.c, draw.c and element.c
// write them in between. There is exactly one frame open at a time, so a token
// handed to the caller would be a second place for that to live and a second
// thing to get wrong.
//
// THE CLOCK IS GONE FROM THIS FILE AND SO IS THE CAMERA. Both were here while
// render owned the scene: a frame counted a nominal frame's worth of seconds and
// stepped a camera by what the caller said the person did. The camera is
// `scene`'s now and the clock is the frame loop's, so what arrives here is two
// matrices that somebody else already worked out.
//
// THE TWO SEMAPHORE KINDS HAVE DIFFERENT LIFETIMES. The acquire semaphore is per
// slot, guarded by the fence beside it. The rendering-finished semaphore is per
// swapchain image, because present is what waits on it and present hands back no
// fence to say when it stopped. Flattening the two is a race the validation
// layers do not reliably catch.
//
// A HEADLESS DEVICE RUNS EVERY LINE OF THIS EXCEPT THE THREE THAT NEED A WINDOW:
// there is nothing to acquire, nothing to blit into and nothing to present, so
// _end submits and returns. That is what lets a test drive the same recording
// path the window does and then read the target itself.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

// The only place a frame slot indexes anything, and the only place an image
// index does. Both asserts are the same mistake read from either end: a slot is
// not an image index and an image index is not a slot, and with arrays this
// short a swap lands in range as often as not.
//
// TWO INDICES RUN THROUGH A FRAME AND THEY ARE NOT INTERCHANGEABLE. A frame
// slot counts how far ahead the CPU is allowed to run and is bounded by
// VOE_RENDER_FRAMES_IN_FLIGHT; a swapchain image index is whatever the driver
// hands back from an acquire and is bounded by the image count it chose. They
// are often both 2 or 3 and that means nothing. Each array is reached through
// one accessor, voe_render_frame_at or image_at, and each asserts its own bound;
// pass.c and draw.c reach a slot through the first.
struct voe_render_frame *voe_render_frame_at(voe_render_device *device,
					     uint32_t slot)
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

	return voe_render_frame_at((voe_render_device *)device, device->slot);
}

struct voe_render_frame *voe_render_frame_open(voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device for its open frame");
	VOE_BASE_ASSERT(device->recording,
			"asking for the open frame when none is open");

	return voe_render_frame_at(device, device->slot);
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
//
// THE CARD'S OWN CLOCK IS READ HERE AND IT IS READ ONE LAP LATE. Two timestamps
// are written into this slot's query pool, at the top and the bottom of the
// command buffer, and they are read at the top of the next frame that lands on
// this slot — which is the first moment the fence says the card has finished
// writing them. Reading them any sooner means waiting for the GPU, and a program
// that waits for the GPU in order to time the GPU is timing something else. See
// voe_render_frame_gpu_time.
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

// Where the two lifetimes meet: this waits on the slot's acquire semaphore,
// signals the image's rendering-finished one, and signals the slot's fence. A
// headless device has neither semaphore and waits for and signals nothing but
// the fence.
//
// THE ACQUIRE IS WAITED ON AT THE BLIT AND NOT BEFORE. The first thing a frame
// does to a swapchain image is a transfer, not a colour write, and the scene
// does not touch that image at all — so drawing the target can start while the
// presentation engine is still finished with the image, and only the copy has to
// wait.
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
		VOE_BASE_ERROR("render", "vkQueueSubmit2 failed (VkResult %d)",
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
//
// A SWAPCHAIN GOES STALE AND THAT IS ORDINARY. Out-of-date means the surface
// changed under us and the swapchain has to be built again; suboptimal means it
// still works but no longer matches. Both are answered by rebuilding, neither is
// an error to report, and both happen for real on a compositor that resizes the
// client area when the titlebar goes away.
static bool rebuild(voe_render_device *device, voe_platform_size size)
{
	if (!voe_render_target_build(device, size))
		return false;
	return voe_render_swapchain_build(device, size);
}

// device->element_count AND device->draw_commands ARE RESET HERE AND WRITTEN
// ELSEWHERE. element.c is what submits
// an element and what draws them, because that path has no mesh, no pool and no
// object record in it; _begin puts both counters back to nought because _begin
// is what the fence has made this slot's buffers safe at. A third pipeline
// therefore exists that this file never binds — see device->bound, which is
// compared and not switched on, so an element draw leaving its own pipeline
// bound simply makes the next mesh draw rebind.
//
// THE TRANSIENT RESET HAPPENS HERE, AFTER THE FENCE, AND THAT ORDER IS THE WHOLE
// OF ITS SAFETY. voe_render_geometry_frame_reset empties this slot's transient
// pools and makes every transient id from last frame stale; it runs once the
// slot's fence says the card has finished with the slot, because the memory it
// empties is what the card was reading. See geometry.c for what the reset does.
//
// THE FENCE WAIT IS FOR THE FRAME VOE_RENDER_FRAMES_IN_FLIGHT AGO, NOT THE LAST
// ONE, AND THAT GAP IS THE WHOLE OF THE OVERLAP. Waiting on this slot's fence
// leaves every frame submitted since it still running on the GPU. What that
// fence makes safe is this slot's own command buffer, its own acquire semaphore,
// its own target, its own uniform buffer, its own object buffer and its own
// element buffer, and nothing else.
bool voe_render_frame_begin(voe_render_device *device, voe_platform_size size,
			    bool *drawing)
{
	struct voe_render_frame *frame;
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
	// Beside the window's rebuild and for the same reason: the top of a
	// frame is where nothing is recording, and it waits for the card only
	// when a target's size has actually changed.
	if (!voe_render_targets_apply_resizes(device) ||
	    !voe_render_bounce_volumes_apply(device))
		return false;
	if (!device->headless && device->swapchain == VK_NULL_HANDLE)
		return true;

	// The slot this frame is, and the last thing indexed until the acquire
	// below hands back an image.
	frame = voe_render_frame_at(device, device->slot);

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

	// Last frame's transient geometry goes stale and this slot's transient
	// pools go back to empty. After the fence above and nowhere else: the
	// pools are what the card was reading two frames ago, and the fence is
	// what says it has stopped.
	voe_render_geometry_frame_reset(device, frame);

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
			VOE_BASE_ERROR("render",
				       "vkAcquireNextImageKHR failed (VkResult %d)",
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

	device->recording = true;
	// No pass is open, none has been, and the window has not been cleared:
	// the first pass onto it will.
	device->pass_open = false;
	device->pass_camera = false;
	device->pass_bounce = false;
	frame->bounced = false;
	device->pass_count = 0;
	device->window_cleared = false;
	device->window_volume.begun[device->slot].begun = false;
	device->bounce_begun = false;
	device->capture_passes = 0;
	for (uint32_t i = 0; i < device->capacities.targets; i++) {
		device->targets[i].cleared = false;
		device->targets[i].volume.begun[device->slot].begun = false;
	}
	device->object_count = 0;
	// Both start again with the frame: the elements because this slot's
	// record buffer is written from the top, and the draw count because it
	// is a measurement of one frame and not a running total.
	device->element_count = 0;
	device->draw_commands = 0;
	*drawing = true;
	return true;
}

bool voe_render_frame_is_open(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device whether a frame is open");
	return device->recording;
}

// A FRAME WITH NO PASS ONTO THE WINDOW STILL CLEARS IT. _end records an empty
// rendering block with the clear load operations when no pass has, so the image
// presented is the clear colour and not whatever the slot held last lap.
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
	// A bounce pass left open is closed here, as the next pass would close it.
	if (device->pass_open && device->pass_bounce)
		voe_render_pass_end(device);
	VOE_BASE_ASSERT(!device->pass_open,
			"ending a frame with a pass still open — every _pass_begin needs its _pass_end first");

	frame = voe_render_frame_at(device, device->slot);
	if (!device->headless)
		image = image_at(device, device->image_index);

	// No pass cleared the window, so an empty block does: what is presented
	// is the clear colour and not what this slot's target held last lap.
	if (!device->window_cleared) {
		voe_render_open_rendering(frame->commands, &frame->target,
					  device->resolution, true, false);
		voe_render_vk.cmd_end_rendering(frame->commands);
		device->window_cleared = true;
	}
	voe_render_ready_for_copy(frame);
	if (image != NULL)
		voe_render_blit_to_screen(device, frame, image);

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

	// voe_render_ready_for_copy above left the slot just spent in
	// TRANSFER_SRC_OPTIMAL, which is what voe_render_target_read needs of
	// the window's picture.
	device->frame_ended = true;

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
		VOE_BASE_ERROR("render", "vkQueuePresentKHR failed (VkResult %d)",
			       (int)result);
		return false;
	}

	return true;
}
