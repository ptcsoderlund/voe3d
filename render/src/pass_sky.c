// A camera pass's sky, placed into its block: voe_render_pass_begin in pass.c
// calls voe_render_pass_place_sky and then voe_render_pass_place_sky_light, in
// that order, after the block's lights, blockers and further lights.
//
// ITS SKY (ADR-0405 point 2) is copied into its block's `sky`; a number not
// finite, light or haze below nought, exposure past ±16 or tone past SOFT
// asserts. A pass with no camera, and every shadow, point-shadow and capture
// pass, leaves it zeroed: as authored. A camera pass whose sky is `drawn` has
// it drawn by sky_draw.c as soon as its block is written, before anything else.
// Its sky `light` above nought fills the block's nine harmonics by
// voe_render_sky_harmonics from its sun's direction and colour × intensity; an
// unshaded or dark sun leaves them nought.
#include "device_internal.h"
#include "sky.h"

#include <base/assert.h>

#include <math.h>

// `camera`'s sky into `block->sky` (ADR-0405 point 2), asserting every number
// finite, light and haze at least nought, exposure within ±16 and tone a
// VOE_RENDER_TONE_*.
void voe_render_pass_place_sky(const voe_render_pass_camera *camera,
			       struct voe_render_frame_block *block)
{
	const voe_render_sky sky = camera->sky;

	VOE_BASE_ASSERT(isfinite(sky.light) && isfinite(sky.haze) &&
				isfinite(sky.exposure),
			"a pass whose sky holds a number that is not finite");
	VOE_BASE_ASSERT(sky.light >= 0.0f && sky.haze >= 0.0f,
			"a pass whose sky light or haze is below nought");
	VOE_BASE_ASSERT(sky.exposure >= -16.0f && sky.exposure <= 16.0f,
			"a pass whose sky exposure is past ±16 stops");
	VOE_BASE_ASSERT(sky.tone <= VOE_RENDER_TONE_SOFT,
			"a pass whose sky tone is past VOE_RENDER_TONE_SOFT");
	block->sky = sky;
}

// The sky's light into `block->sky_harmonics` (ADR-0405 point 5): the nine of
// `camera`'s sun when its sky `light` is above nought and the sun lights; left
// zero otherwise, so an unshaded or dark sun, whose direction may be nothing,
// is never projected.
void voe_render_pass_place_sky_light(const voe_render_pass_camera *camera,
				     struct voe_render_frame_block *block)
{
	const voe_render_light sun = camera->light;

	VOE_BASE_DEBUG_ASSERT(block->sky_harmonics[0].x == 0.0f,
			      "placing the sky's light into a block that already has it");
	if (!(camera->sky.light > 0.0f) || sun.unshaded != 0 ||
	    !(sun.intensity > 0.0f))
		return;
	voe_render_sky_harmonics(sun.direction,
				 voe_math_float3_scale(sun.colour,
						       sun.intensity),
				 block->sky_harmonics);
	VOE_BASE_DEBUG_ASSERT(isfinite(block->sky_harmonics[0].x),
			      "the sky's harmonics are not finite");
}
