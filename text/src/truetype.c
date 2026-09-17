// The reader. Flat, bounds-checked, and with an explicit stack where the obvious
// implementation would recurse — see truetype.h for what it reads and why it
// reads no more than that.
//
// EVERY READ GOES THROUGH struct reader AND THAT IS THE WHOLE OF THE BOUNDS
// CHECKING. A reader carries the buffer, a cursor and one `ok` flag; a read past
// the end clears the flag and hands back zero rather than touching memory, and
// the flag is tested once at the end of each stretch of reading instead of after
// every field. That is what keeps this file readable: the alternative is a
// branch after each of the forty-odd numbers below, and a reader that is hard to
// read is a reader nobody checks.
//
// SO A ZERO MAY MEAN "PAST THE END", AND NOTHING HERE ACTS ON A NUMBER BEFORE
// THE FLAG IS TESTED. A count read out of a truncated file comes back as zero
// and a loop over it does nothing, which is safe; a value used to size a push
// would not be, so every push below happens after a check.
#include "truetype.h"

#include <base/assert.h>

#include <stdint.h>
#include <string.h>

// The sfnt version of a file holding TrueType outlines. The other one anybody
// meets is 'OTTO', which holds compact-font-format outlines instead — cubic, a
// different table entirely, and a different reader.
#define SFNT_TRUETYPE 0x00010000u

// glyf's simple-glyph flags.
#define GLYF_ON_CURVE 0x01
#define GLYF_X_SHORT 0x02
#define GLYF_Y_SHORT 0x04
#define GLYF_REPEAT 0x08
#define GLYF_X_SAME 0x10
#define GLYF_Y_SAME 0x20

// glyf's composite-component flags. This reader refuses the absence of the one
// that says the arguments are an offset, and the two transforms it does not
// implement — independent x/y scale and a full 2x2 matrix; see the header. A
// uniform COMPONENT_HAVE_SCALE is read, not refused — Pixel Operator needs it.
#define COMPONENT_ARGS_ARE_WORDS 0x0001
#define COMPONENT_ARGS_ARE_XY 0x0002
#define COMPONENT_HAVE_SCALE 0x0008
#define COMPONENT_MORE 0x0020
#define COMPONENT_HAVE_XY_SCALE 0x0040
#define COMPONENT_HAVE_TWO_BY_TWO 0x0080

// A component's uniform scale is F2Dot14: a signed 16-bit fixed-point number
// with 14 fractional bits, so 16384 is 1.0 and the sign flips a component,
// which is exactly what turns `!` into `¡` in Pixel Operator.
#define COMPONENT_SCALE_ONE 16384.0f

struct reader {
	const uint8_t *bytes;
	uint32_t size;
	uint32_t at;
	bool ok;
};

static struct reader reader_at(const uint8_t *bytes, uint32_t size, uint32_t at)
{
	struct reader r = { .bytes = bytes, .size = size, .at = at,
			    .ok = at <= size };

	return r;
}

static bool reader_take(struct reader *r, uint32_t count)
{
	if (!r->ok || count > r->size - r->at) {
		r->ok = false;
		return false;
	}
	r->at += count;
	return true;
}

static uint8_t read_u8(struct reader *r)
{
	uint32_t at = r->at;

	if (!reader_take(r, 1))
		return 0;
	return r->bytes[at];
}

static uint16_t read_u16(struct reader *r)
{
	uint32_t at = r->at;

	if (!reader_take(r, 2))
		return 0;
	return (uint16_t)((uint16_t)r->bytes[at] << 8 | r->bytes[at + 1]);
}

static int16_t read_i16(struct reader *r)
{
	return (int16_t)read_u16(r);
}

static uint32_t read_u32(struct reader *r)
{
	uint32_t at = r->at;

	if (!reader_take(r, 4))
		return 0;
	return (uint32_t)r->bytes[at] << 24 | (uint32_t)r->bytes[at + 1] << 16 |
	       (uint32_t)r->bytes[at + 2] << 8 | (uint32_t)r->bytes[at + 3];
}

static bool fail(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
	return false;
}

