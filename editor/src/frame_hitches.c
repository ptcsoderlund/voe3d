// The hitch watch frame_hitches.h describes: stretches read off the platform
// clock, the pace's wait and the shadows' sums added as stretches, then
// app/hitches.h asked about the frame since _begin and the card's newest
// measurement, each hitch's line put on stderr, the frame's saying whether a
// light bounced, the card's with the newest frame's triangles and draws.
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
	watch->began = voe_platform_clock_now();
	watch->since = watch->began;
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
				  const voe_render_device *gpu, double waited)
{
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	char line[VOE_APP_HITCH_LINE];
	double elapsed;
	double total;
	uint32_t count;

	VOE_BASE_ASSERT(watch != NULL && gpu != NULL,
			"ending no hitch watch or with no device");
	elapsed = voe_platform_clock_now() - watch->began;
	VOE_BASE_ASSERT(elapsed >= 0.0 && waited >= 0.0 &&
				watch->shadows >= 0.0 && watch->gather >= 0.0,
			"a frame, its wait, shadows or gathers that took negative time");

	part_add(watch, "pace: wait", waited);
	stretch_add(watch, "shadows", watch->shadows);
	part_add(watch, "shadows: casters", watch->parts.casters);
	part_add(watch, "shadows: ground", watch->parts.ground);
	part_add(watch, "shadows: foliage", watch->parts.foliage);
	part_add(watch, "shadows: lamps", watch->parts.lamps);
	part_add(watch, "shadows: bounce", watch->parts.bounce);
	part_add(watch, "bounce: fit", watch->parts.bounce_fit);
	part_add(watch, "bounce: stale", watch->parts.bounce_stale);
	part_add(watch, "bounce: begin", watch->parts.bounce_begin);
	part_add(watch, "bounce: captures", watch->parts.bounce_captures);
	part_add(watch, "bounce: suns", watch->parts.bounce_suns);
	part_add(watch, "bounce: relight", watch->parts.bounce_relight);
	stretch_add(watch, "gather", watch->gather);
	// Whether a light bounced ends the line, as the counts end the card's.
	if (voe_app_hitches_frame(&watch->hitches, elapsed, watch->stretches,
				  watch->count, line, sizeof line))
		fprintf(stderr, HITCH_PREFIX "%s, %s\n", line,
			watch->parts.bounced > 0 ? "light bounces"
						 : "no light bounces");

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
