// Every pass timed and labelled (ADR-0367 points 1 and 2), and the frame
// breakdown read back. voe_render_pass_start (pass.c) calls _open with the
// pass's name, voe_render_pass_end calls _close, frame.c calls _read beside its
// own GPU time once the slot's fence is down, and voe_render_frame_pass_times
// copies the newest breakdown out. Declared in device_calls.h.
//
// A PASS IS A PAIR IN THE SLOT'S POOL. The frame's own pair is at 0 and 1; the
// i-th pass a frame times writes 2 + 2i as it opens and 3 + 2i as it closes, and
// its name goes into the slot's pass_names[i]. Both stamps are ALL_COMMANDS: each
// is written once everything before it has finished, so the passes do not
// overlap and their sum is not above the frame's pair.
//
// THE SAME NAME IS THE PASS'S DEBUG LABEL, begun before the first stamp and ended
// after the second, so a capture tool and the breakdown read the same words.
//
// Constraints: the pool has room for voe_render_timed_passes passes; a pass past
// it is labelled and not timed, never an assert. A card without timestamps has
// labels only and a breakdown of nought. The read asks the pool once per pass, a
// small loop at the top of a frame; one call over the whole range would lift it.
#include "device_internal.h"

#include <render/device.h>

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

double voe_render_timestamp_seconds(const voe_render_device *device,
				    uint64_t start, uint64_t end)
{
	uint64_t mask = UINT64_MAX;
	uint64_t ticks;

	VOE_BASE_DEBUG_ASSERT(device != NULL, "timing on no device");
	VOE_BASE_DEBUG_ASSERT(device->timestamps,
			      "turning timestamps into seconds on a card that writes none");

	if (device->timestamp_valid_bits < 64)
		mask = ((uint64_t)1 << device->timestamp_valid_bits) - 1;
	start &= mask;
	end &= mask;
	// An end below a start is one wrap of the counter, not a failure.
	ticks = end >= start ? end - start : (mask - start) + end + 1;
	return (double)ticks * (double)device->timestamp_period / 1e9;
}

void voe_render_pass_timing_open(voe_render_device *device,
				 struct voe_render_frame *frame, const char *name)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL && frame != NULL,
			      "timing a pass on no device or no frame");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "timing a pass with no name");

	voe_render_debug_label_begin(frame->commands, name);
	frame->pass_timing = false;
	if (!device->timestamps ||
	    frame->pass_timed >= voe_render_timed_passes(device))
		return;
	(void)snprintf(frame->pass_names[frame->pass_timed],
		       VOE_RENDER_PASS_NAME, "%s", name);
	voe_render_vk.cmd_write_timestamp2(frame->commands,
					   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
					   frame->timestamps,
					   VOE_RENDER_FRAME_TIMESTAMPS +
						   2 * frame->pass_timed);
	frame->pass_timing = true;
}

void voe_render_pass_timing_close(voe_render_device *device,
				  struct voe_render_frame *frame)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL && frame != NULL,
			      "closing a pass's timing on no device or no frame");

	if (frame->pass_timing) {
		voe_render_vk.cmd_write_timestamp2(frame->commands,
						   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
						   frame->timestamps,
						   VOE_RENDER_FRAME_TIMESTAMPS +
							   2 * frame->pass_timed + 1);
		frame->pass_timed++;
		frame->pass_timing = false;
	}
	voe_render_debug_label_end(frame->commands);
	VOE_BASE_DEBUG_ASSERT(frame->pass_timed <= voe_render_timed_passes(device),
			      "timed more passes than the pool has room for");
}

// No WAIT bit, as frame.c's read: the fence has said the card is done. A pair
// that will not read leaves a breakdown of nought rather than half of one.
void voe_render_pass_timing_read(voe_render_device *device,
				 const struct voe_render_frame *frame)
{
	uint64_t stamps[2];

	VOE_BASE_DEBUG_ASSERT(device != NULL && frame != NULL,
			      "reading pass times on no device or no frame");
	VOE_BASE_DEBUG_ASSERT(frame->pass_timed <= voe_render_timed_passes(device),
			      "a frame that timed more passes than the pool has room for");

	device->pass_time_count = 0;
	for (uint32_t i = 0; i < frame->pass_timed; i++) {
		if (voe_render_vk.get_query_pool_results(
			    device->device, frame->timestamps,
			    VOE_RENDER_FRAME_TIMESTAMPS + 2 * i, 2,
			    sizeof(stamps), stamps, sizeof(stamps[0]),
			    VK_QUERY_RESULT_64_BIT) != VK_SUCCESS)
			return;
		memcpy(device->pass_times[i].name, frame->pass_names[i],
		       VOE_RENDER_PASS_NAME);
		device->pass_times[i].seconds =
			voe_render_timestamp_seconds(device, stamps[0], stamps[1]);
	}
	device->pass_time_count = frame->pass_timed;
}

uint32_t voe_render_frame_pass_times(const voe_render_device *device,
				     voe_render_pass_time *times,
				     uint32_t capacity)
{
	uint32_t count;

	VOE_BASE_ASSERT(device != NULL, "asking no device for its pass times");
	VOE_BASE_ASSERT(times != NULL || capacity == 0,
			"asking for pass times with nowhere to put them");

	count = device->pass_time_count < capacity ? device->pass_time_count :
						     capacity;
	if (count > 0)
		memcpy(times, device->pass_times, count * sizeof(*times));
	return count;
}
