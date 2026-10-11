// The sky drawn behind everything (ADR-0405 point 4): voe_render_sky_draw,
// declared in device_calls.h, records one full-screen triangle through the sky
// pipeline (pipeline.c) as a camera pass opens. pass.c calls it right after
// the pass's block is written and before anything else is recorded, so every
// later draw, the blended ones included, lands over it. The shader is
// draw.slang's voe_render_sky_vertex and voe_render_sky_fragment.
//
// IT IS THE SPAN `sky`, so the breakdown lists `<pass>: sky` after the pass.
//
// IT PUSHES A ZEROED PUSH CONSTANT. The fragment exposes through tone.slangh,
// which reads `plain` from it; nought is a sky exposed and toned as the pass
// says, and nothing earlier in the command buffer need have pushed at all.
//
// IT LEAVES `bound` AS THE SKY PIPELINE, which no mesh draw uses, so the pass's
// first mesh draw binds its own (draw.c). It is not counted in
// voe_render_frame_draw_count, which counts the caller's draws.
//
// Constraints: `drawn` is read back from the pass's block in the mapped uniform
// buffer, four bytes from host-visible memory that may be uncached; the block
// is the one record of the pass's camera this side keeps.
#include "device_internal.h"

#include <base/assert.h>

#include <stddef.h>
#include <stdint.h>

void voe_render_sky_draw(voe_render_device *device)
{
	struct voe_render_frame *frame;
	const struct voe_render_frame_block *block;
	const uint32_t push[5] = { 0 };

	VOE_BASE_ASSERT(device != NULL, "drawing the sky on no device");
	VOE_BASE_ASSERT(device->pass_open && device->pass_count > 0,
			"drawing the sky with no pass open");
	if (!device->pass_camera || device->pass_shadow ||
	    device->pass_point_shadow || device->pass_capture ||
	    device->pass_bounce_shadow)
		return;
	frame = voe_render_frame_open(device);
	block = (const struct voe_render_frame_block *)((const unsigned char *)
								frame->uniforms_mapped +
							(device->pass_count - 1) *
								device->pass_stride);
	if (block->sky.drawn == 0)
		return;
	VOE_BASE_DEBUG_ASSERT(device->pipeline_sky != VK_NULL_HANDLE,
			      "drawing the sky on a device not prepared");

	voe_render_frame_span_begin(device, "sky");
	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					device->pipeline_sky);
	device->bound = device->pipeline_sky;
	voe_render_vk.cmd_push_constants(frame->commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(push), push);
	voe_render_vk.cmd_draw(frame->commands, 3, 1, 0, 0);
	voe_render_frame_span_end(device);
	VOE_BASE_DEBUG_ASSERT(device->bound == device->pipeline_sky,
			      "the sky left another pipeline bound");
}
