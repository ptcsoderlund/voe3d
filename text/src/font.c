// The font: the embedded file read, every glyph it covers rasterised into one
// atlas, and a string laid out into one mesh. See include/text/font.h for what
// this folder promises; this file is where the three conventions that meet in
// text are pinned down.
//
// ---- THE THREE SCALES, AND EACH IS NAMED ONCE ----
//
// FONT UNITS are what truetype.c hands over. There are `units_per_em` of them to
// an em and the number is the font's own — a thousand in Oxanium, two thousand
// and forty-eight in a lot of older faces — so nothing outside this file is ever
// shown one.
//
// TEXELS are what the atlas is measured in. ATLAS_EM is how many of them one em
// becomes when a glyph is measured into the sheet. It is not the resolution the
// text has for the rest of its life any more — the sheet holds a description of
// the shape rather than a picture of it — but it is how finely that shape was
// sampled, and a feature thinner than a texel or two is one it cannot hold.
//
// METRES are what the world is measured in. `em` is a parameter of
// voe_text_block_create, because how big a metre of text is belongs to the
// caller and not to the font; everything this file stores is in ems, so that
// multiplication happens once per vertex and nowhere else.
//
// ---- AND THE THREE Y AXES, WHICH IS THE OTHER THING TO GET WRONG ----
//
// FONT SPACE IS Y-UP FROM THE BASELINE: an ascender is positive and a descender
// is negative. THE ATLAS IS Y-DOWN, like every image — row zero is the top,
// which is what Vulkan, glTF and assets/include/assets/image.h all already
// agree on, so a texture coordinate of (0, 0) is the top-left corner.
// THE WORLD IS Y-UP. The flip between the first two is in raster.c, in one
// function, and there is no flip at all between the first and the third: a
// glyph's box comes out of here with +y still up, and the vertices below use it
// as it is.
//
// The engine's other Y flip — Vulkan's clip space, in render/src/frame.c — is
// not one of these and does not interact with them.
//
// ---- THE ATLAS IS A DISTANCE FIELD AND NOT A PICTURE OF ANYTHING ----
//
// Each texel holds three signed distances to the nearest outline, in its red,
// green and blue; the fourth channel is not read and is opaque everywhere. What
// draws it takes the median of the three and works out its own alpha from how
// fast that median moves across one screen pixel — see raster.h for why there
// are three and render/shaders/draw.slang for what the reader does with them.
// There is no coverage anywhere in this file and no picture of a letter at any
// size, which is what makes one sheet serve every size.
//
// IT GOES UP AS VOE_RENDER_TEXTURE_DATA AND ASKS FOR VOE_RENDER_SAMPLING_FIELD.
// DATA, because distances are numbers: uploaded as colour, the sRGB decode would
// bend every one of them towards zero and put every edge in the font slightly in
// the wrong place — uniformly and everywhere, which reads as the spread being
// wrong rather than as a format mistake.
//
// FIELD IS THE ONE FILTERED SAMPLER IN THE ENGINE AND THIS SHEET IS THE ONLY
// THING THAT ASKS FOR IT. It addresses CLAMP_TO_EDGE, because a sheet is not
// tiled and a coordinate a hair outside a glyph's box must not wrap to the far
// side of the atlas; that part it shares with SHARP, which is what this sheet
// used to ask for. What it adds is a linear filter, and the reason it is not
// the antialiasing card 026 swept out is that a distance is not a colour:
// interpolating between two colours blurs a picture, interpolating between two
// distances says where the outline runs between the two texel centres. Point
// sampled, the field is a plateau per texel and its crossing can only land on a
// texel boundary — which is one side of an O coming out a texel thicker than
// the other, at every size. Nothing about the edge is softer for it: the reader
// thresholds the median and a threshold keeps only the sign.
//
// NOTHING IS PREMULTIPLIED HERE AND NOTHING MAY BE. The shader's last act is
// `colour.rgb *= colour.a` (render/shaders/draw.slang), and it does that once,
// to an alpha this sheet did not supply. There is nothing in the sheet to
// multiply by and adding one is the classic way to make text thin and washed out
// rather than obviously broken.
#include "raster.h"
#include "truetype.h"
#include "utf8.h"