// Finds one table in the directory. Comes back false when the file has no such
// table or when the entry names a range that is not inside the file.
static bool find_table(const uint8_t *bytes, uint32_t size, const char *tag,
		       uint32_t *offset, uint32_t *length)
{
	struct reader r = reader_at(bytes, size, 4);
	uint16_t count = read_u16(&r);

	if (!r.ok)
		return false;

	for (uint16_t i = 0; i < count; i++) {
		struct reader entry = reader_at(bytes, size, 12 + 16u * i);
		uint32_t at;
		uint32_t len;
		char name[4];

		for (int c = 0; c < 4; c++)
			name[c] = (char)read_u8(&entry);
		(void)read_u32(&entry);
		at = read_u32(&entry);
		len = read_u32(&entry);

		if (!entry.ok)
			return false;
		if (memcmp(name, tag, 4) != 0)
			continue;
		// The whole point of this function: a directory entry is a
		// number out of a file and may name anything at all.
		if (at > size || len > size - at)
			return false;
		*offset = at;
		*length = len;
		return true;
	}
	return false;
}

// The format 4 subtable inside `cmap`, or false when the file has none. The two
// encodings looked for are the two every font carries: Unicode, and Windows
// Unicode BMP, which are the same table under two names.
static bool find_cmap_format4(const uint8_t *bytes, uint32_t size,
			      uint32_t cmap, uint32_t cmap_size,
			      uint32_t *offset, uint32_t *length)
{
	struct reader r = reader_at(bytes, size, cmap + 2);
	uint16_t count = read_u16(&r);

	if (!r.ok)
		return false;

	for (uint16_t i = 0; i < count; i++) {
		struct reader record = reader_at(bytes, size, cmap + 4 + 8u * i);
		uint16_t platform = read_u16(&record);
		uint16_t encoding = read_u16(&record);
		uint32_t at = read_u32(&record);
		struct reader sub;
		uint16_t format;
		uint16_t subtable_length;

		if (!record.ok)
			return false;
		if (!(platform == 0 || (platform == 3 && encoding == 1)))
			continue;
		if (at > cmap_size)
			return false;

		sub = reader_at(bytes, size, cmap + at);
		format = read_u16(&sub);
		subtable_length = read_u16(&sub);
		if (!sub.ok || format != 4)
			continue;
		if (subtable_length < 16 ||
		    (uint32_t)subtable_length > size - (cmap + at))
			return false;
		*offset = cmap + at;
		*length = subtable_length;
		return true;
	}
	return false;
}

bool voe_text_truetype_read(const uint8_t *bytes, uint32_t size,
			    voe_text_truetype *out, voe_base_error *error)
{
	struct reader r;
	uint32_t head;
	uint32_t maxp;
	uint32_t hhea;
	uint32_t cmap;
	uint32_t length;
	uint32_t loca_length;
	uint32_t loca_needed;

	VOE_BASE_ASSERT(bytes != NULL,
			"a font with no bytes: the embedded file is missing");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the font that was read");

	*out = (voe_text_truetype){ .bytes = bytes, .size = size };

	r = reader_at(bytes, size, 0);
	if (read_u32(&r) != SFNT_TRUETYPE || !r.ok)
		return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	if (!find_table(bytes, size, "head", &head, &length) || length < 54)
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	r = reader_at(bytes, size, head + 18);
	out->units_per_em = read_u16(&r);
	r = reader_at(bytes, size, head + 50);
	out->loca_long = read_i16(&r) != 0;
	if (!r.ok || out->units_per_em == 0)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	if (!find_table(bytes, size, "maxp", &maxp, &length) || length < 6)
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	r = reader_at(bytes, size, maxp + 4);
	out->glyph_count = read_u16(&r);
	if (!r.ok || out->glyph_count == 0)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	if (!find_table(bytes, size, "hhea", &hhea, &length) || length < 36)
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	r = reader_at(bytes, size, hhea + 4);
	out->ascender = read_i16(&r);
	out->descender = read_i16(&r);
	out->line_gap = read_i16(&r);
	r = reader_at(bytes, size, hhea + 34);
	out->hmetric_count = read_u16(&r);
	if (!r.ok || out->hmetric_count == 0 ||
	    out->hmetric_count > out->glyph_count)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	if (!find_table(bytes, size, "hmtx", &out->hmtx, &out->hmtx_size))
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	if (out->hmtx_size < 4u * out->hmetric_count)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	if (!find_table(bytes, size, "loca", &out->loca, &loca_length))
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	// One entry per glyph plus one past the end, which is what makes a
	// glyph's length the difference between two of them.
	loca_needed = (uint32_t)(out->glyph_count + 1u) *
		      (out->loca_long ? 4u : 2u);
	if (loca_length < loca_needed)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	if (!find_table(bytes, size, "glyf", &out->glyf, &out->glyf_size))
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	if (!find_table(bytes, size, "cmap", &cmap, &length))
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	if (!find_cmap_format4(bytes, size, cmap, length, &out->cmap,
			       &out->cmap_size))
		return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	return true;
}

