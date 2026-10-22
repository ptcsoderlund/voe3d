// The hitch watch frame_hitches.h describes: stretches read off the platform
// clock, the shadows' sums added as stretches, then app/hitches.h asked about
// the frame and the card's newest measurement, each hitch's line put on stderr,
// the card's with the newest frame's triangles and draws.
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
	watch->shadows = 0.0;
	watch->gather = 0.0;
	watch->parts = (voe_3d_shadow_times){ 0 };
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

// A part of the shadows that took no time says nothing, so is left out.
static void part_add(voe_editor_frame_hitches *watch, const char *name,
		     double seconds)
{
	VOE_BASE_ASSERT(watch != NULL && name != NULL,
			"adding a part to no watch or with no name");
	VOE_BASE_ASSERT(seconds >= 0.0, "a part that took negative time");

	if (seconds > 0.0)
		stretch_add(watch, name, seconds);
}

void voe_editor_frame_hitches_end(voe_editor_frame_hitches *watch,
				  const voe_render_device *gpu, double elapsed)
{
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	char line[VOE_APP_HITCH_LINE];
	double total;
	uint32_t count;

	VOE_BASE_ASSERT(watch != NULL && gpu != NULL,
			"ending no hitch watch or with no device");
	VOE_BASE_ASSERT(elapsed >= 0.0 && watch->shadows >= 0.0 &&
				watch->gather >= 0.0,
			"a frame, its shadows or gathers that took negative time");

	stretch_add(watch, "shadows", watch->shadows);
	part_add(watch, "shadows: casters", watch->parts.casters);
	part_add(watch, "shadows: ground", watch->parts.ground);
	part_add(watch, "shadows: foliage", watch->parts.foliage);
	part_add(watch, "shadows: lamps", watch->parts.lamps);
	part_add(watch, "shadows: bounce", watch->parts.bounce);
	stretch_add(watch, "gather", watch->gather);
	if (voe_app_hitches_frame(&watch->hitches, elapsed, watch->stretches,
				  watch->count, line, sizeof line))
		fprintf(stderr, HITCH_PREFIX "%s\n", line);

	// No measured frame yet, or a card without timestamps: no card line.
	if (!voe_render_frame_gpu_time(gpu, &total))
		return;
	count = voe_render_frame_pass_times(gpu, passes, VOE_EDITOR_FRAME_PASSES);
	// The counts are the newest recorded frame's, not the measured one's.
	if (voe_app_hitches_card(&watch->hitches, total, passes, count, line,
				 sizeof line))
		fprintf(stderr, HITCH_PREFIX "%s, %llu triangles, %u draws\n",
			line,
			(unsigned long long)voe_render_frame_triangle_count(gpu),
			(unsigned)voe_render_frame_draw_count(gpu));
}