#include <text/font.h>

#include <base/assert.h>
#include <math/float2.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

// The font, in the binary. `#embed` is the mechanism the compiled shaders
// already use (ADR-0046) and it is why the Clang floor is 19. The path is
// relative to this file, which is what makes it need nothing from CMake.
//
// IT IS UNMODIFIED AND IT IS NOT RENAMED, and neither is an accident. The SIL
// Open Font License permits embedding provided the notice travels — that is
// text/fonts/OFL.txt, beside it, also unmodified — and leaving the file exactly
// as it was published is what keeps its reserved-name clause from engaging.
static const uint8_t OXANIUM_TTF[] = {
#embed "../fonts/Oxanium-Regular.ttf"
};

// The characters the atlas holds: Basic Latin from the space up, and the Latin-1
// supplement, which is where `é`, `ü` and `å` live. One contiguous run, so the
// table below is indexed by subtraction rather than searched.
//
// IT IS A RANGE AND NOT THE WHOLE FONT BECAUSE THE ATLAS IS FIXED AND BUILT
// ONCE. Oxanium carries three hundred and seventy-five glyphs and most of them
// are for languages nothing in this engine writes; the day something needs one,
// the card that needs it is the card that decides how the atlas grows.
#define FIRST_CHARACTER 0x20u
#define LAST_CHARACTER 0xffu
#define CHARACTER_COUNT (LAST_CHARACTER - FIRST_CHARACTER + 1u)

// One past the last character's slot, holding the font's own missing-glyph box.
// Anything not in the range above is drawn as this, which is what makes a
// character Oxanium does not carry visible rather than absent.
#define NOTDEF_SLOT CHARACTER_COUNT

// How big the sheet is and at what resolution a glyph's shape is measured into
// it.
//
// THIRTY-TWO, AND IT IS NOT A FUNCTION OF THE SCREEN ANY MORE — WHICH IS THE
// WHOLE OF WHAT CHANGED HERE. This number used to have to guess how many pixels
// a letter would land on, because the sheet held a picture of a letter and a
// picture is only sharp at the size it was drawn at. It holds a description of
// the letter's shape now, so the question it answers is a different one: how
// finely does the SHAPE have to be sampled for its outline to be recoverable
// from it. The answer is set by the thinnest thing in the font — a stem in
// Oxanium Regular is about eight hundredths of an em, so at thirty-two texels to
// the em it is two and a half texels across, and a feature narrower than about
// one and a half texels is one the field has nowhere to put. Everything above
// that is sharp at any size, so there is nothing bought by going higher and a
// megabyte to be paid for it.
//
// THE OLD ARGUMENT FOR FORTY IS GONE, NOT MERELY OUTVOTED, AND IT IS WORTH
// SAYING WHY. It ran: a sheet stored finer than the screen is minified, a
// minified texture is read out of the mipmap chain, and a blend between two
// pre-shrunk copies is blurrier than either — so storing more resolution made
// the ordinary case worse. Every step of that was true and it was a description
// of the SAMPLER, not of text. There is no mipmap chain in this engine at all
// any more, so there is nothing left to be read out of. Do not re-derive it.
//
// FIVE HUNDRED AND TWELVE SQUARE, WHICH THE RANGE ABOVE FITS IN WITH ROOM. One
// megabyte on the graphics card, once, for a program that draws any text at all.
#define ATLAS_PIXELS 512u
#define ATLAS_EM 32.0f