uint16_t voe_text_truetype_glyph(const voe_text_truetype *font,
				 uint32_t codepoint)
{
	struct reader r;
	uint32_t ends;
	uint32_t starts;
	uint32_t deltas;
	uint32_t ranges;
	uint16_t segments;
	uint16_t character;

	VOE_BASE_ASSERT(font != NULL, "no font to look a character up in");

	// Format 4 maps the basic multilingual plane and nothing above it, and
	// 0xffff is the end marker every segment array finishes with rather than
	// a character.
	if (codepoint >= 0xffff)
		return 0;
	character = (uint16_t)codepoint;

	r = reader_at(font->bytes, font->size, font->cmap + 6);
	segments = (uint16_t)(read_u16(&r) / 2);
	if (!r.ok || segments == 0)
		return 0;

	// The four parallel arrays, in the order the format lays them out. The
	// two extra bytes are the reserved pad that sits between the end codes
	// and the start codes and exists for no reason anyone remembers.
	ends = font->cmap + 14;
	starts = ends + 2u * segments + 2u;
	deltas = starts + 2u * segments;
	ranges = deltas + 2u * segments;
	if (ranges + 2u * segments > font->cmap + font->cmap_size)
		return 0;

	for (uint16_t i = 0; i < segments; i++) {
		struct reader entry = reader_at(font->bytes, font->size,
						ends + 2u * i);
		uint16_t end = read_u16(&entry);
		uint16_t start;
		int16_t delta;
		uint16_t range;
		uint16_t found;

		if (!entry.ok || character > end)
			continue;
		entry = reader_at(font->bytes, font->size, starts + 2u * i);
		start = read_u16(&entry);
		if (!entry.ok || character < start)
			return 0;

		entry = reader_at(font->bytes, font->size, deltas + 2u * i);
		delta = read_i16(&entry);
		entry = reader_at(font->bytes, font->size, ranges + 2u * i);
		range = read_u16(&entry);
		if (!entry.ok)
			return 0;

		if (range == 0)
			return (uint16_t)(character + delta);

		// The one genuinely strange thing in the format: the range
		// offset is a byte offset from where it is itself stored, not
		// from the start of anything.
		entry = reader_at(font->bytes, font->size,
				  ranges + 2u * i + range +
					  2u * (uint32_t)(character - start));
		found = read_u16(&entry);
		if (!entry.ok || found == 0)
			return 0;
		return (uint16_t)(found + delta);
	}
	return 0;
}

uint16_t voe_text_truetype_advance(const voe_text_truetype *font,
				   uint16_t glyph)
{
	struct reader r;
	uint16_t row;

	VOE_BASE_ASSERT(font != NULL, "no font to read an advance out of");

	if (glyph >= font->glyph_count)
		return 0;
	// Glyphs past the last long metric all advance by the last one. That is
	// how a font with a run of equal-width glyphs at the end is stored, and
	// it is why hmtx is not simply an array.
	row = glyph < font->hmetric_count ? glyph
					  : (uint16_t)(font->hmetric_count - 1);
	r = reader_at(font->bytes, font->size, font->hmtx + 4u * row);
	return read_u16(&r);
}

// Where one glyph's outline is inside `glyf`, from `loca`. Comes back false when
// the entry does not describe a range inside the table — including the backwards
// one, which is a real corruption rather than an empty glyph.
static bool glyph_range(const voe_text_truetype *font, uint16_t glyph,
			uint32_t *at, uint32_t *length)
{
	struct reader r;
	uint32_t start;
	uint32_t end;

	if (glyph >= font->glyph_count)
		return false;

	if (font->loca_long) {
		r = reader_at(font->bytes, font->size, font->loca + 4u * glyph);
		start = read_u32(&r);
		end = read_u32(&r);
	} else {
		// The short form stores half the offset, which is what limits a
		// short-`loca` font's outlines to 128 kilobytes.
		r = reader_at(font->bytes, font->size, font->loca + 2u * glyph);
		start = (uint32_t)read_u16(&r) * 2u;
		end = (uint32_t)read_u16(&r) * 2u;
	}
	if (!r.ok || end < start || end > font->glyf_size)
		return false;

	*at = font->glyf + start;
	*length = end - start;
	return true;
}

