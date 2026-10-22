// The Frame panel's quarter-second copy of render's timings and the pictures'
// counts, its one frame of `ui` calls and the read of its × afterwards. See the
// header for the lag.
#include "frame_breakdown.h"

#include "themes.h"

#include <base/assert.h>

#include <inttypes.h>
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

// Copies `views`' pictures and writes each one's name and two counts labels.
static void pictures_take(voe_editor_frame_breakdown *breakdown,
			  const voe_editor_views *views)
{
	VOE_BASE_ASSERT(views->picture_count <= VOE_EDITOR_VIEWS + 1,
			"more pictures drawn than there is room for");

	breakdown->picture_count = views->picture_count;
	memcpy(breakdown->pictures, views->pictures,
	       views->picture_count * sizeof views->pictures[0]);
	for (uint32_t i = 0; i < breakdown->picture_count; i++) {
		const voe_editor_picture_counts *picture =
			&breakdown->pictures[i];

		// A number past a line is cut by snprintf, never overrun.
		if (picture->preview)
			(void)snprintf(breakdown->picture_names[i],
				       VOE_EDITOR_FRAME_PICTURE, "Camera");
		else
			(void)snprintf(breakdown->picture_names[i],
				       VOE_EDITOR_FRAME_PICTURE, "View %" PRIu32,
				       picture->view + 1);
		(void)snprintf(breakdown->drawn_labels[i], VOE_EDITOR_FRAME_COUNTS,
			       "%" PRIu32 " drawn, %" PRIu32 " skipped",
			       picture->counts.drawn, picture->counts.skipped);
		(void)snprintf(breakdown->cast_labels[i], VOE_EDITOR_FRAME_COUNTS,
			       "%" PRIu32 " drawn, %" PRIu32 " skipped",
			       picture->counts.cast, picture->counts.cast_skipped);
	}
	VOE_BASE_ASSERT(breakdown->picture_count <= VOE_EDITOR_VIEWS + 1,
			"more pictures kept than there is room for");
}

void voe_editor_frame_breakdown_take(voe_editor_frame_breakdown *breakdown,
				     const voe_render_device *gpu,
				     const voe_editor_views *views,
				     float seconds)
{
	voe_render_pass_time passes[VOE_EDITOR_FRAME_PASSES];
	uint32_t count;
	double total;

	VOE_BASE_ASSERT(breakdown != NULL, "taking into no frame breakdown");
	VOE_BASE_ASSERT(gpu != NULL, "taking the timings of no device");
	VOE_BASE_ASSERT(views != NULL, "taking the counts of no views");
	VOE_BASE_ASSERT(seconds >= 0.0f, "a frame that took negative time");

	if (!breakdown->showing)
		return;
	breakdown->since_refresh += seconds;
	if (breakdown->since_refresh < FRAME_REFRESH)
		return;
	// The counts need no timestamps, so every refresh takes them; nothing
	// measured keeps the times last shown until the next refresh.
	breakdown->since_refresh = 0.0f;
	pictures_take(breakdown, views);
	if (!voe_render_frame_gpu_time(gpu, &total))
		return;
	count = voe_render_frame_pass_times(gpu, passes, VOE_EDITOR_FRAME_PASSES);
	if (count == 0)
		return;

	breakdown->measured = true;
	breakdown->count = count;
	memcpy(breakdown->passes, passes, count * sizeof passes[0]);
	breakdown->total = total;
	for (uint32_t i = 0; i < count; i++)
		milliseconds_write(breakdown->milliseconds[i],
				   breakdown->passes[i].seconds);
	milliseconds_write(breakdown->total_milliseconds, total);
	breakdown->triangles = voe_render_frame_triangle_count(gpu);
	(void)snprintf(breakdown->triangles_label, VOE_EDITOR_FRAME_TRIANGLES,
		       "%" PRIu64, breakdown->triangles);
	breakdown->draws = voe_render_frame_draw_count(gpu);
	(void)snprintf(breakdown->draws_label, VOE_EDITOR_FRAME_DRAWS,
		       "%" PRIu32, breakdown->draws);
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
		row_draw(ui, "Triangles", breakdown->triangles_label);
		row_draw(ui, "Draws", breakdown->draws_label);
	}
	for (uint32_t i = 0; i < breakdown->picture_count; i++) {
		row_draw(ui, breakdown->picture_names[i],
			 breakdown->drawn_labels[i]);
		row_draw(ui, "  shadows", breakdown->cast_labels[i]);
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
