// The fog drawn over the world after the clouds (ADR-0493 point 5):
// voe_render_frame_draw_fog, declared in render/device.h, marches the open
// camera pass's fog volumes into its target's half-size clouds image and
// composites them over what the pass holds. 3d's run calls it right after
// voe_render_frame_draw_clouds and before the overlay's depth clear, once a
// pass. The shaders are draw_fog.slangh's march and composite behind the sky's
// vertex stage.
//
// THE SEQUENCE IS half_march.c's, the clouds' own: the depth copy if missing,
// the block split round the half-size march, `clouds_image` written, the
// composite, inside the span `fog`. The clouds image is free once the clouds
// are composited, so the fog reuses it rather than keeping one of its own.
// This file asserts the pass is a camera pass, skips a pass with no volumes,
// and hands over the fog's two pipelines.
//
// Constraints: a pass whose fog count is 0 records nothing; no pass, no
// camera, or a shadow, point-shadow or capture pass asserts. The count is read
// back from the pass's block in mapped, possibly uncached memory: four bytes.
#include "frame_internal.h"

#include <base/assert.h>

bool voe_render_frame_draw_fog(voe_render_device *device)
{
	const struct voe_render_frame *frame;
	const struct voe_render_frame_block *block;

	VOE_BASE_ASSERT(device != NULL, "drawing fog on no device");
	VOE_BASE_ASSERT(device->pass_open && device->pass_count > 0,
			"drawing fog with no pass open");
	VOE_BASE_ASSERT(device->pass_camera,
			"drawing fog in a pass opened with no camera");
	VOE_BASE_ASSERT(!device->pass_shadow && !device->pass_bounce_shadow &&
				!device->pass_point_shadow && !device->pass_capture,
			"drawing fog in a shadow, point-shadow or capture pass");

	frame = voe_render_frame_at(device, device->slot);
	// The open pass's block is the one before pass_count.
	block = (const struct voe_render_frame_block *)((const unsigned char *)
								frame->uniforms_mapped +
							(device->pass_count - 1) *
								device->pass_stride);
	if (block->fog.count == 0)
		return true;
	return voe_render_frame_half_march(device, device->pipeline_fog_march,
					   device->pipeline_fog_composite,
					   "fog");
}
