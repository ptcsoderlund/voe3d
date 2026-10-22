// The hitch watch frame_hitches.h describes: stretches read off the platform
// clock, then app/hitches.h asked about the frame and the card's newest
// measurement, each hitch's line put on stderr.
#include "frame_hitches.h"

#include "frame_breakdown.h"

#include <base/assert.h>

#include <platform/clock.h>

#include <stdio.h>

// The prefix every hitch line is written after (0458 point 3).
#define HITCH_PREFIX "editor: hitch: "

void voe_editor_frame_hitches_begin(voe_editor_frame_hitches *watch)
{
	VOE_BASE_ASSERT(watch != NULL, "beginning no hitch watch");

	watch->count = 0;
	watch->since = voe_platform_clock_now();
	VOE_BASE_ASSERT(watch->since >= 0.0, "a clock reading below zero");
}

// A stretch past the room is the caller's bug: main.c marks a fixed few.
static void stretch_add(voe_editor_frame_hitches *watch, const char *name,
			double seconds)
{
	VOE_BASE_ASSERT(watch != NULL && name != NULL,
			"adding a stretch to no watch or with no name");
	VOE_BASE_ASSERT(watch->count < VOE_EDITOR_FRAME_HITCHES_STRETCHES,
			"more stretches in a frame than there is room for");

	watch->stretches[watch->count] =
		(voe_app_stretch){ .name = name, .seconds = seconds };
	watch->count++;
}

void voe_editor_frame_hitches_mark(voe_editor_frame_hitches *watch,
				   const char *name)
{
	double now = voe_platform_clock_now();

	VOE_BASE_ASSERT(watch != NULL && name != NULL,
			"marking no hitch watch or with no name");

	stretch_add(watch, name, now - watch->since);
	watch->since = now;
	VOE_BASE_ASSERT(watch->count <= VOE_EDITOR_FRAME_HITCHES_STRETCHES,
			"a mark past the stretches' room");
}

void voe_editor_frame_hitches_end(voe_editor_frame_hitches *watch,
				  const voe_render_device *gpu, double elapsed,
				  double shadows)
{
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	char line[VOE_APP_HITCH_LINE];
	double total;
	uint32_t count;

	VOE_BASE_ASSERT(watch != NULL && gpu != NULL,
			"ending no hitch watch or with no device");
	VOE_BASE_ASSERT(elapsed >= 0.0 && shadows >= 0.0,
			"a frame or its shadows that took negative time");

	stretch_add(watch, "shadows", shadows);
	if (voe_app_hitches_frame(&watch->hitches, elapsed, watch->stretches,
				  watch->count, line, sizeof line))
		fprintf(stderr, HITCH_PREFIX "%s\n", line);

	// No measured frame yet, or a card without timestamps: no card line.
	if (!voe_render_frame_gpu_time(gpu, &total))
		return;
	count = voe_render_frame_pass_times(gpu, passes, VOE_EDITOR_FRAME_PASSES);
	if (voe_app_hitches_card(&watch->hitches, total, passes, count, line,
				 sizeof line))
		fprintf(stderr, HITCH_PREFIX "%s\n", line);
}
