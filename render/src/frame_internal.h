// The seam between a frame, its passes, its draws and its present: the calls
// frame.c, pass.c, draw.c, present.c and point_shadow.c (whose pass opens through
// pass.c's start) make across one another, and nothing else. Included by those
// five files only; every other file in render reaches a frame through
// device_internal.h, and outside render through device.h.
//
// These were static functions of one file until that file was split by what
// each part does, which is the only reason they carry the voe_render_ prefix
// and external linkage. The why of each is above its definition.
#ifndef VOE_RENDER_FRAME_INTERNAL_H
#define VOE_RENDER_FRAME_INTERNAL_H

#include "device_internal.h"

// frame.c. The frame slot `slot`, asserting it is one: the only place a slot
// indexes the device's frames.
struct voe_render_frame *voe_render_frame_at(voe_render_device *device,
					     uint32_t slot);

// pass.c. Opens a rendering block onto `images` at `extent`, clearing when
// `clear` and loading otherwise; `own` says the colour image is a caller's
// target, which lives in GENERAL.
void voe_render_open_rendering(VkCommandBuffer commands,
			       const struct voe_render_target *images,
			       VkExtent2D extent, bool clear, bool own);

// pass.c. What every pass does once its rendering block is open: `block` into
// the pass's block of the slot's uniform buffer, `pipeline` and the set and the
// static pools bound, the pass counted and open, of no special kind.
void voe_render_pass_start(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   const struct voe_render_frame_block *block,
			   VkPipeline pipeline);

// pass.c. `count` point lights into pass `region`'s records from index `first`.
void voe_render_pass_copy_lights(const struct voe_render_frame *frame,
				 uint32_t region, uint32_t first,
				 const voe_render_point_light *lights,
				 uint32_t count);

// point_shadow.c. The point shadow maps back to where the shader reads them,
// after the point-shadow pass's rendering ends; pass.c's _pass_end calls it.
void voe_render_point_shadow_to_read(const struct voe_render_frame *frame);

// draw.c. Binds the static pools or this slot's transient pair, and records
// which in device->bound_transient.
void voe_render_bind_pools(voe_render_device *device,
			   const struct voe_render_frame *frame, bool transient);

// present.c. Moves the slot's colour image to where a copy can read it.
void voe_render_ready_for_copy(const struct voe_render_frame *frame);

// present.c. Blits the slot's target into swapchain image `image` and leaves
// that image ready to present.
void voe_render_blit_to_screen(voe_render_device *device,
			       const struct voe_render_frame *frame,
			       const struct voe_render_image *image);

#endif