// Blank texels left round every glyph, and between one glyph and the next.
//
// IT IS THE SPREAD, AND IT REPLACES A PADDING THAT WAS ABOUT FILTERING. The
// field is written out to VOE_TEXT_FIELD_SPREAD texels either side of the
// outline, so a box drawn tighter than that would cut it off and two glyphs
// closer than that would have their fields run into each other — which is one
// letter's edge reappearing faintly beside another's. It is a floor and not a
// taste: raster.h owns the number and this follows it.
#define ATLAS_MARGIN ((uint32_t)VOE_TEXT_FIELD_SPREAD)

// The widest and tallest one glyph may rasterise to. Oxanium's largest is
// thirty-seven by forty-one at the size above, and no face puts a character more
// than about twice an em across, so this is slack rather than a limit anything
// approaches — and a glyph that did exceed it is refused rather than written
// past the end of the scratch bitmap.
#define GLYPH_PIXELS 128u

// Where one character sits, all of it in ems so that a caller's metre is applied
// once. The box is relative to the pen, with +y up, and is the box the sheet
// actually holds — which is ATLAS_MARGIN texels wider on every side than the
// outline, because the field carries on past the outline and the quad has to be
// big enough to show where it says the edge is.
struct glyph {
	float x0;
	float y0;
	float x1;
	float y1;
	float u0;
	float v0;
	float u1;
	float v1;
	float advance;
	bool drawn;
};

struct voe_text_font {
	voe_render_texture atlas;
	// From hhea: the distance from one baseline to the next, in ems.
	float line_height;
	struct glyph glyphs[CHARACTER_COUNT + 1];
};

// Where the next glyph goes in the atlas. Shelf packing: glyphs go along a row
// until one does not fit, then a new row starts below the tallest of them. It
// wastes the difference between the tallest glyph in a row and the rest, which
// for one font at one size is a few per cent — and the alternative is a packer,
// which is a card of its own for a saving nothing needs.
struct shelf {
	uint32_t x;
	uint32_t y;
	uint32_t height;
};

static bool fail(voe_base_error *error, voe_base_error code)
{
	if (error != NULL)
		*error = code;
	return false;
}

// The outline's extent in font units. Control points are included, so the box
// can be a hair larger than the curve actually reaches; that costs a pixel of
// atlas and never clips a glyph, which is the right way round to be wrong.
static bool outline_bounds(const voe_text_truetype_outline *outline,
			   voe_math_float2 *low, voe_math_float2 *high)
{
	if (outline->point_count == 0 || outline->contour_count == 0)
		return false;

	*low = outline->points[0].point;
	*high = outline->points[0].point;
	for (uint16_t i = 1; i < outline->point_count; i++) {
		voe_math_float2 p = outline->points[i].point;

		if (p.x < low->x)
			low->x = p.x;
		if (p.y < low->y)
			low->y = p.y;
		if (p.x > high->x)
			high->x = p.x;
		if (p.y > high->y)
			high->y = p.y;
	}
	return true;
}

