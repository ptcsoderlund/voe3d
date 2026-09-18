// The OKLab conversion. See oklab.h for what it is for and why it lives here.
#include "oklab.h"

#include <math.h>

static float clamp01(float v)
{
	if (v < 0.0f)
		return 0.0f;
	if (v > 1.0f)
		return 1.0f;
	return v;
}

static float srgb_to_linear(float c)
{
	if (c <= 0.04045f)
		return c / 12.92f;
	return powf((c + 0.055f) / 1.055f, 2.4f);
}

static float linear_to_srgb(float c)
{
	c = clamp01(c);
	if (c <= 0.0031308f)
		return c * 12.92f;
	return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

// The shared half of both public "into OKLab" conversions, so the matrices
// below appear exactly once regardless of whether the input started sRGB or
// linear.
static voe_ui_oklab linear_to_oklab(voe_math_float3 linear)
{
	float r = linear.x, g = linear.y, b = linear.z;

	float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
	float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
	float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

	float l_ = cbrtf(l);
	float m_ = cbrtf(m);
	float s_ = cbrtf(s);

	return (voe_ui_oklab){
		.l = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
		.a = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
		.b = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
	};
}

voe_ui_oklab voe_ui_oklab_from_srgb(voe_math_float3 srgb)
{
	return linear_to_oklab((voe_math_float3){
		srgb_to_linear(srgb.x),
		srgb_to_linear(srgb.y),
		srgb_to_linear(srgb.z),
	});
}

voe_ui_oklab voe_ui_oklab_from_linear(voe_math_float3 linear)
{
	return linear_to_oklab(linear);
}

// The shared half of both public conversions: OKLab to linear RGB, clamped.
// Kept as one function so the two callers below can never compute it two
// different ways.
voe_math_float3 voe_ui_oklab_to_linear(voe_ui_oklab lab)
{
	float l_ = lab.l + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
	float m_ = lab.l - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
	float s_ = lab.l - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

	float l = l_ * l_ * l_;
	float m = m_ * m_ * m_;
	float s = s_ * s_ * s_;

	float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
	float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
	float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

	return (voe_math_float3){ clamp01(r), clamp01(g), clamp01(b) };
}

voe_math_float3 voe_ui_oklab_to_srgb(voe_ui_oklab lab)
{
	voe_math_float3 linear = voe_ui_oklab_to_linear(lab);

	return (voe_math_float3){
		linear_to_srgb(linear.x),
		linear_to_srgb(linear.y),
		linear_to_srgb(linear.z),
	};
}
