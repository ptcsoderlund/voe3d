// A decoded picture halved until it is small enough to be worth uploading.
// `dev`'s own, deliberately: it is a property of the two pictures this program
// happens to embed and NOT a rule about textures, and nothing in the engine
// grew one because of it.
//
// ---- IT IS NOT MIPMAPPING, AND THE DIFFERENCE MATTERS ----
//
// THIS ENGINE HAS NO MIPMAPS AND THAT IS A DECISION, not a gap: every texture
// is one level, sampled NEAREST, magnified and minified, and render/src/texture.c
// says so at the top and says what it costs. Nothing here changes that. What
// this file does is pick a BETTER SINGLE LEVEL than the file happened to ship,
// which is the only lever a caller has when there is one level to pick.
//
// The construction is a mip chain's — each step is an exact 2x2 box average of
// the step before — and it simply stops at the first level that fits instead of
// keeping every level and letting the sampler choose. So it is one level off a
// mip chain, computed once at startup, and the sampler is none the wiser.
//
// ---- WHY BOTHER, IN NUMBERS ----
//
// The wordmark ships at 4800 by 2000 and lands on a cube face a few hundred
// pixels across. That is minification of better than ten to one, and with
// NEAREST and no mip chain it means most of the picture is never sampled and
// what IS sampled changes as the camera moves: the caption under the wordmark
// came out as a moving smear of dots rather than as text. Halved twice to
// 1200 by 500 it is minified about three to one, and the caption reads.
//
// IT REDUCES THE ALIASING AND DOES NOT REMOVE IT. Anything minified past about
// one texel per pixel still aliases and still shimmers as the camera moves —
// removing that is what a mip chain is for and this engine has decided against
// one. Do not read a quieter picture as a fixed one.
//
// And it is cheaper three times over: the decoded picture is 38 megabytes of
// the frame arena at 4800 by 2000 and under 3 at 1200 by 500, the upload is the
// same ratio, and so is what the card holds afterwards.
//
// ---- WHAT IT DOES NOT DO ----
//
// It does not touch the files. logo.png and app icon light.png are whatever
// they are and stay that way; this runs on the decoded pixels, at startup, on
// the way to voe_render_texture_create. Re-encoding somebody's artwork to save
// a megabyte is not this program's business.
#pragma once

#include <assets/image.h>

#include <stdint.h>

// How long the longest side may be before this halves the picture again. 1920,
// because that is a size a person recognises and because it leaves the two
// pictures readable close up and at a window far bigger than this program is
// usually run in.
//
// IT IS DELIBERATELY MORE THAN A CUBE FACE IS WORTH. What is actually on screen
// is a few hundred pixels, so the honest number is much smaller — but the cubes
// can be flown right up to, the window can be a 4K one, and a picture that is
// too small is a blurry mess that cannot be fixed by moving closer while one
// that is too big only costs memory.
#define VOE_DEV_SHRINK_LONG_SIDE 1920

// Halves `image` — in place, in its own buffer — until neither side is longer
// than `long_side`, and leaves it alone if it already fits. `long_side` must be
// greater than nought.
//
// AN ODD SIDE LOSES NOTHING: the last row or column of an odd-sized picture is
// averaged with itself rather than dropped, so the edge of the picture is still
// the edge of the picture. Neither of the two pictures here is odd at any step
// — 4800 by 2000 and 2048 square both halve cleanly — but a picture somebody
// drops in later may be, and silently cropping it would be a strange thing to
// debug.
void voe_dev_image_shrink(voe_assets_image *image, uint32_t long_side);
