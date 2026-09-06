// Text on screen. The whole of what this folder offers: make the font, then turn
// a string into one mesh you can draw.
//
//     voe_text_font *font = voe_text_font_new(gpu, scratch, &error);
//     voe_text_block block;
//
//     voe_text_block_create(font, gpu, scratch, "Hello", 0.25f, &block, &error);
//     // block.geometry is a mesh; voe_text_font_atlas(font) is its picture.
//
// ONE TEXT BLOCK IS ONE OBJECT: ONE MESH, ONE MATERIAL, ONE DRAW. Not one per
// glyph. That is the load-bearing rule of this folder — it is what keeps a
// sentence from becoming a hundred blended objects with a hundred sort keys, and
// it is why a block is a geometry id and not a list of anything.
//
// THE MATERIAL IS THE CALLER'S TO BUILD, AND IT IS UNLIT AND BLENDED. This
// folder makes a mesh and a texture; what wears them is a `3d` material, and
// this folder may not name `3d`. The four things it must say are the atlas as
// the base colour texture, the tint as the base colour factor,
// VOE_RENDER_ALPHA_BLENDED as the alpha mode, and `unlit` set. Text is not lit
// by the sun: a letter that goes dark as a light moves round is a material that
// forgot the flag.
//
// A TEXT BLOCK IS BUILT ONCE AND DOES NOT CHANGE. voe_render_geometry_create
// waits for the graphics card to go idle and appends to a pool with no destroy,
// so building a block is a startup operation in the same sense uploading a model
// is. A block rebuilt every frame would stall the card and fill the pool until
// there was none left. Text that changes after it is built is a mechanism this
// engine does not have yet, and it is why the frame statistics are still printed
// on the console.
//
// EVERYTHING IS IN THE WORLD, IN THREE DIMENSIONS. A block is quads at z = 0
// facing +Z, in metres, and where it goes is a transform like anything else's:
// standing in the scene, or placed in front of the camera every frame so it
// reads as a heads-up display. There is no screen-space path here and a "just
// for debug" one is what that rule exists to prevent.
//
// THE FONT IS OXANIUM REGULAR AND IT IS IN THE BINARY. text/fonts/ holds the
// file and `#embed` puts it in the executable, so there is no path, no file I/O
// and no way for it to be missing — the same mechanism the compiled shaders use.
// The licence is the SIL Open Font License and it permits this provided the
// notice travels: text/fonts/OFL.txt is that notice, it is unmodified, and
// anything shipping this engine ships it too.
//
// ---- WHAT IT DOES NOT DO, EACH BY NAME ----
//
// NO KERNING, BECAUSE THIS FONT HAS NONE TO READ. Kerning here would mean the
// legacy `kern` table, and Oxanium — like most of what Google Fonts publishes
// now — carries its kerning in OpenType Layout's `GPOS` instead. Reading `GPOS`
// is a project rather than a paragraph, so there is no kerning at all rather
// than half of one.
//
// AND NO SHAPING, LIGATURES, BIDIRECTIONAL TEXT, VERTICAL SCRIPTS, HYPHENATION,
// WORD WRAP, RICH TEXT OR TAB STOPS. A string is laid out left to right, one
// advance at a time, and a newline starts a line. That is the whole layout
// engine.
//
// NO HINTING, NO SIGNED DISTANCE FIELDS, NO SUBPIXEL ANYTHING. Glyphs are
// rasterised once into one atlas with ordinary coverage anti-aliasing, at
// whatever size this folder picked, and scaled from there. Subpixel rendering is
// wrong the moment a quad turns, which here it does.
//
// ONE WEIGHT, ONE FONT, NO FALLBACK. There is no bold, no italic and no second
// face, and a character Oxanium does not carry is drawn as the font's own
// missing-glyph box rather than looked for somewhere else.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <render/device.h>

// The embedded font, read, rasterised into one atlas and uploaded. Long-lived:
// it is created once and destroyed at shutdown, which is the exception rule 11
// names.
typedef struct voe_text_font voe_text_font;

// One string, ready to draw.
//
// THE ORIGIN IS THE LEFT END OF THE FIRST LINE'S BASELINE, and the mesh runs
// right and up from there in metres, at z = 0. So a block placed at the origin
// has most of itself in +x and +y, with the descenders of the first line below
// zero. `size` is how far it reaches: the widest line's advance, and the height
// from the top of the first line to the bottom of the last.
typedef struct {
	voe_render_geometry geometry;
	voe_math_float2 size;
} voe_text_block;

// Reads the embedded font, rasterises every glyph it covers into one atlas and
// uploads it. `arena` is scratch — the outlines and the atlas pixels come out of
// it and nothing the font keeps does, so it may be rewound as soon as this
// returns.
//
// THE ATLAS IS BUILT HERE AND NOWHERE ELSE, WHICH IS WHY THIS IS A CALL AND NOT
// SOMETHING THE DEVICE DOES. A program that draws no text never calls this and
// pays nothing; a program that draws a hundred strings calls it once and they
// all sample the one texture.
//
// NULL on failure. UNSUPPORTED and MALFORMED come from the reader and mean it is
// wrong rather than the font is — the bytes are in the binary and are the same
// everywhere — and REFUSED means the graphics card would not take the atlas.
// error may be NULL.
[[nodiscard]] voe_text_font *voe_text_font_new(voe_render_device *device,
					       voe_base_arena *arena,
					       voe_base_error *error);

void voe_text_font_destroy(voe_text_font *font);

// The atlas, which is what a text material's base colour texture is.
//
// IT IS WHITE EVERYWHERE AND CARRIES THE GLYPHS IN ITS ALPHA, INCLUDING IN THE
// GAPS BETWEEN THEM. That is not a detail: a transparent pixel that is white
// rather than black cannot tint the edge of a glyph beside it when the sampler
// filters across the two. It is not premultiplied either — the shader multiplies
// colour by alpha once, at output, and an atlas that had already done it would
// be multiplied by its own coverage twice and the text would read as too faint.
voe_render_texture voe_text_font_atlas(const voe_text_font *font);

// Lays the string out and uploads it as one mesh. `em` is how many metres one em
// is, which is the one place a font's units become the world's; a capital letter
// comes out around seven tenths of it.
//
// `utf8` is UTF-8. A newline starts a new line; every other control character is
// laid out as whatever glyph the font maps it to, which is usually nothing. A
// character the font does not carry is drawn as its missing-glyph box.
//
// `arena` is scratch for the vertices and indices, and is the caller's again the
// moment this returns.
//
// False when the string is empty of anything to draw, when it holds more glyphs
// than VOE_TEXT_MAX_GLYPHS, or when `render` has no room left in its pools. All
// three are returned rather than fatal: a caller that could not fit a string is
// a caller that can say so and carry on. error may be NULL.
[[nodiscard]] bool voe_text_block_create(const voe_text_font *font,
					 voe_render_device *device,
					 voe_base_arena *arena,
					 const char *utf8, float em,
					 voe_text_block *out,
					 voe_base_error *error);

// The most glyphs one block may hold. It bounds the scratch one block costs —
// four vertices and six indices each — and it is a limit on a single string and
// not on how much text a program may draw, because a program may have as many
// blocks as it likes.
#define VOE_TEXT_MAX_GLYPHS 1024
