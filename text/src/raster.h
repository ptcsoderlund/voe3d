// An outline, turned into pixels. Internal to this folder, and the half of glyph
// rendering that has nothing to do with file formats: it is handed points and
// hands back either how much of each pixel the shape covers, or how far each
// texel is from the shape's edge.
//
// TWO ANSWERS FROM ONE FLATTENER, AND THE SECOND IS WHAT THE ATLAS IS MADE OF.
// voe_text_raster_fill is coverage — a stored picture of the shape at one size.
// voe_text_raster_field is three signed distances per texel, which is a
// description of the shape at no size at all and is what lets text be sharp when
// it is magnified. Both walk the same flattened line segments and both get their
// sign from the same thing: which way round a contour runs.
//
// NON-ZERO WINDING, WHICH IS WHAT TRUETYPE MEANS, AND CONTOUR DIRECTION IS
// LOAD-BEARING. Each crossing of a scanline counts +1 or −1 depending on which
// way the edge is going, and a pixel is inside the shape where the running total
// is anything but zero. That is the whole of the fill rule and it is why a
// glyph's counters are holes: the outer contour of an `o` runs one way round and
// the inner one runs the other, so their windings cancel in the middle. Sum the
// crossings without their sign — or use an even-odd rule on a glyph whose
// contours overlap — and the counters in `o`, `e`, `a`, `p`, `B` and `8` fill in
// solid. It is visible in the first word rendered.
//
// ---- AND THE FIELD, WHICH IS THE HARDER HALF ----
//
// THREE DISTANCES AND NOT ONE, BECAUSE ONE ROUNDS OFF EVERY CORNER. A single
// signed distance field is smooth, and the linear filter that reads it between
// texels turns a right angle into an arc — Oxanium's flat-cut terminals and
// squared joins are exactly what that ruins. So each edge of the outline is
// assigned two of the three channels, chosen so that the two edges meeting at a
// corner do not share all of theirs, and each channel measures distance only to
// the edges it was given. A reader takes the MEDIAN of the three: away from a
// corner all three agree and the median is simply the distance, and at a corner
// two of them agree on the correct side while the third is the one that would
// have rounded it, so the median reconstructs the corner exactly.
//
// THE SIGN COMES FROM WHICH WAY ROUND THE CONTOUR RUNS, WHICH IS THE SAME FACT
// THE NON-ZERO WINDING RULE IS BUILT ON. Positive is inside. It is per edge and
// not per texel — a texel's three channels may legitimately disagree about their
// signs near a corner, and that disagreement is the whole mechanism. Where the
// median's sign nonetheless disagrees with the fill rule by more than half a
// texel, the field is wrong rather than clever and the texel is replaced by the
// plain signed distance; that is the one place the two halves of this file meet.
//
// THE Y FLIP IS HERE AND IT IS THE ONLY ONE IN THIS FOLDER. Font units are Y-up
// from the baseline and a bitmap's rows go down the image, so this file negates
// y exactly once, where the outline is turned into bitmap coordinates. The
// engine's other Y flip is Vulkan's, in render/src/frame.c, and the two are
// unrelated: one is a picture being written and the other is clip space.
#pragma once

#include "truetype.h"

#include <base/arena.h>
#include <math/float2.h>

#include <stdint.h>

// How many heights each row of pixels is sampled at. Eight is where a curved
// stem stops showing steps at the sizes text/src/font.c rasterises at.
//
// THE ANTI-ALIASING IS SUB-SCANLINES IN Y AND EXACT SPANS IN X. Each row of
// pixels is sampled at VOE_TEXT_RASTER_SAMPLES evenly spaced heights; along each
// of those the spans that are inside the shape are added to the row with their
// fractional ends counted as fractions. So a vertical edge is exact and a
// horizontal one is quantised to one part in VOE_TEXT_RASTER_SAMPLES, which is
// the trade this makes and the reason that number is not smaller.
#define VOE_TEXT_RASTER_SAMPLES 8

// How far a flattened curve may sit from the curve it replaces, in pixels. A
// tenth of a pixel is below what the coverage above can represent, so the
// flattening is not what limits the quality.
//
// CURVES ARE FLATTENED TO LINES AND VOE_TEXT_RASTER_TOLERANCE IS HOW STRAIGHT
// THAT HAS TO BE. `glyf` outlines are quadratic Béziers; each is split into as
// many equal pieces as it takes for the chord to sit within the tolerance of the
// curve, in the pixels the caller is rasterising into rather than in font units,
// so a big glyph gets more pieces than a small one for free.
#define VOE_TEXT_RASTER_TOLERANCE 0.1f

