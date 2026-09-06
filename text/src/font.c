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
// PIXELS are what the atlas is measured in. ATLAS_EM is how many of them one em
// becomes when a glyph is rasterised, and it is the resolution the text has for
// the rest of its life: there is one atlas, built once, and drawing bigger than
// this magnifies it. `ATLAS_SCALE` is the one multiplication between the two.
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
// ---- THE ATLAS IS WHITE WITH THE GLYPHS IN ITS ALPHA ----
//
// Every pixel is (255, 255, 255) and the coverage goes in the fourth channel,
// the gaps between glyphs included. A gap that was black would tint the edge of
// a glyph next to it as soon as the sampler filtered across the two, and that is
// a grey fringe nobody can find afterwards. It is uploaded as
// VOE_RENDER_TEXTURE_COLOUR: white survives the sRGB decode as white, and alpha
// is never decoded, so the coverage arrives exactly as it was written.
//
// IT IS NOT PREMULTIPLIED AND MUST NOT BE. The shader's last act is
// `colour.rgb *= colour.a` (render/shaders/draw.slang); an atlas that had
// already multiplied would be multiplied by its own coverage twice, and the
// result is text that looks thin and washed out rather than obviously wrong.
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

// How big the atlas is and how big a glyph is drawn into it. Forty pixels to the
// em packs the range above into about four hundred rows of five hundred and
// twelve, which leaves room for the range to grow and costs one megabyte on the
// graphics card once.
//
// FORTY AND NOT MORE, AND BIGGER IS NOT SHARPER — THAT IS THE WHOLE OF THIS
// NUMBER AND IT IS THE OPPOSITE OF WHAT IT LOOKS LIKE. An atlas coarser than the
// screen is magnified, which goes soft in the obvious way. An atlas FINER than
// the screen is minified, and a minified texture is read out of the mipmap
// chain: a glyph stored at eighty pixels and drawn at thirty is fetched from
// between the half-size and the quarter-size copy and blended between them, and
// that blend is blurrier than either of them. So the sharpest atlas is the one
// whose texels are about the size of the pixels the text lands on, and asking
// for headroom by storing it larger makes the ordinary case worse. Eighty was
// tried first and is visibly soft at the sizes dev draws.
//
// WHICH MEANS THERE IS ONE RIGHT ANSWER PER SCREEN SIZE, AND THIS IS A FIXED
// NUMBER. Text a long way off still minifies and text walked up to still
// magnifies; forty is chosen for text that fills a few tens of pixels to the em,
// which is what a line of writing in a scene does. A card that wants text sharp
// at every size is the card that brings signed distance fields, and this
// paragraph is what it replaces.
#define ATLAS_PIXELS 512u
#define ATLAS_EM 40.0f

// Empty pixels between one glyph and the next in the atlas. Two, because the
// sampler filters between neighbouring pixels and the mipmap chain filters
// between more of them than that; what stops the bleed being visible is the
// white above rather than this, and this is what stops one glyph's coverage
// reaching another's edge at the first mip level.
#define ATLAS_PAD 2u

// The widest and tallest one glyph may rasterise to. Oxanium's largest is
// thirty-seven by forty-one at the size above, and no face puts a character more
// than about twice an em across, so this is slack rather than a limit anything
// approaches — and a glyph that did exceed it is refused rather than written
// past the end of the scratch bitmap.
#define GLYPH_PIXELS 128u

// Where one character sits, all of it in ems so that a caller's metre is applied
// once. The box is relative to the pen, with +y up, and is the box the atlas
// actually holds — which is a pixel wider on every side than the outline, so a
// filtered edge has something to fade into.
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

// One glyph: rasterised into `bitmap`, copied into the atlas, and measured into
// `out`. `bitmap` is scratch of GLYPH_PIXELS squared and arrives holding
// whatever the last glyph left in it.
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

	// One pixel of margin on every side, so the outermost coverage a
	// filtered sample can reach is a pixel of nothing rather than the edge
	// of the box.
	width = (uint32_t)ceilf((high.x - low.x) * scale) + 2u;
	height = (uint32_t)ceilf((high.y - low.y) * scale) + 2u;
	if (width > GLYPH_PIXELS || height > GLYPH_PIXELS)
		ok = fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	if (ok && shelf->x + width + ATLAS_PAD > ATLAS_PIXELS) {
		shelf->x = 0;
		shelf->y += shelf->height + ATLAS_PAD;
		shelf->height = 0;
	}
	if (ok && shelf->y + height > ATLAS_PIXELS)
		ok = fail(error, VOE_BASE_ERROR_UNSUPPORTED);

	if (ok) {
		// Where font-unit (0, 0) — the left end of the baseline — lands
		// in the glyph's own bitmap. x is one pixel in from the left
		// edge of the outline; y is one pixel below the top of it, and
		// it is a plus because the bitmap counts downwards while the
		// font counts upwards.
		origin.x = 1.0f - low.x * scale;
		origin.y = 1.0f + high.y * scale;

		memset(bitmap, 0, (size_t)width * height);
		voe_text_raster_fill(&outline, scale, origin, bitmap,
				     (uint16_t)width, (uint16_t)height, arena);

		for (uint32_t r = 0; r < height; r++) {
			uint8_t *row = atlas + ((size_t)(shelf->y + r) *
							ATLAS_PIXELS +
						shelf->x) *
					               4u;

			for (uint32_t c = 0; c < width; c++)
				row[c * 4u + 3u] = bitmap[r * width + c];
		}

		out->drawn = true;
		out->x0 = (low.x * scale - 1.0f) / ATLAS_EM;
		out->x1 = out->x0 + (float)width / ATLAS_EM;
		out->y1 = (high.y * scale + 1.0f) / ATLAS_EM;
		out->y0 = out->y1 - (float)height / ATLAS_EM;
		out->u0 = (float)shelf->x / (float)ATLAS_PIXELS;
		out->v0 = (float)shelf->y / (float)ATLAS_PIXELS;
		out->u1 = (float)(shelf->x + width) / (float)ATLAS_PIXELS;
		out->v1 = (float)(shelf->y + height) / (float)ATLAS_PIXELS;

		shelf->x += width + ATLAS_PAD;
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
	bitmap = voe_base_arena_push(arena, (size_t)GLYPH_PIXELS * GLYPH_PIXELS);

	// White everywhere, transparent everywhere, before a single glyph is
	// drawn — see the header on why the gaps are white and not black.
	for (size_t i = 0; i < (size_t)ATLAS_PIXELS * ATLAS_PIXELS; i++) {
		pixels[i * 4u + 0u] = 255;
		pixels[i * 4u + 1u] = 255;
		pixels[i * 4u + 2u] = 255;
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

	if (ok)
		ok = voe_render_texture_create(device,
					       VOE_RENDER_TEXTURE_COLOUR,
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
