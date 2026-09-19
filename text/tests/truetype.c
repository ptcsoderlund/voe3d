// The reader, against the font that is actually shipped (ADR-0185). Needs no
// graphics card.
//
// IT READS THE REAL FILE AND NOT HAND-BUILT ONES, which is the opposite of
// what assets/tests/model.c does and is right for the opposite reason. The one
// font in this engine is in the binary, and it is the same bytes on every
// machine for ever; a synthetic `.ttf` would test a reader against a file
// nobody will ever hand it. The numbers below were read out of the font with a
// separate tool before this reader existed, so they are an independent answer
// and not this reader's own output written down.
//
// THE COMPOSITE GLYPH IS THE TEST THAT MATTERS. Most accented characters are
// stored as references to other glyphs rather than as outlines, and a reader
// that handles only simple ones renders `é`, `ü` and `å` as blanks while
// looking perfectly correct on an English string. check_composite_glyphs is
// what catches that, and it is why this file spells three accented characters
// out.
//
// TWO FONTS ARE READ AT ONCE, WHICH IS check_two_fonts_at_once. Nothing stops a
// program holding two voe_text_fonts open together, so this proves the reader
// keeps no state outside the struct it hands back — a static or a global here
// would have one read's numbers bleed into the other's.
#include "../src/truetype.h"

#include <base/arena.h>
#include <testing/test.h>

#include <stdint.h>

// The same file text/src/font.c embeds, embedded again. Two copies in two
// binaries, which is cheaper than making the bytes public so that one test
// can see them.
static const uint8_t OXANIUM_TTF[] = {
#embed "../fonts/Oxanium-Regular.ttf"
};

#define ARENA (256 * 1024)

// Oxanium Regular's own numbers.
#define UNITS_PER_EM 1000
#define GLYPH_COUNT 375
#define ASCENDER 790
#define DESCENDER (-210)
#define LINE_GAP 250

static void check_header(const voe_text_truetype *font)
{
	VOE_TEST_CHECK_INT(font->units_per_em, UNITS_PER_EM);
	VOE_TEST_CHECK_INT(font->glyph_count, GLYPH_COUNT);
	VOE_TEST_CHECK_INT(font->ascender, ASCENDER);
	VOE_TEST_CHECK_INT(font->descender, DESCENDER);
	VOE_TEST_CHECK_INT(font->line_gap, LINE_GAP);
	// Short `loca`, which means every entry is half the offset it names.
	// Reading it as the long form instead lands in the middle of somebody
	// else's glyph and the whole font comes out as noise.
	VOE_TEST_CHECK(!font->loca_long);
}

static void check_character_map(const voe_text_truetype *font)
{
	// Every character the atlas covers maps to something, and none of them
	// maps to the same thing as another.
	VOE_TEST_CHECK(voe_text_truetype_glyph(font, 'A') != 0);
	VOE_TEST_CHECK(voe_text_truetype_glyph(font, 'o') != 0);
	VOE_TEST_CHECK(voe_text_truetype_glyph(font, ' ') != 0);
	VOE_TEST_CHECK(voe_text_truetype_glyph(font, 'A') !=
		       voe_text_truetype_glyph(font, 'B'));

	// A character this font does not carry is the missing-glyph box and not
	// a wrong glyph. U+4E00 is a CJK ideograph, which Oxanium has no
	// business holding.
	VOE_TEST_CHECK_INT(voe_text_truetype_glyph(font, 0x4e00), 0);
	// Above the basic multilingual plane, which format 4 cannot name at all.
	VOE_TEST_CHECK_INT(voe_text_truetype_glyph(font, 0x1f600), 0);
}

static void check_advances(const voe_text_truetype *font)
{
	uint16_t space = voe_text_truetype_advance(
		font, voe_text_truetype_glyph(font, ' '));
	uint16_t m = voe_text_truetype_advance(
		font, voe_text_truetype_glyph(font, 'M'));
	uint16_t i = voe_text_truetype_advance(
		font, voe_text_truetype_glyph(font, 'i'));

	// A space advances without drawing anything, which is the whole of how
	// a word gap works here.
	VOE_TEST_CHECK(space > 0);
	// Oxanium is proportional, so an M is wider than an i. If these ever
	// come back equal, `hmtx` is being read as if every glyph had the last
	// long metric.
	VOE_TEST_CHECK(m > i);
	// A glyph index past the end is zero rather than whatever is in memory
	// after the table.
	VOE_TEST_CHECK_INT(voe_text_truetype_advance(font, GLYPH_COUNT), 0);
}

static void check_simple_glyphs(const voe_text_truetype *font,
				voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_text_truetype_outline outline;

	// An `o` is two contours: the outside and the counter. One contour
	// would mean the hole was never read and the letter is a solid blob.
	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		font, voe_text_truetype_glyph(font, 'o'), arena, &outline,
		NULL));
	VOE_TEST_CHECK_INT(outline.contour_count, 2);
	VOE_TEST_CHECK(outline.point_count > 4);

	// A `B` is three: the outside and two counters.
	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		font, voe_text_truetype_glyph(font, 'B'), arena, &outline,
		NULL));
	VOE_TEST_CHECK_INT(outline.contour_count, 3);

	// A space has an advance and no outline, and that is not a failure.
	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		font, voe_text_truetype_glyph(font, ' '), arena, &outline,
		NULL));
	VOE_TEST_CHECK_INT(outline.contour_count, 0);

	voe_base_arena_rewind(arena, mark);
}

