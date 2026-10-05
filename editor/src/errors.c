// The Errors panel's log read, its one frame of `ui` calls and the read of
// its × afterwards. See the header for why only the log's tail is kept.
#include "errors.h"

#include "themes.h"

#include <base/arena.h>
#include <base/assert.h>

#include <platform/file.h>

#include <stdio.h>

// The same plate, padding and gap preferences.c's panel uses. Millimetres.
#define ERRORS_PAD (3.0f * VOE_EDITOR_SPACING)
#define ERRORS_GAP (2.0f * VOE_EDITOR_SPACING)

// A block size for the log's bytes, not a limit (base/arena.h).
#define ERRORS_SCRATCH (64u * 1024u)

// Copies bytes[0..length) into line, cut at VOE_EDITOR_ERRORS_LINE_BYTES and
// back to the start of a UTF-8 character, a CR dropped and a tab a space.
static void line_keep(char *line, const uint8_t *bytes, size_t length)
{
	size_t kept = 0;

	VOE_BASE_ASSERT(line != NULL, "keeping a line nowhere");
	VOE_BASE_ASSERT(bytes != NULL || length == 0, "keeping no bytes");
	for (size_t i = 0; i < length && kept < VOE_EDITOR_ERRORS_LINE_BYTES;
	     i++) {
		if (bytes[i] == '\r')
			continue;
		line[kept++] = bytes[i] == '\t' ? ' ' : (char)bytes[i];
	}
	// A character cut in two draws as nothing worth reading: its lead
	// byte and whatever followed it go.
	if (kept == VOE_EDITOR_ERRORS_LINE_BYTES) {
		size_t end = kept;

		while (end > 0 && ((uint8_t)line[end - 1] & 0xC0u) == 0x80u)
			end--;
		if (end > 0 && ((uint8_t)line[end - 1] & 0x80u) != 0)
			kept = end - 1;
	}
	line[kept] = '\0';
	VOE_BASE_ASSERT(kept <= VOE_EDITOR_ERRORS_LINE_BYTES, "a line past its cut");
}

void voe_editor_errors_show(voe_editor_errors *errors, const char *log)
{
	voe_base_arena *scratch;
	const uint8_t *bytes;
	size_t size;
	size_t start;
	uint32_t found = 0;

	VOE_BASE_ASSERT(errors != NULL, "showing no errors");
	VOE_BASE_ASSERT(log != NULL, "showing the errors of no log");

	errors->showing = true;
	errors->count = 0;
	scratch = voe_base_arena_new(ERRORS_SCRATCH);
	VOE_BASE_ASSERT(scratch != NULL, "no scratch to read the log into");
	bytes = voe_platform_file_read(log, scratch, &size, NULL);
	if (bytes == NULL) {
		char why[VOE_EDITOR_ERRORS_LINE_BYTES + 1];
		int length = snprintf(why, sizeof why, "Could not read %s", log);

		line_keep(errors->lines[0], (const uint8_t *)why,
			  length < 0 ? 0 : (size_t)length);
		errors->count = 1;
		goto destroy;
	}

	// Back from the end, a trailing newline not starting a line of its own,
	// to just after the newline before the last VOE_EDITOR_ERRORS_LINES.
	if (size > 0 && bytes[size - 1] == '\n')
		size--;
	start = size;
	while (start > 0) {
		if (bytes[start - 1] == '\n' &&
		    ++found == VOE_EDITOR_ERRORS_LINES)
			break;
		start--;
	}

	for (size_t at = start; at <= size &&
				errors->count < VOE_EDITOR_ERRORS_LINES;) {
		size_t end = at;

		while (end < size && bytes[end] != '\n')
			end++;
		line_keep(errors->lines[errors->count++], bytes + at, end - at);
		at = end + 1;
	}
destroy:
	voe_base_arena_destroy(scratch);
	VOE_BASE_ASSERT(errors->count > 0 &&
				errors->count <= VOE_EDITOR_ERRORS_LINES,
			"errors kept no line or too many");
}

void voe_editor_errors_hide(voe_editor_errors *errors)
{
	VOE_BASE_ASSERT(errors != NULL, "hiding no errors");

	errors->showing = false;
}

void voe_editor_errors_draw(voe_ui_context *ui, voe_editor_errors *errors,
			    float top, voe_math_float2 size)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no errors into no interface");
	VOE_BASE_ASSERT(errors != NULL, "drawing no errors");
	VOE_BASE_ASSERT(errors->showing, "drawing errors that are not showing");

	// Anchored exactly as preferences.c's panel is.
	voe_ui_panel_begin(
		ui, "errors", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = ERRORS_GAP,
			.pad = { ERRORS_PAD, ERRORS_PAD, ERRORS_PAD,
				 ERRORS_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_FILL, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top } },
			.size = { .across = { VOE_UI_SIZE_FIXED, size.y } } });
	// The title at the left and its × at the right (0363 point 2), spread
	// apart along a row the panel stretches across, as a dock header is.
	voe_ui_row_begin(ui, (voe_ui_container){ .along = VOE_UI_ALONG_SPREAD,
						 .across = VOE_UI_ACROSS_CENTER });
	voe_ui_label(ui, "Build errors");
	errors->close_button = voe_ui_button_begin(ui, "errors_close", 0);
	voe_ui_label(ui, "×");
	voe_ui_end(ui); // × button
	voe_ui_end(ui); // title row

	// Both axes: a compiler's line is often wider than the panel.
	voe_ui_scroll_begin(
		ui, "error_lines", 0,
		(voe_ui_container){ .size = { .along = { VOE_UI_SIZE_GROW,
							 1.0f } },
				    .across = VOE_UI_ACROSS_START },
		(voe_ui_scroll_axes){ .x = true, .y = true });
	for (uint32_t i = 0; i < errors->count; i++)
		voe_ui_label(ui, errors->lines[i]);
	voe_ui_end(ui); // scroll area

	voe_ui_end(ui); // panel
}

bool voe_editor_errors_clicks_read(const voe_ui_context *ui,
				   const voe_editor_errors *errors)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(errors != NULL, "reading the clicks of no errors");

	// A refused frame hands back VOE_UI_NODE_NONE, skipped as
	// preferences.c's are.
	return errors->close_button != VOE_UI_NODE_NONE &&
	       voe_ui_button_action(ui, errors->close_button).fired;
}
