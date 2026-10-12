// A camera pass's sky and picture, placed into its block: voe_render_pass_begin
// in pass.c calls voe_render_pass_place_sky and then
// voe_render_pass_place_sky_light, in that order, after the block's lights,
// blockers and further lights.
//
// ITS SKY AND ITS EXPOSURE (ADR-0407 point 1) are copied into its block's `sky`
// and `exposure`; a number not finite, a sky field outside its range
// (voe_render_sky in render/device.h), exposure past ±16 or tone past SOFT
// asserts. A pass with no camera, and every shadow, point-shadow and capture
// pass, leaves both zeroed: as authored. A camera pass whose sky is `drawn` has
// it drawn by sky_draw.c as soon as its block is written, before anything else.
// A `drawn` sky whose `fill` is above nought fills the block's nine harmonics by
// voe_render_sky_harmonics from its sun's direction and colour × intensity; an
// unshaded or dark sun leaves them nought.
#include "device_internal.h"
#include "sky.h"

#include <base/assert.h>

#include <math.h>

// Every component of `colour` finite and at least nought.
static bool colour_valid(voe_math_float3 colour)
{
	return isfinite(colour.x) && isfinite(colour.y) && isfinite(colour.z) &&
	       colour.x >= 0.0f && colour.y >= 0.0f && colour.z >= 0.0f;
}

// `value` finite and within [low, high].
static bool within(float value, float low, float high)
{
	return isfinite(value) && value >= low && value <= high;
}

// `camera`'s sky and exposure into `block->sky` and `block->exposure` (ADR-0407
// point 1), asserting each in its range.
void voe_render_pass_place_sky(const voe_render_pass_camera *camera,
			       struct voe_render_frame_block *block)
{
	const voe_render_sky sky = camera->sky;
	const voe_render_exposure exposure = camera->exposure;

	VOE_BASE_ASSERT(colour_valid(sky.zenith) && colour_valid(sky.horizon) &&
				colour_valid(sky.ground),
			"a pass whose sky colour is not finite or below nought");
	VOE_BASE_ASSERT(within(sky.horizon_height, 0.0f, 1.0f) &&
				within(sky.sun_glow, 0.0f, 1.0f) &&
				within(sky.sunset, 0.0f, 1.0f),
			"a pass whose sky horizon height, glow or sunset is past 0 to 1");
	VOE_BASE_ASSERT(within(sky.sun_size, 0.0f, 10.0f),
			"a pass whose sky sun size is past 0 to 10 degrees");
	VOE_BASE_ASSERT(within(sky.haze, 0.0f, INFINITY) &&
				within(sky.fill, 0.0f, INFINITY),
			"a pass whose sky haze or fill is not finite or below nought");
	VOE_BASE_ASSERT(within(exposure.exposure, -16.0f, 16.0f),
			"a pass whose exposure is not finite or past ±16 stops");
	VOE_BASE_ASSERT(exposure.tone <= VOE_RENDER_TONE_SOFT,
			"a pass whose tone is past VOE_RENDER_TONE_SOFT");
	block->sky = sky;
	block->exposure = exposure;
}

// The sky's light into `block->sky_harmonics` (ADR-0405 point 5): the nine of
// `camera`'s sun when its sky is `drawn` with `fill` above nought and the sun
// lights; left zero otherwise, so an unshaded or dark sun, whose direction may
// be nothing, is never projected.
void voe_render_pass_place_sky_light(const voe_render_pass_camera *camera,
				     struct voe_render_frame_block *block)
{
	const voe_render_light sun = camera->light;

	VOE_BASE_DEBUG_ASSERT(block->sky_harmonics[0].x == 0.0f,
			      "placing the sky's light into a block that already has it");
	if (camera->sky.drawn == 0 || !(camera->sky.fill > 0.0f) ||
	    sun.unshaded != 0 || !(sun.intensity > 0.0f))
		return;
	voe_render_sky_harmonics(sun.direction,
				 voe_math_float3_scale(sun.colour,
						       sun.intensity),
				 block->sky_harmonics);
	VOE_BASE_DEBUG_ASSERT(isfinite(block->sky_harmonics[0].x),
			      "the sky's harmonics are not finite");
}