// One simple glyph's points, appended to what the outline already holds.
// `scale` multiplies a point before `shift` moves it — one is 1.0 and the
// other is zero for a glyph that is not part of a composite, and scale is
// applied first because that is the order a scaled composite's own flags
// describe: scaled about its own origin, then offset into the parent.
static bool append_simple(const voe_text_truetype *font, uint32_t at,
			  uint32_t length, float scale, voe_math_float2 shift,
			  voe_text_truetype_outline *out,
			  voe_base_error *error)
{
	struct reader r = reader_at(font->bytes, font->size, at);
	uint32_t limit = at + length;
	int16_t contours;
	uint16_t first = out->point_count;
	uint16_t points;
	uint16_t instructions;
	int32_t x = 0;
	int32_t y = 0;

	contours = read_i16(&r);
	if (!r.ok || contours <= 0)
		return fail(error, VOE_BASE_ERROR_MALFORMED);
	if ((uint16_t)contours > VOE_TEXT_MAX_CONTOURS - out->contour_count)
		return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	// Past the four bounding-box values, which this reader has no use for:
	// the atlas measures a glyph from the outline it actually rasterised.
	r = reader_at(font->bytes, font->size, at + 10);

	for (int16_t c = 0; c < contours; c++) {
		uint16_t end = read_u16(&r);

		if (!r.ok)
			return fail(error, VOE_BASE_ERROR_MALFORMED);
		// The ends are indices into this glyph's own points and they
		// ascend; the last one is one less than the point count.
		if (c > 0 && end <= out->contour_ends[out->contour_count - 1] -
					    first)
			return fail(error, VOE_BASE_ERROR_MALFORMED);
		out->contour_ends[out->contour_count++] =
			(uint16_t)(first + end);
	}
	points = (uint16_t)(out->contour_ends[out->contour_count - 1] - first +
			    1);
	if (points > VOE_TEXT_MAX_POINTS - out->point_count)
		return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	// The hinting program. Read for its length and stepped over: hinting is
	// a bytecode interpreter and this engine does not have one.
	instructions = read_u16(&r);
	if (!reader_take(&r, instructions))
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	// The flags, which are run-length encoded, and the two delta-encoded
	// coordinate arrays after them. Three passes over the same points
	// because that is the order the file stores them in.
	for (uint16_t i = 0; i < points;) {
		uint8_t flags = read_u8(&r);
		uint8_t repeat = 0;

		if (!r.ok)
			return fail(error, VOE_BASE_ERROR_MALFORMED);
		if (flags & GLYF_REPEAT)
			repeat = read_u8(&r);
		if (!r.ok || (uint32_t)i + repeat + 1u > points)
			return fail(error, VOE_BASE_ERROR_MALFORMED);

		for (uint8_t n = 0; n <= repeat; n++) {
			out->points[first + i].on_curve =
				(flags & GLYF_ON_CURVE) != 0;
			// The flags are needed again by each of the two
			// coordinate loops below, and re-reading them there
			// would mean decoding the run-length encoding three
			// times. They are parked in the point itself instead —
			// in both components, because each loop overwrites the
			// one it read from as it goes.
			out->points[first + i].point.x = (float)flags;
			out->points[first + i].point.y = (float)flags;
			i++;
		}
	}

	for (uint16_t i = 0; i < points; i++) {
		uint8_t flags = (uint8_t)out->points[first + i].point.x;

		if (flags & GLYF_X_SHORT)
			x += (flags & GLYF_X_SAME) ? read_u8(&r)
						   : -(int32_t)read_u8(&r);
		else if (!(flags & GLYF_X_SAME))
			x += read_i16(&r);
		out->points[first + i].point.x = (float)x * scale + shift.x;
	}
	for (uint16_t i = 0; i < points; i++) {
		uint8_t flags = (uint8_t)out->points[first + i].point.y;

		if (flags & GLYF_Y_SHORT)
			y += (flags & GLYF_Y_SAME) ? read_u8(&r)
						   : -(int32_t)read_u8(&r);
		else if (!(flags & GLYF_Y_SAME))
			y += read_i16(&r);
		out->points[first + i].point.y = (float)y * scale + shift.y;
	}
	if (!r.ok || r.at > limit)
		return fail(error, VOE_BASE_ERROR_MALFORMED);

	out->point_count = (uint16_t)(first + points);
	return true;
}

// One composite glyph being expanded: where its next component record starts,
// where the composite itself sits, and the one uniform scale it and every
// frame above it have combined into.
struct composite_frame {
	uint32_t cursor;
	uint32_t limit;
	voe_math_float2 shift;
	float scale;
	bool more;
};