// How far out from the outline the field is encoded, in texels. A byte per
// channel spans plus or minus this, so 0 is that far outside, 255 is that far
// inside and 128 is the outline itself.
//
// THE SPREAD IS HOW FAR OUT THE FIELD IS ENCODED AND IT COSTS PADDING. Distances
// are stored as a byte per channel over the range plus or minus
// VOE_TEXT_FIELD_SPREAD texels, so a caller has to leave at least that much
// blank around each glyph or two glyphs' fields run into one another.
//
// FOUR, AND IT IS A TRADE BETWEEN TWO THINGS AND NOT A QUALITY SETTING. It has
// to be wide enough that a reader minifying the text still finds a gradient to
// smooth across — one screen pixel spanning k texels sees the encoded range move
// by k/(2·spread), so at a spread of four the field still has meaning down to
// about eight texels to the pixel, which is text a long way off. It has to be
// narrow enough that a byte resolves it: four texels either side over 256 steps
// is a thirty-second of a texel, far finer than anything the edge is placed to.
// And every texel of it is padding around every glyph in the sheet, which is
// what stops it being made larger for free.
//
// render/shaders/elements.slang holds a copy, VOE_RENDER_ELEMENT_FIELD_SPREAD,
// from which its glyph cutoff measures half a screen pixel less one byte step
// of the field (ADR-0183, ADR-0184); change both or neither.
#define VOE_TEXT_FIELD_SPREAD 4.0f

// How sharp a turn between two edges of the outline has to be before they are
// made to differ in their channels, in degrees.
//
// THREE, WHICH IS SMALL BECAUSE IT IS MEASURED AT THE OUTLINE'S OWN JOINS AND
// NOT BETWEEN FLATTENED PIECES. A curve broken into straight pieces turns by
// tens of degrees at every piece, so a threshold applied there would call every
// curve a string of corners and the colouring would be noise. It is applied
// between one `glyf` edge and the next instead — a line or a whole quadratic —
// where a smooth join turns by very nearly nothing and a real corner turns by
// tens of degrees. Three degrees separates those two populations with room to
// spare, and it is msdfgen's own default for the same reason.
#define VOE_TEXT_FIELD_CORNER_DEGREES 3.0f

// How many channels the field has. Three, and it is not a number to change: the
// median of three is what reconstructs a corner, and the reader is a texture
// sample's rgb.
#define VOE_TEXT_FIELD_CHANNELS 3

// Fills `coverage`, which is `width` by `height` bytes with row 0 at the top of
// the image and is expected to arrive zeroed.
//
// `scale` turns font units into pixels. `origin` is where font-unit (0, 0) — the
// left end of the baseline — lands in the bitmap, in pixels, with x to the right
// and y downwards. Anything the outline covers outside the bitmap is clipped and
// is not an error, which is what lets a caller round a bounding box outwards
// without having to be exact.
//
// `arena` is scratch: the flattened segments and one row of accumulated coverage
// come out of it, and it may be rewound the moment this returns.
//
// COVERAGE IS ONE BYTE PER PIXEL AND IT IS NOT A COLOUR. 0 is outside the shape
// and 255 is entirely inside it; what the caller does with that is the caller's.
void voe_text_raster_fill(const voe_text_truetype_outline *outline, float scale,
			  voe_math_float2 origin, uint8_t *coverage,
			  uint16_t width, uint16_t height,
			  voe_base_arena *arena);

// Fills `field`, which is `width` by `height` by VOE_TEXT_FIELD_CHANNELS bytes
// with row 0 at the top of the image. Unlike the fill above it writes every
// texel, so it does not need to arrive zeroed.
//
// `scale`, `origin`, the clipping and `arena` all mean what they mean above. The
// caller is responsible for leaving VOE_TEXT_FIELD_SPREAD texels of margin round
// the outline: the field is written outside the shape as well as inside it, and
// a box drawn tight to the outline cuts it off.
void voe_text_raster_field(const voe_text_truetype_outline *outline, float scale,
			   voe_math_float2 origin, uint8_t *field,
			   uint16_t width, uint16_t height,
			   voe_base_arena *arena);
