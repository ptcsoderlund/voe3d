// Text on screen. Two things this folder offers, and they are two paths on
// purpose: turn a string into one mesh you can draw in the world, or ask where
// one character sits and place it yourself.
//
//     voe_text_font *font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, gpu,
//                                             scratch, &error);
//     voe_text_block block;
//
//     voe_text_block_create(font, gpu, scratch, "Hello", 0.25f, &block, &error);
//     // block.geometry is a mesh; voe_text_font_atlas(font) is its picture.
//
//     voe_text_glyph g = voe_text_font_glyph(font, 'H');
//     // g says where the box is, what of the sheet to read, and how far to
//     // step. Where the pen goes next is the caller's.
//
// TWO TEXT PATHS NOW EXIST AND THAT IS CORRECT. A THIRD WOULD NOT BE. The mesh
// path is text standing in the world — a sign, a label on a thing, a readout in
// front of the camera — and it is one mesh, one material, one draw. The metrics
// path is text on a two-dimensional surface, where a caller emits one
// voe_render_element per character and the whole panel goes out as one draw
// command; `ui` is what does that and it is where a string is laid out. This
// folder has no third answer in it and adding one is how a folder ends up with
// three ways to draw the same letter that disagree at the edges.
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
// NO HINTING AND NO SUBPIXEL ANYTHING. Subpixel rendering is wrong the moment a
// quad turns, which here it does, and hinting is a bytecode interpreter for a
// problem a distance field does not have.
//
// AND NO ANALYTIC CURVES. The sheet is sampled at a fixed resolution, so a
// feature thinner than a texel or two — a hairline stem in text drawn very small
// — is one it cannot hold, and such text thins or breaks rather than blurring.
// Evaluating the outline itself in the fragment stage would be exact at every
// size and is written down as the right next move if this is ever not enough; it
// is a much larger piece of work and nothing has asked for it.
//
// ONE WEIGHT, ONE FONT, NO FALLBACK. A voe_text_font is one face at one weight:
// there is no bold and no italic. A character the face does not carry is drawn
// as its own missing-glyph box rather than looked for anywhere else.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <math/float4.h>
#include <render/device.h>

// The embedded font, read, rasterised into one atlas and uploaded. Long-lived:
// it is created once and destroyed at shutdown, which is the exception rule 11
// names.
typedef struct voe_text_font voe_text_font;

// Which embedded face a font draws from. An enum and not a path or a caller's
// bytes: the set is exactly the one face the binary carries, Oxanium, so naming
// it can never fail at a file's mercy and callers still name the face they
// make. A theme naming any other font gets Oxanium (ADR-0185).
//
// ONE FACE IS IN THE BINARY, OXANIUM, AND A FONT STILL NAMES IT (ADR-0185).
// text/fonts/ holds Oxanium Regular, and `#embed` puts it in the executable, so
// there is no path, no file I/O and no way for it to be missing — the same
// mechanism the compiled shaders use. A voe_text_typeface is not a path or a
// caller's bytes: it is a lookup into a set the binary always carries, which is
// what lets a theme file name a font without ever failing at a file's mercy.
// Oxanium's licence is the SIL Open Font License, which permits embedding, and
// its notice, text/fonts/OFL.txt, travels unmodified beside it — so anything
// shipping this engine ships it.
typedef enum {
	VOE_TEXT_TYPEFACE_OXANIUM,
} voe_text_typeface;

// One string, ready to draw.
//
// THE ORIGIN IS THE LEFT END OF THE FIRST LINE'S BASELINE, and the mesh runs
// right and up from there in metres, at z = 0. So a block placed at the origin
// has most of itself in +x and +y, with the descenders of the first line below
// zero. `size` is how far it reaches: the widest line's advance, and the height
// from the top of the first line to the bottom of the last.
//
// EVERYTHING IS IN THE WORLD, IN THREE DIMENSIONS. A block is quads at z = 0
// facing +Z, in metres, and where it goes is a transform like anything else's:
// standing in the scene, or placed in front of the camera every frame so it
// reads as a heads-up display. There is no screen-space path here and a "just
// for debug" one is what that rule exists to prevent.
//
// ONE TEXT BLOCK IS ONE OBJECT: ONE MESH, ONE MATERIAL, ONE DRAW. Not one per
// glyph. That is the load-bearing rule of this folder — it is what keeps a
// sentence from becoming a hundred blended objects with a hundred sort keys, and
// it is why a block is a geometry id and not a list of anything.
typedef struct {
	voe_render_geometry geometry;
	voe_math_float2 size;
} voe_text_block;

