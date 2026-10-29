// A camera pass's sky, picture, clouds and fog, placed into its block:
// voe_render_pass_begin in pass.c calls voe_render_pass_place_sky and then
// voe_render_pass_place_sky_light, in that order, after the block's lights,
// blockers and further lights.
//
// ITS SKY, EXPOSURE, CLOUDS AND FOG (ADR-0407 point 1, 0492 point 3, 0493 point
// 3) are copied into its block's `sky`, `exposure`, `clouds` and `fog`, and
// `clouds_image` is written VOE_RENDER_NO_CLOUDS_IMAGE; a number not finite, a
// sky, clouds or fog field outside its range (voe_render_sky, voe_render_clouds
// and voe_render_fog_volume in render/device.h), a fog count past
// VOE_RENDER_FOG_VOLUMES, exposure past ±16 or tone past SOFT asserts. A pass
// with no camera, and every shadow, point-shadow and capture pass, leaves all
// four zeroed: as authored. A camera
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

// Every component of `place` within [0, period].
static bool place_within(voe_math_float3 place, float period)
{
	return within(place.x, 0.0f, period) && within(place.y, 0.0f, period) &&
	       within(place.z, 0.0f, period);
}

// `camera`'s fog into `block->fog` (ADR-0493 point 3), whole, asserting the
// count and each counted volume's fields in their ranges.
static void place_fog(const voe_render_pass_camera *camera,
		      struct voe_render_frame_block *block)
{
	const voe_render_fog *fog = &camera->fog;

	VOE_BASE_ASSERT(fog->count <= VOE_RENDER_FOG_VOLUMES,
			"a pass whose fog count is past VOE_RENDER_FOG_VOLUMES");
	for (uint32_t i = 0; i < fog->count && i < VOE_RENDER_FOG_VOLUMES; i++) {
		const voe_render_fog_volume *volume = &fog->volumes[i];

		for (uint32_t r = 0; r < 3; r++)
			VOE_BASE_ASSERT(isfinite(volume->rows[r].x) &&
						isfinite(volume->rows[r].y) &&
						isfinite(volume->rows[r].z) &&
						isfinite(volume->rows[r].w),
					"a pass whose fog volume row is not finite");
		VOE_BASE_ASSERT(within(volume->density, 0.0f, INFINITY),
				"a pass whose fog density is not finite or below nought");
		VOE_BASE_ASSERT(within(volume->patchiness, 0.0f, 1.0f) &&
					within(volume->godrays, 0.0f, 1.0f),
				"a pass whose fog patchiness or godrays is past 0 to 1");
		VOE_BASE_ASSERT(place_within(volume->patches, VOE_RENDER_FOG_PATCHES),
				"a pass whose fog patches are past 0 to VOE_RENDER_FOG_PATCHES");
		VOE_BASE_ASSERT(place_within(volume->change, VOE_RENDER_FOG_CHANGE),
				"a pass whose fog change is past 0 to VOE_RENDER_FOG_CHANGE");
	}
	block->fog = *fog;
	VOE_BASE_DEBUG_ASSERT(block->fog.count == fog->count,
			      "the fog was not copied whole");
}

// `camera`'s sky, exposure, clouds and fog into `block->sky`, `block->exposure`,
// `block->clouds` and `block->fog` (ADR-0407 point 1, 0492 point 3, 0493 point
// 3), asserting each in its range.
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
	place_fog(camera, block);
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
