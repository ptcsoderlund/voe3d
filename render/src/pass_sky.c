// A camera pass's sky, picture and clouds, placed into its block:
// voe_render_pass_begin in pass.c calls voe_render_pass_place_sky and then
// voe_render_pass_place_sky_light, in that order, after the block's lights,
// blockers and further lights.
//
// ITS SKY, EXPOSURE AND CLOUDS (ADR-0407 point 1, 0492 point 3) are copied into
// its block's `sky`, `exposure` and `clouds`, and `clouds_image` is written
// VOE_RENDER_NO_CLOUDS_IMAGE; a number not finite, a sky or clouds field outside
// its range (voe_render_sky and voe_render_clouds in render/device.h), exposure
// past ±16 or tone past SOFT asserts. A pass with no camera, and every shadow,
// point-shadow and capture pass, leaves all three zeroed: as authored. A camera
// pass whose sky is `drawn` has
// it drawn by sky_draw.c as soon as its block is written, before anything else.
// A `drawn` sky fills the block's nine harmonics by voe_render_sky_harmonics
// from its backdrop alone, whatever its lights (ADR-0412 point 4), at strength
// 1 whatever the row's (ADR-0424 point 4).
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

// `camera`'s clouds into `block->clouds` (ADR-0492 point 3), asserting each
// field in its range, and no clouds image yet.
static void place_clouds(const voe_render_pass_camera *camera,
			 struct voe_render_frame_block *block)
{
	const voe_render_clouds clouds = camera->clouds;

	VOE_BASE_ASSERT(clouds.kind <= VOE_RENDER_CLOUDS_VOLUMETRIC,
			"a pass whose clouds kind is past VOE_RENDER_CLOUDS_VOLUMETRIC");
	VOE_BASE_ASSERT(within(clouds.coverage, 0.0f, 1.0f) &&
				within(clouds.shadow_strength, 0.0f, 1.0f),
			"a pass whose clouds coverage or shadow strength is past 0 to 1");
	VOE_BASE_ASSERT(within(clouds.thickness, 0.0f, INFINITY),
			"a pass whose clouds thickness is not finite or below nought");
	VOE_BASE_ASSERT(isfinite(clouds.bottom), "a pass whose clouds bottom is not finite");
	VOE_BASE_ASSERT(within(clouds.origin.x, 0.0f, VOE_RENDER_CLOUDS_PERIOD) &&
				within(clouds.origin.y, 0.0f, VOE_RENDER_CLOUDS_PERIOD),
			"a pass whose clouds origin is past 0 to VOE_RENDER_CLOUDS_PERIOD");
	block->clouds = clouds;
	block->clouds_image = VOE_RENDER_NO_CLOUDS_IMAGE;
}

// `camera`'s sky, exposure and clouds into `block->sky`, `block->exposure` and
// `block->clouds` (ADR-0407 point 1, 0492 point 3), asserting each in its range.
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
	VOE_BASE_ASSERT(isfinite(sky.haze_top), "a pass whose sky haze_top is not finite");
	VOE_BASE_ASSERT(within(exposure.exposure, -16.0f, 16.0f),
			"a pass whose exposure is not finite or past ±16 stops");
	VOE_BASE_ASSERT(exposure.tone <= VOE_RENDER_TONE_SOFT,
			"a pass whose tone is past VOE_RENDER_TONE_SOFT");
	block->sky = sky;
	block->exposure = exposure;
	place_clouds(camera, block);
}

// The nine harmonics a rough metal reflects into `block->sky_harmonics`: of
// `camera`'s backdrop at strength 1 when it is `drawn`, so the row's strength
// changes no surface, no light's glow or disc in them; left zero otherwise.
void voe_render_pass_place_sky_light(const voe_render_pass_camera *camera,
				     struct voe_render_frame_block *block)
{
	voe_render_sky mirrored = camera->sky;

	VOE_BASE_DEBUG_ASSERT(block->sky_harmonics[0].x == 0.0f,
			      "placing the sky's light into a block that already has it");
	if (mirrored.drawn == 0)
		return;
	mirrored.strength = 1.0f;
	voe_render_sky_harmonics(&mirrored, block->sky_harmonics);
	VOE_BASE_DEBUG_ASSERT(isfinite(block->sky_harmonics[0].x),
			      "the sky's harmonics are not finite");
}
