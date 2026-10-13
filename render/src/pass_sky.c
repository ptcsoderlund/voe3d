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
// A `drawn` sky fills the block's nine harmonics by voe_render_sky_harmonics
// from its backdrop alone, whatever its lights (ADR-0412 point 4).
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
	VOE_BASE_ASSERT(within(sky.horizon_height, 0.0f, 1.0f),
			"a pass whose sky horizon height is past 0 to 1");
	for (uint32_t i = 0; i < VOE_RENDER_DIRECTIONAL_LIGHTS; i++) {
		VOE_BASE_ASSERT(within(sky.disc_size[i], 0.0f, 10.0f),
				"a pass whose sky disc size is past 0 to 10 degrees");
		VOE_BASE_ASSERT(within(sky.glow_size[i], 0.0f, 1.0f),
				"a pass whose sky glow size is past 0 to 1");
	}
	VOE_BASE_ASSERT(within(sky.strength, 0.0f, INFINITY) &&
				within(sky.haze, 0.0f, INFINITY) &&
				within(sky.haze_start, 0.0f, INFINITY),
			"a pass whose sky strength, haze or haze_start is not finite or below nought");
	VOE_BASE_ASSERT(within(exposure.exposure, -16.0f, 16.0f),
			"a pass whose exposure is not finite or past ±16 stops");
	VOE_BASE_ASSERT(exposure.tone <= VOE_RENDER_TONE_SOFT,
			"a pass whose tone is past VOE_RENDER_TONE_SOFT");
	block->sky = sky;
	block->exposure = exposure;
}

// The nine harmonics a rough metal reflects into `block->sky_harmonics`: of
// `camera`'s backdrop when it is `drawn`, no light's glow or disc in them;
// left zero otherwise.
void voe_render_pass_place_sky_light(const voe_render_pass_camera *camera,
				     struct voe_render_frame_block *block)
{
	VOE_BASE_DEBUG_ASSERT(block->sky_harmonics[0].x == 0.0f,
			      "placing the sky's light into a block that already has it");
	if (camera->sky.drawn == 0)
		return;
	voe_render_sky_harmonics(&camera->sky, block->sky_harmonics);
	VOE_BASE_DEBUG_ASSERT(isfinite(block->sky_harmonics[0].x),
			      "the sky's harmonics are not finite");
}