// One glyph: measured into `bitmap` as a field, copied into the atlas, and its
// box written into `out`. `bitmap` is scratch of GLYPH_PIXELS squared times
// VOE_TEXT_FIELD_CHANNELS and arrives holding whatever the last glyph left in
// it; voe_text_raster_field writes every texel of the part it is given.
static bool add_glyph(const voe_text_truetype *ttf, uint16_t index,
		      voe_base_arena *arena, uint8_t *bitmap, uint8_t *atlas,
		      struct shelf *shelf, struct glyph *out,
		      voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	voe_text_truetype_outline outline;
	voe_math_float2 low;
	voe_math_float2 high;
	voe_math_float2 origin;
	float scale = ATLAS_EM / (float)ttf->units_per_em;
	uint32_t width;
	uint32_t height;
	bool ok = true;

	*out = (struct glyph){ .advance = (float)voe_text_truetype_advance(
					  ttf, index) /
				          (float)ttf->units_per_em };

	if (!voe_text_truetype_outline_read(ttf, index, arena, &outline, error)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}
	// A glyph with nothing to draw. A space is one, and so is every
	// character the font maps to an empty outline; it still advances.
	if (!outline_bounds(&outline, &low, &high)) {
		voe_base_arena_rewind(arena, mark);
		return true;
	}

	// ATLAS_MARGIN texels on every side, because the field is written that
	// far outside the outline and a box drawn tight to the outline would cut
	// it off. This is the spread, not a filtering allowance.
	width = (uint32_t)ceilf((high.x - low.x) * scale) + 2u * ATLAS_MARGIN;
	height = (uint32_t)ceilf((high.y - low.y) * scale) + 2u * ATLAS_MARGIN;
	if (width > GLYPH_PIXELS || height > GLYPH_PIXELS)
		ok = fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	if (ok && shelf->x + width + ATLAS_MARGIN > ATLAS_PIXELS) {
		shelf->x = 0;
		shelf->y += shelf->height + ATLAS_MARGIN;
		shelf->height = 0;
	}
	if (ok && shelf->y + height > ATLAS_PIXELS)
		ok = fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	if (ok) {
		float margin = (float)ATLAS_MARGIN;

		// Where font-unit (0, 0) — the left end of the baseline — lands
		// in the glyph's own bitmap. x is the margin in from the left
		// edge of the outline; y is the margin below the top of it, and
		// it is a plus because the bitmap counts downwards while the
		// font counts upwards.
		origin.x = margin - low.x * scale;
		origin.y = margin + high.y * scale;

		voe_text_raster_field(&outline, scale, origin, bitmap,
				      (uint16_t)width, (uint16_t)height, arena);

		// The three distances go into the sheet's first three channels
		// and the fourth is left opaque. Nothing reads the fourth — see
		// the header — and leaving it at zero would be a sheet that
		// looked like a picture of nothing to anybody debugging it.
		for (uint32_t r = 0; r < height; r++) {
			uint8_t *row = atlas + ((size_t)(shelf->y + r) *
							ATLAS_PIXELS +
						shelf->x) *
					               4u;

			for (uint32_t c = 0; c < width; c++) {
				const uint8_t *from =
					bitmap + ((size_t)r * width + c) *
							 VOE_TEXT_FIELD_CHANNELS;

				row[c * 4u + 0u] = from[0];
				row[c * 4u + 1u] = from[1];
				row[c * 4u + 2u] = from[2];
			}
		}

		out->drawn = true;
		out->x0 = (low.x * scale - margin) / ATLAS_EM;
		out->x1 = out->x0 + (float)width / ATLAS_EM;
		out->y1 = (high.y * scale + margin) / ATLAS_EM;
		out->y0 = out->y1 - (float)height / ATLAS_EM;
		out->u0 = (float)shelf->x / (float)ATLAS_PIXELS;
		out->v0 = (float)shelf->y / (float)ATLAS_PIXELS;
		out->u1 = (float)(shelf->x + width) / (float)ATLAS_PIXELS;
		out->v1 = (float)(shelf->y + height) / (float)ATLAS_PIXELS;

		shelf->x += width + ATLAS_MARGIN;
		if (height > shelf->height)
			shelf->height = height;
	}

	voe_base_arena_rewind(arena, mark);
	return ok;
}