// Reads `typeface`'s embedded bytes, rasterises every glyph it covers into one
// atlas and uploads it. `arena` is scratch — the outlines and the atlas pixels
// come out of it and nothing the font keeps does, so it may be rewound as soon
// as this returns.
//
// THE ATLAS IS BUILT HERE AND NOWHERE ELSE, WHICH IS WHY THIS IS A CALL AND NOT
// SOMETHING THE DEVICE DOES. A program that draws no text never calls this and
// pays nothing; a program that draws a hundred strings in one face calls it
// once and they all sample the one texture. A program making two fonts calls
// it twice and pays for two atlases — nothing shares a sheet between fonts.
//
// NULL on failure. UNSUPPORTED and MALFORMED come from the reader and mean it is
// wrong rather than the font is — the bytes are in the binary and are the same
// everywhere — and REFUSED means the graphics card would not take the atlas.
// error may be NULL.
[[nodiscard]] voe_text_font *voe_text_font_new(voe_text_typeface typeface,
					       voe_render_device *device,
					       voe_base_arena *arena,
					       voe_base_error *error);

void voe_text_font_destroy(voe_text_font *font);

// The sheet, which is what a text material's base colour texture is.
//
// THE MATERIAL IS THE CALLER'S TO BUILD, AND IT IS UNLIT AND BLENDED. This
// folder makes a mesh and a texture; what wears them is a `3d` material, and
// this folder may not name `3d`. The five things it must say are the atlas as
// the base colour texture, the tint as the base colour factor,
// VOE_RENDER_ALPHA_BLENDED as the alpha mode, `unlit` set, and the base colour
// texture declared a distance field. Text is not lit by the sun: a letter that
// goes dark as a light moves round is a material that forgot the fourth. And a
// material that forgot the fifth draws every glyph as a solid rectangle, because
// the sheet is numbers and reading it as a picture is reading nonsense.
//
// IT IS NOT A PICTURE OF THE LETTERS. Each texel holds three signed distances to
// the nearest outline, in its first three channels; the fourth is opaque and is
// not read. A material has to say so — see above — because what the shader does
// with it is take the median of the three and compute its own alpha, and nothing
// checks that a caller meant to hand over a distance field.
//
// WHICH IS ALSO WHY IT IS SHARP AT ANY SIZE. A picture of a letter is only sharp
// at the size it was drawn at, and there is no such size here: text is in metres
// through a perspective camera, so it is magnified and minified constantly. A
// description of the shape has no size at all, and the shader recovers how wide
// the edge should be from how fast the field moves across one screen pixel.
//
// NOTHING HERE IS PREMULTIPLIED. The shader multiplies colour by alpha once, at
// output, on an alpha this sheet did not supply; there is nothing in it to
// premultiply and a caller must not add one.
voe_render_texture voe_text_font_atlas(const voe_text_font *font);

// Where one character sits, for a caller placing characters itself. Everything
// here is a fact about the font and none of it is a fact about a screen: this
// folder does not know what an element, a millimetre or a panel is.
//
// EVERYTHING IS IN EMS, WITH +Y UP, AND THE HEADER SAYS SO HERE BECAUSE IT IS
// THE ONE THING A CALLER WILL GET WRONG. An em is the font's own unit and one
// of them is however many millimetres or metres the caller decides it is — that
// multiplication happens once, at the caller, which is what makes a size
// measured from the font in hand rather than a number somebody typed. And +y is
// UP, because that is what a font means by up: `high.y` is the top of the box
// and `low.y` is the bottom, and `low.y` is BELOW the baseline — negative — for
// anything with a descender. A GUI surface runs y DOWN from its top-left
// corner, so a caller emitting onto one turns the direction round, once, where
// it emits. This folder never does.
//
// THE BOX IS THE BOX THE SHEET HOLDS AND NOT THE OUTLINE'S. It is wider than
// the letter on every side, because the field carries on past the outline and
// the quad has to be big enough to show the shader where the field says the
// edge is. A caller that shrinks it to what the letter looks like gets clipped
// stems, which reads as a bad font rather than as a wrong rectangle.
typedef struct {
	// The box's low corner relative to the pen: its left edge and its
	// bottom. In ems, +y up, so this is negative below the baseline.
	voe_math_float2 low;
	// The high corner: its right edge and its top.
	//
	// A LOW-AND-HIGH PAIR AND NOT A CORNER PLUS A SIZE, because a corner
	// plus a size has to name WHICH corner, and naming one would be this
	// folder deciding which way y runs. It does not get to.
	voe_math_float2 high;
	// What of the sheet this character is: `xy` the top-left corner and `zw`
	// the width and height, in texture coordinates. Straight into a
	// voe_render_element's `sheet`, which is the same shape for the same
	// reason.
	//
	// THE ONE PLACE THE TWO Y DIRECTIONS ARE RECONCILED IS HERE, AND IT IS
	// DONE ALREADY. A texture's (0,0) is its top-left and v runs down, while
	// the box above runs up — so `sheet.xy` is the sheet's corner for
	// `(low.x, high.y)`, the box's TOP-left. Pair the two rectangles corner
	// for corner and a letter comes out the right way up; pair them by name
	// and every letter is upside down in its own box, which looks like a
	// broken font and is not one.
	voe_math_float4 sheet;
	// How far the pen moves along the line, in ems. A character that draws
	// nothing still advances.
	float advance;
	// Whether there is anything to draw at all. FALSE FOR A SPACE, and for
	// every character the font maps to an empty outline. A caller that
	// ignores this spends a record on a rectangle of nothing — which on the
	// element path is a letter's worth of a frame's capacity for every space
	// in the string.
	bool drawn;
} voe_text_glyph;

