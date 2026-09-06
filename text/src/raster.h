// An outline, filled into a coverage bitmap. Internal to this folder, and the
// half of glyph rendering that has nothing to do with file formats: it is handed
// points and hands back how much of each pixel the shape covers.
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
// COVERAGE IS ONE BYTE PER PIXEL AND IT IS NOT A COLOUR. 0 is outside the shape
// and 255 is entirely inside it; what the caller does with that is the caller's,
// and text/src/font.c writes it into the alpha channel of a white pixel.
//
// THE ANTI-ALIASING IS SUB-SCANLINES IN Y AND EXACT SPANS IN X. Each row of
// pixels is sampled at VOE_TEXT_RASTER_SAMPLES evenly spaced heights; along each
// of those the spans that are inside the shape are added to the row with their
// fractional ends counted as fractions. So a vertical edge is exact and a
// horizontal one is quantised to one part in VOE_TEXT_RASTER_SAMPLES, which is
// the trade this makes and the reason that number is not smaller.
//
// CURVES ARE FLATTENED TO LINES AND VOE_TEXT_RASTER_TOLERANCE IS HOW STRAIGHT
// THAT HAS TO BE. `glyf` outlines are quadratic Béziers; each is split into as
// many equal pieces as it takes for the chord to sit within the tolerance of the
// curve, in the pixels the caller is rasterising into rather than in font units,
// so a big glyph gets more pieces than a small one for free.
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
#define VOE_TEXT_RASTER_SAMPLES 8

// How far a flattened curve may sit from the curve it replaces, in pixels. A
// tenth of a pixel is below what the coverage above can represent, so the
// flattening is not what limits the quality.
#define VOE_TEXT_RASTER_TOLERANCE 0.1f

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
void voe_text_raster_fill(const voe_text_truetype_outline *outline, float scale,
			  voe_math_float2 origin, uint8_t *coverage,
			  uint16_t width, uint16_t height,
			  voe_base_arena *arena);
