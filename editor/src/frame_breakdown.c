// The Frame panel's quarter-second copy of render's timings, its one frame of
// `ui` calls and the read of its × afterwards. See the header for the lag.
#include "frame_breakdown.h"

#include "themes.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// The same plate, padding and gap errors.c's panel uses. Millimetres.
#define FRAME_PAD (3.0f * VOE_EDITOR_SPACING)
#define FRAME_GAP (2.0f * VOE_EDITOR_SPACING)

// Seconds between two copies: four a second, readable rather than flickering.
#define FRAME_REFRESH 0.25f

// Writes `seconds` as milliseconds to two places into a label's line.
static void milliseconds_write(char *line, double seconds)
{
	VOE_BASE_ASSERT(line != NULL, "writing milliseconds nowhere");

	// A number past the line is cut by snprintf, never overrun.
	(void)snprintf(line, VOE_EDITOR_FRAME_MILLISECONDS, "%.2f ms",
		       seconds * 1000.0);
}

void voe_editor_frame_breakdown_show(voe_editor_frame_breakdown *breakdown)
{
	VOE_BASE_ASSERT(breakdown != NULL, "showing no frame breakdown");

	breakdown->showing = true;
	// The first take after a show copies at once.
	breakdown->since_refresh = FRAME_REFRESH;
}

void voe_editor_frame_breakdown_hide(voe_editor_frame_breakdown *breakdown)
{
	VOE_BASE_ASSERT(breakdown != NULL, "hiding no frame breakdown");

	breakdown->showing = false;
}

void voe_editor_frame_breakdown_take(voe_editor_frame_breakdown *breakdown,
				     const voe_render_device *gpu, float seconds)
{
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	uint32_t count;
	double total;

	VOE_BASE_ASSERT(breakdown != NULL, "taking into no frame breakdown");
	VOE_BASE_ASSERT(gpu != NULL, "taking the timings of no device");
	VOE_BASE_ASSERT(seconds >= 0.0f, "a frame that took negative time");

	if (!breakdown->showing)
		return;
	breakdown->since_refresh += seconds;
	if (breakdown->since_refresh < FRAME_REFRESH)
		return;
	// Nothing measured keeps the last shown, and tries again next frame.
	if (!voe_render_frame_gpu_time(gpu, &total))
		return;
	count = voe_render_frame_pass_times(gpu, passes, VOE_EDITOR_FRAME_PASSES);
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
	VOE_BASE_ASSERT(breakdown->count <= VOE_EDITOR_FRAME_PASSES,
			"more passes kept than there is room for");
}

// One row: `name` at the left, `value` at the right.
static void row_draw(voe_ui_context *ui, const char *name, const char *value)
{
	voe_ui_row_begin(ui, (voe_ui_container){ .along = VOE_UI_ALONG_SPREAD,
						 .across = VOE_UI_ACROSS_CENTER,
						 .gap = FRAME_GAP });
	voe_ui_label(ui, name);
	voe_ui_label(ui, value);
	voe_ui_end(ui); // row
}

void voe_editor_frame_breakdown_draw(voe_ui_context *ui,
				     voe_editor_frame_breakdown *breakdown,
				     voe_math_float2 at)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no frame breakdown into no interface");
	VOE_BASE_ASSERT(breakdown != NULL, "drawing no frame breakdown");
	VOE_BASE_ASSERT(breakdown->showing,
			"drawing a frame breakdown that is not showing");

	voe_ui_panel_begin(
		ui, "frame breakdown", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = FRAME_GAP,
			.pad = { FRAME_PAD, FRAME_PAD, FRAME_PAD, FRAME_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_START, at.x },
				    .y = { VOE_UI_ACROSS_START, at.y } } });
	// The title at the left and its × at the right, as errors.c's.
	voe_ui_row_begin(ui, (voe_ui_container){ .along = VOE_UI_ALONG_SPREAD,
						 .across = VOE_UI_ACROSS_CENTER,
						 .gap = FRAME_GAP });
	voe_ui_label(ui, "Frame");
	breakdown->close_button = voe_ui_button_begin(ui, "frame_close", 0);
	voe_ui_label(ui, "×");
	voe_ui_end(ui); // × button
	voe_ui_end(ui); // title row

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

bool voe_editor_frame_breakdown_clicks_read(
	const voe_ui_context *ui, const voe_editor_frame_breakdown *breakdown)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(breakdown != NULL,
			"reading the clicks of no frame breakdown");

	// A refused frame hands back VOE_UI_NODE_NONE, skipped as errors.c's.
	return breakdown->close_button != VOE_UI_NODE_NONE &&
	       voe_ui_button_action(ui, breakdown->close_button).fired;
}
