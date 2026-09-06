// A TrueType file, read. Internal to this folder: it turns the bytes of a `.ttf`
// into the six things drawing a string needs — how big an em is, which glyph a
// character is, how far that glyph advances, how tall a line is, and the outline
// itself — and it reads nothing else.
//
// IT READS SEVEN TABLES AND NOT ONE MORE, WHICH IS RULE 10. `head` for the em
// and for how wide a `loca` entry is, `maxp` for how many glyphs there are,
// `loca` and `glyf` for the outlines, `cmap` for character to glyph, and `hhea`
// with `hmtx` for the advances and the line. Everything else in a font file —
// hinting programs, layout features, names, the digital signature — is stepped
// over without being looked at. A table this engine has no use for is not a
// table this file has an opinion about.
//
// FORMAT 4 IS THE ONLY CHARACTER MAP READ. Oxanium covers Adobe Latin 3, which
// is entirely inside the basic multilingual plane, and format 4 is the subtable
// every font carries for it. A file whose only map is a format this does not
// know is VOE_BASE_ERROR_UNSUPPORTED naming the format.
//
// AND THE FONT IS EMBEDDED, SO A FAILURE HERE IS A BUG IN THIS FILE. There is no
// file I/O anywhere near this: the bytes come out of the binary (text/src/font.c
// `#embed`s them) and they are the same bytes on every machine that ever runs
// the engine. The failures below are still returned rather than fatal, because
// rule 13 says a reader reports and the caller decides, but the honest reading
// of one is that this reader is wrong and not that the font is.
//
// EVERY LENGTH IN THE FILE IS CHECKED BEFORE IT IS TRUSTED. A table directory
// entry that runs past the end, a `loca` entry that points outside `glyf`, a
// point count that does not fit the space the glyph was given: each is
// VOE_BASE_ERROR_MALFORMED at the read that noticed. This is the same rule
// assets/src/png.c states and it is here for the same reason — nothing
// downstream may assume a number that came out of a file is sane.
//
// THERE IS NO RECURSION IN HERE, INCLUDING OVER COMPOSITE GLYPHS. A composite
// glyph is a list of references to other glyphs, and those may themselves be
// composite; the obvious reader is recursive descent and rule 14 forbids it. The
// walk below is an explicit stack with VOE_TEXT_COMPOSITE_DEPTH as its limit,
// and a glyph that nests deeper than that is refused rather than descended into.
//
// FONT UNITS COME OUT OF HERE AND NOTHING ELSE DOES. Every coordinate, advance
// and metric below is in the font's own units, which are `units_per_em` to the
// em and are neither pixels nor metres. The one place they become anything else
// is text/src/font.c — see the paragraph there about the scale being named once.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>

#include <stdint.h>

// How deep a composite glyph may reference other composite glyphs before this
// reader refuses it. The specification allows any depth and real fonts stay
// shallow — Oxanium's deepest is two — so a small limit costs nothing and is
// what keeps a corrupt or hostile file from walking a stack that is not there.
#define VOE_TEXT_COMPOSITE_DEPTH 8

// The most points and contours one glyph's outline may have, composites
// included. They are the size of the arrays voe_text_truetype_outline_read
// pushes,
// so they bound the work one glyph can cause; Oxanium's largest simple glyph is
// fifty points in four contours, and its largest composite is four of those.
#define VOE_TEXT_MAX_POINTS 1024
#define VOE_TEXT_MAX_CONTOURS 64

// A file, and the parts of it worth keeping. Offsets are byte offsets into
// `bytes`; every one of them has been checked to lie inside it.
typedef struct {
	const uint8_t *bytes;
	uint32_t size;

	uint32_t loca;
	uint32_t glyf;
	uint32_t glyf_size;
	uint32_t cmap;  // the format 4 subtable itself, not the table
	uint32_t cmap_size;
	uint32_t hmtx;
	uint32_t hmtx_size;

	// What an em is, in the units every coordinate below is in.
	uint16_t units_per_em;
	uint16_t glyph_count;
	// How many rows of `hmtx` carry an advance. Glyphs past it all share the
	// last one, which is how a font with a long run of equal-width glyphs is
	// stored.
	uint16_t hmetric_count;
	// head's indexToLocFormat: false for a `loca` of 16-bit halves of the
	// real offset, true for one of 32-bit offsets. Getting it the wrong way
	// round reads every outline from the wrong place.
	bool loca_long;

	// hhea's, in font units. The ascender is above the baseline and the
	// descender is below it and therefore negative.
	int16_t ascender;
	int16_t descender;
	int16_t line_gap;
} voe_text_truetype;

// One point of an outline. TrueType outlines are quadratic: a point that is not
// on the curve is a control point, and two control points in a row imply an
// on-curve point exactly between them.
typedef struct {
	voe_math_float2 point;
	bool on_curve;
} voe_text_truetype_point;

// One glyph's outline, in font units, with every composite already resolved into
// plain points. `contour_ends[i]` is the index of the last point of contour i,
// which is how `glyf` itself numbers them.
typedef struct {
	voe_text_truetype_point *points;
	uint16_t *contour_ends;
	uint16_t point_count;
	uint16_t contour_count;
} voe_text_truetype_outline;

// Reads the table directory and the five tables that describe the file. The
// bytes are not copied and must outlive `out`, which is what makes an embedded
// font free — see text/src/font.c.
//
// UNSUPPORTED for a file this reader knows it will not read: an sfnt version
// that is not TrueType outlines, a `cmap` with no format 4 subtable in it.
// MALFORMED for a file that does not hold together: a directory entry past the
// end, a table shorter than the fields it must carry. error may be NULL.
[[nodiscard]] bool voe_text_truetype_read(const uint8_t *bytes, uint32_t size,
					  voe_text_truetype *out,
					  voe_base_error *error);

// Which glyph draws this character, or 0 — `.notdef` — for one the font does not
// carry. A codepoint outside the basic multilingual plane is 0 as well, because
// format 4 cannot name one.
uint16_t voe_text_truetype_glyph(const voe_text_truetype *font,
				 uint32_t codepoint);

// How far the pen moves after this glyph, in font units. Zero for a glyph index
// past the end of the font, which is the same answer an empty glyph gives and is
// the harmless one.
uint16_t voe_text_truetype_advance(const voe_text_truetype *font,
				   uint16_t glyph);

// The outline, pushed into `arena` — one array of points and one of contour
// ends, each pushed once, because two pushes are not guaranteed to be adjacent
// (base/arena.h). A glyph with nothing to draw comes back with no contours and
// is not a failure: a space is that.
//
// MALFORMED for an outline that does not fit the space `loca` gave it or that
// runs past the end of `glyf`. UNSUPPORTED for a composite component this reader
// does not implement — a scaled one, or one positioned by matching points rather
// than by an offset. Neither happens in the embedded font; both are refused by
// name rather than silently mis-drawn.
[[nodiscard]] bool
voe_text_truetype_outline_read(const voe_text_truetype *font, uint16_t glyph,
			       voe_base_arena *arena,
			       voe_text_truetype_outline *out,
			       voe_base_error *error);
