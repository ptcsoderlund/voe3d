// The physical line number of every section header and every key a sectioned
// document holds — shared by scene_read.c and project.c, both of which name a
// line in a refusal.
//
// A SECOND WALK OF THE SAME TEXT, BECAUSE THE SECTIONED READER DOES NOT HAND
// LINE NUMBERS BACK WITH ITS PARSED DOCUMENT (assets/sectioned.h). It only
// classifies a line — blank or `//` is nothing, `[` is a header, anything
// else is a key — and does not validate, because `doc` came from a text that
// already parsed: the k-th header or key this walk finds is the k-th `doc`
// holds, and an assert ties the two counts together.
#pragma once

#include <assets/sectioned.h>

#include <stddef.h>
#include <stdint.h>

// A key's own line, trimmed of blanks at either end, pointing into the same
// text voe_authoring_line_index walked — not copied, and not NUL-terminated
// on its own.
typedef struct {
	const char *bytes;
	size_t size;
} voe_authoring_span;

// Fills `out_section_line[0..doc->section_count)` and
// `out_key_line[0..doc->key_count)` with each one's 1-based line in `text`,
// in the order voe_assets_sectioned_parse numbers them. `out_key_span`, not
// NULL, is filled the same way with each key's own trimmed bytes, for a
// caller that must hand a kept section's lines back out byte for byte; a
// caller with no use for it passes NULL. `doc` must be exactly what
// voe_assets_sectioned_parse returned for this `text` and `size`. Every
// output array is the caller's, sized to `doc->section_count` and
// `doc->key_count`.
void voe_authoring_line_index(const char *text, size_t size,
			      const voe_assets_sectioned *doc,
			      uint32_t *out_section_line, uint32_t *out_key_line,
			      voe_authoring_span *out_key_span);
