// The clouds drawn over the world (ADR-0492 point 5):
// voe_render_frame_draw_clouds, declared in render/device.h, marches the open
// camera pass's clouds into its target's half-size clouds image and composites
// them over what the pass holds. 3d's run calls it after the world's blended
// draws and before the overlay's depth clear, once a pass. The shaders are
// draw_clouds.slangh's march and composite behind the sky's vertex stage.
//
// THE SEQUENCE IS half_march.c's: the depth copy if missing, the block split
// round the half-size march, `clouds_image` written, the composite, inside the
// span `clouds`. This file asserts the pass is a camera pass, skips a pass
// whose clouds kind is none, and hands over the clouds' two pipelines.
//
// Constraints: a pass whose clouds kind is none records nothing; no pass, no
// camera, or a shadow, point-shadow or capture pass asserts. The kind is read
// back from the pass's block in mapped, possibly uncached memory: four bytes.
#include "frame_internal.h"

#include <base/assert.h>

bool voe_render_frame_draw_clouds(voe_render_device *device)
{
	const struct voe_render_frame *frame;
	const struct voe_render_frame_block *block;

	VOE_BASE_ASSERT(device != NULL, "drawing clouds on no device");
	VOE_BASE_ASSERT(device->pass_open && device->pass_count > 0,
			"drawing clouds with no pass open");
	VOE_BASE_ASSERT(device->pass_camera,
			"drawing clouds in a pass opened with no camera");
	VOE_BASE_ASSERT(!device->pass_shadow && !device->pass_bounce_shadow &&
				!device->pass_point_shadow && !device->pass_capture,
			"drawing clouds in a shadow, point-shadow or capture pass");

	frame = voe_render_frame_at(device, device->slot);
	// The open pass's block is the one before pass_count.
	block = (const struct voe_render_frame_block *)((const unsigned char *)
								frame->uniforms_mapped +
							(device->pass_count - 1) *
								device->pass_stride);
	if (block->clouds.kind == VOE_RENDER_CLOUDS_NONE)
		return true;
	return voe_render_frame_half_march(device, device->pipeline_clouds_march,
					   device->pipeline_clouds_composite,
					   "clouds");
}
