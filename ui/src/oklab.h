// OKLab, internal to this folder. theme.c is the only caller: the palette in
// ui/theme.h is derived by stepping LIGHTNESS in this space, because a step
// sized in sRGB or linear RGB is a different size per hue and the palette
// looks derived (ADR-0087, kept by ADR-0171). Björn Ottosson's published
// matrices, unchanged — this is a well-known, checkable conversion and not
// this folder's own invention.
//
// IT LIVES HERE AND NOT IN `math` (ADR-0170, answering D-131). `math` is
// spelled the way Slang spells it and OKLab is not a Slang concept; nothing
// outside `ui` has asked for one (rule 10). Move it to `math` the day a second
// folder needs it, unchanged.
//
// L, a AND b ARE NOT x, y AND z. This is deliberately its own three-float
// struct rather than a reused voe_math_float3: a lightness, a green-red axis
// and a blue-yellow axis are not a position or a direction, and giving them
// vector operations would invite treating a hue rotation as if it were a
// cross product.
//
// CONVERSION IS LOSSY AT THE EDGES OF THE GAMUT, ON PURPOSE. Not every (L, a,
// b) triple is a real colour a monitor can show; voe_ui_oklab_to_linear and
// voe_ui_oklab_to_srgb both clamp their result into 0..1 per channel rather
// than handing back a negative or over-range number, which is what lets
// theme.c push a text lightness all the way to an extreme without checking
// first. A round trip through srgb -> oklab -> srgb is bit-close for any
// colour that started in gamut, which is what ui/tests/theme.c pins down; a
// round trip is not expected to hold near the gamut's edge, where clamping is
// exactly what changes the answer.
#pragma once

#include <math/float3.h>

// A colour in OKLab. `l` is lightness, roughly 0 (black) to 1 (white); `a` and
// `b` are the two chroma axes and are unbounded in principle, though every
// real colour keeps chroma — sqrt(a*a + b*b) — small next to 1.
typedef struct {
	float l, a, b;
} voe_ui_oklab;

// `srgb` is 0..1 per channel, sRGB-encoded, exactly as a person picks it and
// as voe_ui_theme_inputs.accent is authored. Never fails: every sRGB triple in
// range is a real colour, so there is nothing here to clamp on the way in.
voe_ui_oklab voe_ui_oklab_from_srgb(voe_math_float3 srgb);

// The same conversion, skipping the sRGB transfer function because `linear`
// already is one. theme.c never calls this — every colour it starts from is
// authored sRGB — and it exists so ui/tests/theme.c can read the chroma back
// out of a role voe_ui_theme_derive already converted to linear, which is the
// only way to check the dark/light chroma clamp against what the derivation
// actually produced.
voe_ui_oklab voe_ui_oklab_from_linear(voe_math_float3 linear);

// Linear RGB, clamped to 0..1 per channel — ready for a linear element record
// with no sRGB step left to apply. This is what theme.c calls for every role:
// the palette never goes back through the sRGB curve, because nothing
// downstream of it reads sRGB (ADR-0069).
voe_math_float3 voe_ui_oklab_to_linear(voe_ui_oklab lab);

// The same conversion, finished with the sRGB transfer function, for a round
// trip against an authored colour. Nothing in theme.c calls this — its palette
// is linear all the way — and it exists for ui/tests/theme.c to check the
// conversion against a value a person can read off a colour picker.
voe_math_float3 voe_ui_oklab_to_srgb(voe_ui_oklab lab);
