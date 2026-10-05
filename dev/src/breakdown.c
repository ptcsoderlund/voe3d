// The frame breakdown's quarter-second copy of render's timings and its one
// frame of `ui` calls. See the header for the lag and the cost.
#include "breakdown.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// The plate's padding and the gap between its rows, in millimetres.
#define BREAKDOWN_PAD 3.0f
#define BREAKDOWN_GAP 2.0f

// Seconds between two copies: four a second, readable rather than flickering.
#define BREAKDOWN_REFRESH 0.25f

// Writes `seconds` as milliseconds to two places into a label's line.
static void milliseconds_write(char *line, double seconds)
{
	VOE_BASE_ASSERT(line != NULL, "writing milliseconds nowhere");

	// A number past the line is cut by snprintf, never overrun.
	(void)snprintf(line, VOE_DEV_BREAKDOWN_MILLISECONDS, "%.2f ms",
		       seconds * 1000.0);
}

void voe_dev_breakdown_toggle(voe_dev_breakdown *breakdown)
{
	VOE_BASE_ASSERT(breakdown != NULL, "toggling no frame breakdown");

	breakdown->shown = !breakdown->shown;
	// The first take after a show copies at once.
	breakdown->since_refresh = BREAKDOWN_REFRESH;
}

void voe_dev_breakdown_take(voe_dev_breakdown *breakdown,
			    const voe_render_device *gpu, float seconds)
{
	voe_render_pass_time passes[VOE_DEV_BREAKDOWN_PASSES];
	uint32_t count;
	double total;

	VOE_BASE_ASSERT(breakdown != NULL, "taking into no frame breakdown");
	VOE_BASE_ASSERT(gpu != NULL, "taking the timings of no device");
	VOE_BASE_ASSERT(seconds >= 0.0f, "a frame that took negative time");

	if (!breakdown->shown)
		return;
	breakdown->since_refresh += seconds;
	if (breakdown->since_refresh < BREAKDOWN_REFRESH)
		return;
	// Nothing measured keeps the last shown, and tries again next frame.
	if (!voe_render_frame_gpu_time(gpu, &total))
		return;
	count = voe_render_frame_pass_times(gpu, passes,
					    VOE_DEV_BREAKDOWN_PASSES);
	if (count == 0)
		return;

	breakdown->since_refresh = 0.0f;
	breakdown->measured = true;
	breakdown->count = count;
	memcpy(breakdown->passes, passes, count * sizeof passes[0]);
	breakdown->total = total;
	for (uint32_t i = 0; i < count; i++)
		milliseconds_write(breakdown->milliseconds[i],
				   breakdown->passes[i].seconds);
	milliseconds_write(breakdown->total_milliseconds, total);
	VOE_BASE_ASSERT(breakdown->count <= VOE_DEV_BREAKDOWN_PASSES,
			"more passes kept than there is room for");
}

// One row: `name` at the left, `value` at the right.
static void row_draw(voe_ui_context *ui, const char *name, const char *value)
{
	VOE_BASE_ASSERT(name != NULL && value != NULL, "a row with no text");

	voe_ui_row_begin(ui, (voe_ui_container){ .along = VOE_UI_ALONG_SPREAD,
						 .across = VOE_UI_ACROSS_CENTER,
						 .gap = BREAKDOWN_GAP });
	voe_ui_label(ui, name);
	voe_ui_label(ui, value);
	voe_ui_end(ui); // row
}

void voe_dev_breakdown_draw(voe_ui_context *ui,
			    const voe_dev_breakdown *breakdown)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a frame breakdown into no interface");
	VOE_BASE_ASSERT(breakdown != NULL, "drawing no frame breakdown");
	VOE_BASE_ASSERT(breakdown->shown,
			"drawing a frame breakdown that is not shown");

	voe_ui_panel_begin(
		ui, "frame breakdown", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = BREAKDOWN_GAP,
			.pad = { BREAKDOWN_PAD, BREAKDOWN_PAD, BREAKDOWN_PAD,
				 BREAKDOWN_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_END, 0.0f },
				    .y = { VOE_UI_ACROSS_START, 0.0f } } });
	voe_ui_label(ui, "Frame");

	if (!breakdown->measured) {
		voe_ui_label(ui, "No GPU timings");
	} else {
		for (uint32_t i = 0; i < breakdown->count; i++)
			row_draw(ui, breakdown->passes[i].name,
				 breakdown->milliseconds[i]);
		row_draw(ui, "Total", breakdown->total_milliseconds);
	}

	voe_ui_end(ui); // panel
}