bool voe_text_truetype_outline_read(const voe_text_truetype *font,
				    uint16_t glyph, voe_base_arena *arena,
				    voe_text_truetype_outline *out,
				    voe_base_error *error)
{
	// Two pushes and not one per glyph, because two pushes are not
	// guaranteed to be next to each other and the arrays are indexed.
	struct composite_frame stack[VOE_TEXT_COMPOSITE_DEPTH];
	uint32_t depth = 0;
	uint16_t current = glyph;
	bool pending = true;
	voe_math_float2 shift = { 0.0f, 0.0f };
	float scale = 1.0f;

	VOE_BASE_ASSERT(font != NULL, "no font to read an outline out of");
	VOE_BASE_ASSERT(arena != NULL,
			"an outline has to be pushed somewhere: rule 11");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the outline");

	out->points = voe_base_arena_push(
		arena, sizeof *out->points * VOE_TEXT_MAX_POINTS);
	out->contour_ends = voe_base_arena_push(
		arena, sizeof *out->contour_ends * VOE_TEXT_MAX_CONTOURS);
	out->point_count = 0;
	out->contour_count = 0;

	// THE WALK, AND IT IS A STACK BECAUSE RULE 14 SAYS SO. `pending` is a
	// glyph waiting to be looked at; a simple one is appended and the walk
	// carries on with whatever component comes next, and a composite one
	// pushes a frame instead. Nothing here calls itself.
	while (pending || depth > 0) {
		if (pending) {
			uint32_t at;
			uint32_t length;
			struct reader r;
			int16_t contours;

			pending = false;
			if (!glyph_range(font, current, &at, &length))
				return fail(error, VOE_BASE_ERROR_MALFORMED);

			// An empty range is a glyph with nothing to draw, which
			// a space is. Not a failure and not a missing glyph.
			if (length != 0) {
				r = reader_at(font->bytes, font->size, at);
				contours = read_i16(&r);
				if (!r.ok)
					return fail(error,
						    VOE_BASE_ERROR_MALFORMED);

				if (contours >= 0) {
					if (!append_simple(font, at, length,
							   scale, shift, out,
							   error))
						return false;
				} else if (depth ==
					   VOE_TEXT_COMPOSITE_DEPTH) {
					return fail(error,
						    VOE_BASE_ERROR_UNSUPPORTED);
				} else {
					stack[depth++] = (struct composite_frame){
						.cursor = at + 10,
						.limit = at + length,
						.shift = shift,
						.scale = scale,
						.more = true,
					};
				}
			}
		}

		while (depth > 0) {
			struct composite_frame *frame = &stack[depth - 1];
			struct reader r;
			uint16_t flags;
			float dx;
			float dy;
			float component_scale;

			if (!frame->more) {
				depth--;
				continue;
			}

			r = reader_at(font->bytes, font->size, frame->cursor);
			flags = read_u16(&r);
			current = read_u16(&r);
			if (!r.ok)
				return fail(error, VOE_BASE_ERROR_MALFORMED);

			// Refused by name rather than mis-drawn. The embedded
			// fonts use neither: a component skewed independently
			// in x and y, a full 2x2 matrix, or one positioned by
			// matching a point in the glyph it joins is a reader
			// this card did not need. A uniform scale IS read below
			// — see COMPONENT_HAVE_SCALE's own comment.
			if (!(flags & COMPONENT_ARGS_ARE_XY) ||
			    (flags & (COMPONENT_HAVE_XY_SCALE |
				      COMPONENT_HAVE_TWO_BY_TWO)))
				return fail(error, VOE_BASE_ERROR_UNSUPPORTED);

			if (flags & COMPONENT_ARGS_ARE_WORDS) {
				dx = (float)read_i16(&r);
				dy = (float)read_i16(&r);
			} else {
				dx = (float)(int8_t)read_u8(&r);
				dy = (float)(int8_t)read_u8(&r);
			}
			// The transform, which sits right after the args: one
			// F2Dot14 for a uniform scale, or nothing at all.
			component_scale = (flags & COMPONENT_HAVE_SCALE) ?
						   (float)read_i16(&r) /
							   COMPONENT_SCALE_ONE :
						   1.0f;
			if (!r.ok || r.at > frame->limit)
				return fail(error, VOE_BASE_ERROR_MALFORMED);

			frame->cursor = r.at;
			// A composite's own hinting program sits after its last
			// component, so nothing has to step over it: the walk
			// stops reading components here and never looks past.
			frame->more = (flags & COMPONENT_MORE) != 0;

			// The offset is in the component's OWN space, so the
			// parent's scale applies to it too — a component ten
			// units to the right of a parent shrunk by half really
			// sits five units to the right. The scales themselves
			// simply multiply, because a uniform scale composes
			// with a uniform scale into one more uniform scale.
			shift.x = frame->scale * dx + frame->shift.x;
			shift.y = frame->scale * dy + frame->shift.y;
			scale = frame->scale * component_scale;
			pending = true;
			break;
		}
	}
	return true;
}