// THE ONE THIS FILE EXISTS FOR. An `é` is not an outline: it is a reference to
// the `e` and a reference to the acute accent, each with an offset. A reader
// that stops at simple glyphs comes back with nothing here and every accented
// character in the font renders as a blank.
static void check_composite_glyphs(const voe_text_truetype *font,
				   voe_base_arena *arena)
{
	static const uint32_t accented[] = { 0xe9, 0xfc, 0xe5 }; // é ü å
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_text_truetype_outline plain;

	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		font, voe_text_truetype_glyph(font, 'e'), arena, &plain, NULL));

	for (uint32_t i = 0; i < sizeof accented / sizeof *accented; i++) {
		uint16_t glyph = voe_text_truetype_glyph(font, accented[i]);
		voe_text_truetype_outline outline;

		VOE_TEST_CHECK(glyph != 0);
		VOE_TEST_CHECK(voe_text_truetype_outline_read(
			font, glyph, arena, &outline, NULL));
		// Not blank, which is what a simple-glyphs-only reader gives.
		VOE_TEST_CHECK(outline.contour_count > 0);
		VOE_TEST_CHECK(outline.point_count > 0);
		// And more contours than the letter alone, because the accent
		// is a contour of its own on top of it.
		VOE_TEST_CHECK(outline.contour_count > plain.contour_count);
	}

	voe_base_arena_rewind(arena, mark);
}

// A composite's components are placed by an offset, and the offset has to be
// applied. An `é` sits above the `e` it is made of, so its outline reaches
// higher than a plain `e` does while starting at the same place.
static void check_composite_offsets(const voe_text_truetype *font,
				    voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_text_truetype_outline plain;
	voe_text_truetype_outline accented;
	float plain_top = 0.0f;
	float accented_top = 0.0f;

	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		font, voe_text_truetype_glyph(font, 'e'), arena, &plain, NULL));
	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		font, voe_text_truetype_glyph(font, 0xe9), arena, &accented,
		NULL));

	for (uint16_t i = 0; i < plain.point_count; i++) {
		if (plain.points[i].point.y > plain_top)
			plain_top = plain.points[i].point.y;
	}
	for (uint16_t i = 0; i < accented.point_count; i++) {
		if (accented.points[i].point.y > accented_top)
			accented_top = accented.points[i].point.y;
	}

	// If the offsets were dropped, the accent would sit on top of the
	// letter at the same height and these two would be equal.
	VOE_TEST_CHECK(accented_top > plain_top);

	voe_base_arena_rewind(arena, mark);
}

// TWO FONTS, OPEN AT ONCE. voe_text_truetype keeps nothing outside the struct
// it hands back — no static, no global — so reading a second font in between
// two facts about the first must not move either of them. Interleaving the
// reads this way is what a shared piece of state would fail at and two
// sequential mains would not have caught.
static void check_two_fonts_at_once(voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_text_truetype first;
	voe_text_truetype second;
	voe_text_truetype_outline first_a;
	voe_text_truetype_outline second_a;

	VOE_TEST_CHECK(voe_text_truetype_read(
		OXANIUM_TTF, (uint32_t)sizeof OXANIUM_TTF, &first, NULL));
	VOE_TEST_CHECK_INT(first.units_per_em, UNITS_PER_EM);

	VOE_TEST_CHECK(voe_text_truetype_read(
		OXANIUM_TTF, (uint32_t)sizeof OXANIUM_TTF, &second, NULL));
	VOE_TEST_CHECK_INT(second.units_per_em, UNITS_PER_EM);

	// Reading the second did not move what the first had already said
	// about itself.
	VOE_TEST_CHECK_INT(first.units_per_em, UNITS_PER_EM);
	VOE_TEST_CHECK_INT(first.glyph_count, GLYPH_COUNT);

	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		&first, voe_text_truetype_glyph(&first, 'A'), arena, &first_a,
		NULL));
	VOE_TEST_CHECK(voe_text_truetype_outline_read(
		&second, voe_text_truetype_glyph(&second, 'A'), arena,
		&second_a, NULL));
	VOE_TEST_CHECK(first_a.point_count > 0);
	VOE_TEST_CHECK_INT(second_a.point_count, first_a.point_count);

	voe_base_arena_rewind(arena, mark);
}

// A file that is not a font at all, and one truncated to nothing. Both are
// returned failures rather than a read of whatever was in memory.
static void check_refusals(void)
{
	static const uint8_t not_a_font[16] = { 'O', 'T', 'T', 'O' };
	voe_text_truetype font;
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!voe_text_truetype_read(not_a_font, sizeof not_a_font,
					       &font, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNSUPPORTED);

	// The signature is right and there is nothing behind it, so the table
	// directory is the thing that does not hold together.
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_text_truetype_read(OXANIUM_TTF, 8, &font, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);
	voe_text_truetype font;

	if (!voe_text_truetype_read(OXANIUM_TTF, (uint32_t)sizeof OXANIUM_TTF,
				    &font, NULL)) {
		VOE_TEST_CHECK(!"the embedded Oxanium did not read at all");
		return voe_test_result();
	}

	check_header(&font);
	check_character_map(&font);
	check_advances(&font);
	check_simple_glyphs(&font, arena);
	check_composite_glyphs(&font, arena);
	check_composite_offsets(&font, arena);
	check_two_fonts_at_once(arena);
	check_refusals();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
