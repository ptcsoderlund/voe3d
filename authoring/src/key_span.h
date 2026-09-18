// Each key's own trimmed bytes in a sectioned document — used by scene_read.c
// to write a kept section back out byte for byte.
//
// A LINE IS THE PARSER'S, A SPAN IS THIS FILE'S. voe_assets_sectioned_parse
// puts a line on every section and key, so nothing here counts lines for a
// caller; but it hands back values, not source bytes — a quoted value has lost
// its quotes — so a key's text as the file spelled it comes from a second walk
// of the same text. The walk only classifies a line — blank or `//` is
// nothing, `[` is a header, anything else is a key — and does not validate,
// because `doc` came from a text that already parsed: the k-th header or key
// this walk finds is the k-th `doc` holds, and an assert ties the two counts
// together.
#pragma once

#include <assets/sectioned.h>

#include <stddef.h>

// A key's own line, trimmed of blanks at either end, pointing into the same
// text voe_authoring_key_spans walked — not copied, and not NUL-terminated on
// its own.
typedef struct {
	const char *bytes;
	size_t size;
} voe_authoring_span;

// Fills `out_key_span[0..doc->key_count)` with each key's own trimmed bytes
// in `text`, in the order voe_assets_sectioned_parse numbers them. `doc` must
// be exactly what voe_assets_sectioned_parse returned for this `text` and
// `size`. The array is the caller's, sized to `doc->key_count`.
void voe_authoring_key_spans(const char *text, size_t size,
			     const voe_assets_sectioned *doc,
			     voe_authoring_span *out_key_span);