voe_text_font *voe_text_font_new(voe_render_device *device,
				 voe_base_arena *arena, voe_base_error *error)
{
	struct voe_base_arena_mark mark;
	voe_text_truetype ttf;
	voe_text_font *font;
	struct shelf shelf = { 0 };
	uint8_t *pixels;
	uint8_t *bitmap;
	bool ok = true;

	VOE_BASE_ASSERT(device != NULL, "a font needs a device to upload to");
	VOE_BASE_ASSERT(arena != NULL,
			"building an atlas needs scratch: rule 11");

	if (!voe_text_truetype_read(OXANIUM_TTF, (uint32_t)sizeof OXANIUM_TTF,
				    &ttf, error))
		return NULL;

	font = calloc(1, sizeof *font);
	font->line_height = (float)(ttf.ascender - ttf.descender +
				    ttf.line_gap) /
			    (float)ttf.units_per_em;

	mark = voe_base_arena_mark(arena);
	// One push and not one per row: two pushes are not guaranteed to be
	// next to each other (base/arena.h) and this is indexed as one array.
	pixels = voe_base_arena_push(arena,
				     (size_t)ATLAS_PIXELS * ATLAS_PIXELS * 4u);
	bitmap = voe_base_arena_push(arena, (size_t)GLYPH_PIXELS *
						    GLYPH_PIXELS *
						    VOE_TEXT_FIELD_CHANNELS);

	// Every texel as far outside the shape as the field goes, and opaque,
	// before a single glyph is measured. The gaps between glyphs are then
	// distances that say "nothing here" rather than distances that say the
	// outline is exactly on top of them, which is what a zeroed fourth
	// channel and a zeroed field would have meant in two different
	// directions.
	for (size_t i = 0; i < (size_t)ATLAS_PIXELS * ATLAS_PIXELS; i++) {
		pixels[i * 4u + 0u] = 0;
		pixels[i * 4u + 1u] = 0;
		pixels[i * 4u + 2u] = 0;
		pixels[i * 4u + 3u] = 255;
	}

	// The missing-glyph box first, so that it is in the atlas whatever
	// happens to the rest, and then the range.
	ok = add_glyph(&ttf, 0, arena, bitmap, pixels, &shelf,
		       &font->glyphs[NOTDEF_SLOT], error);
	for (uint32_t c = 0; ok && c < CHARACTER_COUNT; c++) {
		uint16_t index = voe_text_truetype_glyph(&ttf,
							 FIRST_CHARACTER + c);

		ok = add_glyph(&ttf, index, arena, bitmap, pixels, &shelf,
			       &font->glyphs[c], error);
	}

	// DATA and FIELD, and neither is optional: see the header. A sheet
	// uploaded as colour has the sRGB decode run over its distances, and a
	// sheet sampled unfiltered is a grid of plateaus rather than a field.
	if (ok)
		ok = voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
					       VOE_RENDER_SAMPLING_FIELD,
					       ATLAS_PIXELS, ATLAS_PIXELS,
					       pixels, &font->atlas, error);

	voe_base_arena_rewind(arena, mark);
	if (!ok) {
		free(font);
		return NULL;
	}
	return font;
}

void voe_text_font_destroy(voe_text_font *font)
{
	// The atlas texture is not given back here: destroying it needs the
	// device, and a font outliving the device it uploaded to is not a shape
	// this engine has. Both go at shutdown and the device takes its own
	// slots with it.
	free(font);
}

voe_render_texture voe_text_font_atlas(const voe_text_font *font)
{
	VOE_BASE_ASSERT(font != NULL, "no font to take an atlas from");

	return font->atlas;
}

// Which slot a character is drawn from. Anything outside the range the atlas
// holds is the missing-glyph box, which is the whole of what this engine does
// about a character Oxanium does not carry.
static const struct glyph *glyph_of(const voe_text_font *font,
				    uint32_t codepoint)
{
	if (codepoint < FIRST_CHARACTER || codepoint > LAST_CHARACTER)
		return &font->glyphs[NOTDEF_SLOT];
	return &font->glyphs[codepoint - FIRST_CHARACTER];
}