// The metrics for one character. Never fails: the atlas covers U+0020 to
// U+00FF and a short list of symbols beyond it, √ among them, and a character
// it does not cover is the font's own missing-glyph box, which is what makes
// it visible rather than absent, and there is nothing here the world can
// refuse.
//
// It is a copy and not a pointer into the font, so a caller may keep it.
voe_text_glyph voe_text_font_glyph(const voe_text_font *font,
				   uint32_t codepoint);

// The distance from one baseline to the next, in ems, as the font holds it. A
// caller placing lines itself needs it and the font is what knows it; the mesh
// path uses the same number.
float voe_text_font_line_height(const voe_text_font *font);

// How much room a whole string takes, for a caller that has to know before it
// places anything. The convenience over voe_text_font_glyph and nothing more:
// this walks the string with the same decoder and the same advances a caller
// would, and hands back what they add up to.
//
// IN EMS, LIKE EVERY OTHER NUMBER THIS FOLDER PRODUCES, so a caller multiplies
// by its own millimetres-per-em or metres-per-em once and that multiplication is
// the only place a size becomes a size. Which is what makes a label measured
// from the font in hand rather than a number somebody typed: change the font and
// every label changes with it, because there is no table of advances anywhere
// above this call.
//
// `size` IS THE SAME BOX voe_text_block.size REPORTS, deliberately — the widest
// line's advance, and one line height for every line. The two paths measure the
// same string the same way or a caller comparing them finds them disagreeing.
typedef struct {
	// The widest line's advance, and the height from the top of the first
	// line to the bottom of the last. A string with nothing in it is nought
	// wide and one line high, because an empty label still occupies a line.
	voe_math_float2 size;
	// How far BELOW the top of that box the first line's baseline sits,
	// which is the font's own ascender.
	//
	// IT IS HERE BECAUSE THE BOX IS NOT WHERE A CALLER PUTS THE PEN. Every
	// other number in this folder is relative to a baseline, and a surface
	// laying text out has a rectangle instead — so somebody has to say how
	// the two line up, and the font is what knows. Add it to the top edge of
	// the rectangle, going DOWN if the surface runs y down, and that is the
	// baseline the glyph boxes are measured from.
	//
	// The rest of the line height — the descender and the line gap — is
	// below it, so a descender falls inside the box rather than under it.
	float baseline;
} voe_text_measure;

// Never fails: every character is either in the atlas or is the missing-glyph
// box, and there is nothing here the world can refuse. A newline starts a line,
// exactly as the mesh path's layout does.
voe_text_measure voe_text_font_measure(const voe_text_font *font,
				       const char *utf8);

// A TEXT BLOCK HAS TWO LIFETIMES AND TWO CALLS, AND THE LAYOUT IS THE SAME
// UNDERNEATH BOTH. voe_text_block_create is a startup operation: it goes through
// voe_render_geometry_create, which waits for the graphics card to go idle and
// appends to a pool with no destroy, so a label that never changes costs exactly
// one upload and is then drawn for the life of the program for nothing. That is
// its cache, and it is the only cache text has. voe_text_block_create_transient
// lives inside a frame: it goes through voe_render_geometry_create_transient, so
// the block is valid until that frame ends and is then gone, and text that
// changes — a readout, a label being edited — is laid out again every frame from
// the current string. Nothing here remembers a string, hashes one or asks
// whether it changed: a cache that misses an invalidation draws yesterday's text,
// which looks exactly like a program that has frozen. Rebuilding a few thousand
// glyphs a frame is a tenth of a millisecond and is the cost this design chose.
//
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

// The same layout into a block that lives one frame: identical in every argument
// and every answer except that the mesh goes through
// voe_render_geometry_create_transient, so it is drawn this frame and named by
// an id that is stale from the next frame's begin. Call it between the frame's
// begin and its end and nowhere else — the render call asserts otherwise — and
// call it again next frame with whatever the string is then.
//
// Fails for the three reasons above and for one more: `render` has no transient
// room left in this frame, which is REFUSED and means a transient capacity was
// chosen too small for what the frame builds.
[[nodiscard]] bool voe_text_block_create_transient(const voe_text_font *font,
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