bool voe_text_block_create(const voe_text_font *font,
			   voe_render_device *device, voe_base_arena *arena,
			   const char *utf8, float em, voe_text_block *out,
			   voe_base_error *error)
{
	struct voe_base_arena_mark mark;
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t glyphs = 0;
	float pen_x = 0.0f;
	float pen_y = 0.0f;
	float widest = 0.0f;
	bool ok = true;

	VOE_BASE_ASSERT(font != NULL, "no font to lay a string out with");
	VOE_BASE_ASSERT(device != NULL, "a block needs a device to upload to");
	VOE_BASE_ASSERT(arena != NULL,
			"laying a string out needs scratch: rule 11");
	VOE_BASE_ASSERT(utf8 != NULL, "no string to lay out");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the block");
	VOE_BASE_ASSERT(em > 0.0f, "a text block of no size at all");

	mark = voe_base_arena_mark(arena);
	vertices = voe_base_arena_push(
		arena, sizeof *vertices * 4u * VOE_TEXT_MAX_GLYPHS);
	indices = voe_base_arena_push(arena,
				      sizeof *indices * 6u * VOE_TEXT_MAX_GLYPHS);

	for (const char *at = utf8; *at != '\0';) {
		uint32_t codepoint;
		const struct glyph *g;
		float x0;
		float x1;
		float y0;
		float y1;
		uint32_t v;

		at += voe_text_utf8_next(at, &codepoint);

		if (codepoint == '\n') {
			if (pen_x > widest)
				widest = pen_x;
			pen_x = 0.0f;
			pen_y -= font->line_height;
			continue;
		}

		g = glyph_of(font, codepoint);
		if (g->drawn) {
			if (glyphs == VOE_TEXT_MAX_GLYPHS) {
				ok = fail(error, VOE_BASE_ERROR_UNSUPPORTED);
				break;
			}

			// Ems into metres, and this is the only multiplication
			// by `em` in the folder.
			x0 = (pen_x + g->x0) * em;
			x1 = (pen_x + g->x1) * em;
			y0 = (pen_y + g->y0) * em;
			y1 = (pen_y + g->y1) * em;
			v = glyphs * 4u;

			// Corner for corner and index for index the same shape
			// dev/src/quad.c uses, which is the winding already
			// proven to survive back-face culling from +Z. Facing
			// the other way is a mesh that is invisible from half
			// the scene and nothing that fails.
			vertices[v + 0] = (voe_render_vertex){
				{ x0, y1, 0.0f }, { 0.0f, 0.0f, 1.0f },
				{ g->u0, g->v0 }
			};
			vertices[v + 1] = (voe_render_vertex){
				{ x1, y1, 0.0f }, { 0.0f, 0.0f, 1.0f },
				{ g->u1, g->v0 }
			};
			vertices[v + 2] = (voe_render_vertex){
				{ x1, y0, 0.0f }, { 0.0f, 0.0f, 1.0f },
				{ g->u1, g->v1 }
			};
			vertices[v + 3] = (voe_render_vertex){
				{ x0, y0, 0.0f }, { 0.0f, 0.0f, 1.0f },
				{ g->u0, g->v1 }
			};

			indices[glyphs * 6u + 0] = v + 3;
			indices[glyphs * 6u + 1] = v + 2;
			indices[glyphs * 6u + 2] = v + 1;
			indices[glyphs * 6u + 3] = v + 3;
			indices[glyphs * 6u + 4] = v + 1;
			indices[glyphs * 6u + 5] = v + 0;
			glyphs++;
		}

		pen_x += g->advance;
	}

	if (pen_x > widest)
		widest = pen_x;

	// A string of nothing but spaces has no mesh, and a mesh of no vertices
	// is the caller's bug at voe_render_geometry_create. Refused here
	// instead, with the same code an over-long string gets.
	if (ok && glyphs == 0)
		ok = fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	if (ok)
		ok = voe_render_geometry_create(device, vertices, glyphs * 4u,
						indices, glyphs * 6u,
						&out->geometry, error);
	if (ok) {
		// The height is from the top of the first line to the bottom of
		// the last, which is one line height per line — pen_y is
		// already the negative of all the lines but the first.
		out->size.x = widest * em;
		out->size.y = (font->line_height - pen_y) * em;
	}

	voe_base_arena_rewind(arena, mark);
	return ok;
}
