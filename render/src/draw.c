// The draws inside an open pass: voe_render_frame_draw and _draw_blended record
// one mesh each, _clear_depth clears depth between two groups of them, and
// _draw_count reports how many draw commands the frame held. Every call asserts
// a pass is open, and a mesh draw that the pass was opened with a camera.
//
// ONE OBJECT RECORD PER DRAW. A draw writes its voe_render_object into the
// slot's object buffer at device->object_count and pushes that number as the
// only push constant; the shader reads its own record with it. The buffer holds
// capacities.objects records, and a draw past that is a returned failure.
//
// WHICH PIPELINE IS BOUND, AND WHICH POOLS. A pass opens with the solid
// pipeline and the static pools bound (pass.c). A blended draw needs the other
// pipeline and a transient mesh the other pool pair; draw_with rebinds either
// only when it differs from what device->bound and device->bound_transient say
// was bound last, so a run of draws of one kind costs one bind and a caller that
// interleaves them is still drawn right. In a shadow pass (pass.c) every mesh draw
// goes through the shadow pipeline, in a bounce pass through the bounce one; a
// blended draw and the depth clear assert in either. The element pipeline (element.c) is
// never bound here; it leaves `bound` different, and the next mesh rebinds.
//
// THIS FILE DOES NOT SORT. The order the blended draws arrive in is the order
// they are recorded in, and getting it right is voe_3d_draw_system_run's.
#include "frame_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// The vertex buffer and the index buffer a draw reads: the device's static pair
// or this slot's transient pair. One function for both so that the rendering's
// opening bind and draw_with's rebind cannot bind them differently, and so that
// `bound_transient` is set in the one place the binding happens.
//
// TWO PAIRS OF GEOMETRY POOLS CAN BE DRAWN FROM AND ONLY ONE PAIR IS BOUND AT A
// TIME. The static pair is bound as a pass opens, because that is what a pass's
// first draws come out of; a range in this slot's transient pair (card
// 028) needs the other pair bound, and draw_with rebinds when — and only when —
// the pool a range is in differs from the pair last bound. It is tracked exactly
// as the pipeline is: a run of draws out of one pair costs one bind, and neither
// pair is ever bound per draw. Forgetting the rebind draws one object wearing
// another's shape out of the wrong buffer, which is the failure to look for.
void voe_render_bind_pools(voe_render_device *device,
			   const struct voe_render_frame *frame, bool transient)
{
	VkDeviceSize vertex_offset = 0;
	VkBuffer vertices = transient ?
				    frame->transient_vertices.pool.buffer.buffer :
				    device->vertices.buffer.buffer;
	VkBuffer indices = transient ?
				   frame->transient_indices.pool.buffer.buffer :
				   device->indices.buffer.buffer;

	voe_render_vk.cmd_bind_vertex_buffers(frame->commands, 0, 1, &vertices,
					      &vertex_offset);
	voe_render_vk.cmd_bind_index_buffer(frame->commands, indices, 0,
					    VK_INDEX_TYPE_UINT32);
	device->bound_transient = transient;
}

// Whether shading record `shading` names texture slot `texture` in any of its
// five texture slots. For the self-sampling assert in draw_with and nothing else,
// which is why it is compiled into a release build only as an operand of sizeof.
//
// IT READS THE RECORD BACK OUT OF THE MAPPED BUFFER. That buffer is host-visible
// and coherent, and nothing in a debug build is a reason to keep a second copy
// of every record on this side. An index past the buffer names no record and so
// names no texture; the shader clamps nothing, but that is a different mistake
// and not this assert's.
[[maybe_unused]] static bool shading_names(const voe_render_device *device,
					   uint32_t shading, uint32_t texture)
{
	voe_render_shading_values values;

	if (shading >= device->capacities.shadings)
		return false;

	memcpy(&values,
	       (const unsigned char *)device->shadings_mapped +
		       (size_t)shading * sizeof(values),
	       sizeof(values));
	return values.base_colour_texture == texture ||
	       values.metallic_roughness_texture == texture ||
	       values.normal_texture == texture ||
	       values.occlusion_texture == texture ||
	       values.emissive_texture == texture;
}

// The whole of both draw calls; `pipeline` is the only thing that differs
// between them.
//
// THE BIND IS CONDITIONAL AND THAT IS THE ONLY REASON `bound` EXISTS. A frame is
// a run of solid draws and then a run of blended ones, so this costs one bind at
// the boundary rather than one per draw — and it is still correct if a caller
// ever interleaves them, which is what makes it a condition and not an
// assumption about the caller's order.
static bool draw_with(voe_render_device *device, voe_render_geometry geometry,
		      voe_render_object object, VkPipeline pipeline)
{
	const struct voe_render_geometry_slot *slot;
	struct voe_render_frame *frame;

	VOE_BASE_ASSERT(device != NULL, "drawing on no device");
	VOE_BASE_ASSERT(device->pass_open,
			"drawing with no pass open — every draw is inside a voe_render_pass_begin and its _pass_end");
	VOE_BASE_ASSERT(device->pass_camera,
			"drawing a mesh in a pass opened with no camera — only elements may be drawn in one");
	VOE_BASE_DEBUG_ASSERT(device->pass_target == NULL ||
				      !shading_names(device, object.shading,
						     device->pass_target->texture),
			      "drawing a mesh that shows the target this pass draws into — a picture may not read itself while it is written");

	slot = voe_render_geometry_at(device, geometry);
	if (slot == NULL) {
		VOE_BASE_ERROR("render",
			       "a draw named mesh %u generation %u, which is not a mesh this device handed out",
			       geometry.index, geometry.generation);
		return false;
	}

	if (device->object_count >= device->capacities.objects) {
		VOE_BASE_ERROR("render",
			       "this frame already holds %u objects, which is what the device was made for",
			       device->capacities.objects);
		return false;
	}

	frame = voe_render_frame_at(device, device->slot);
	// A shadow pass draws depth alone, whichever draw call asked.
	if (device->pass_shadow)
		pipeline = device->pipeline_shadow;
	else if (device->pass_bounce)
		pipeline = device->pipeline_bounce;

	if (device->bound != pipeline) {
		voe_render_vk.cmd_bind_pipeline(frame->commands,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipeline);
		device->bound = pipeline;
	}

	// Which pool pair the range is in, and a rebind only when that changes
	// — tracked exactly as the pipeline is above, and for the same reason.
	if (device->bound_transient != slot->transient)
		voe_render_bind_pools(device, frame, slot->transient);

	// The record, into this slot's own object buffer at this object's
	// number. Written rather than staged because the buffer is host-visible
	// and this slot's; the fence at the top of the frame is what makes that
	// safe.
	memcpy((unsigned char *)frame->objects_mapped +
		       (size_t)device->object_count * sizeof(object),
	       &object, sizeof(object));

	// The object's number, and the only push constant left in this engine.
	// The shader reads its own record out of the buffer with it — which is
	// also why this stops being a push constant the day the draws become
	// indirect: an indirect draw's shader reads the same number out of its
	// instance index instead.
	voe_render_vk.cmd_push_constants(frame->commands, device->layout,
					 VK_SHADER_STAGE_VERTEX_BIT |
						 VK_SHADER_STAGE_FRAGMENT_BIT,
					 0, sizeof(device->object_count),
					 &device->object_count);

	// first_vertex is the vertexOffset rather than something added to the
	// indices on the way in, which is what lets a mesh keep the numbering
	// its file gave it.
	voe_render_vk.cmd_draw_indexed(frame->commands, slot->index_count, 1,
				       slot->first_index,
				       (int32_t)slot->first_vertex, 0);

	device->object_count++;
	// One mesh, one draw command, which is the thing the element path exists
	// not to do. Counted here and in the element draw and nowhere else — see
	// voe_render_frame_draw_count.
	device->draw_commands++;
	return true;
}

// THERE ARE TWO DRAW CALLS AND THEY DIFFER IN ONE ARGUMENT. _draw goes through
// the solid pipeline, _draw_blended through the one that tests depth without
// writing it and blends premultiplied; both are draw_with() above. This file
// does not sort and does not know how to: the order the blended draws arrive in
// is the order they are recorded in, and getting that order right is
// voe_3d_draw_system_run's.
bool voe_render_frame_draw(voe_render_device *device,
			   voe_render_geometry geometry,
			   voe_render_object object)
{
	VOE_BASE_ASSERT(device != NULL, "drawing on no device");
	return draw_with(device, geometry, object, device->pipeline);
}

bool voe_render_frame_draw_blended(voe_render_device *device,
				   voe_render_geometry geometry,
				   voe_render_object object)
{
	VOE_BASE_ASSERT(device != NULL, "drawing on no device");
	VOE_BASE_ASSERT(!device->pass_shadow,
			"drawing a blended mesh in a shadow pass — nothing see-through casts");
	VOE_BASE_ASSERT(!device->pass_bounce,
			"drawing a blended mesh in a bounce pass — nothing see-through bounces");
	return draw_with(device, geometry, object, device->pipeline_blended);
}

// The overlay's depth clear: one command into the rendering block that is
// already open, and the whole of what card 024 needed from this folder.
//
// vkCmdClearAttachments AND NOT A SECOND RENDERING BLOCK. Ending the rendering
// and beginning it again would work and would cost a second set of load and
// store operations, a second transition of both images, and a colour attachment
// that has to be reloaded rather than kept — for a clear that this one command
// performs inside the block the pass already has open. One rendering block per
// pass is the shape, and this does not change it.
//
// IT IS ORDERED AGAINST THE DRAWS AROUND IT AND THAT IS WHY THIS WORKS AT ALL. A
// clear inside a rendering block executes in command order like a draw, so
// everything recorded before this sees the depth buffer it wrote and everything
// recorded after it sees an empty one. It is not a load operation and does not
// happen at the top of the pass.
//
// NO PIPELINE STATE REACHES IT. It clears the attachment directly rather than
// through a pipeline, so the blended pipeline's depth write being off does not
// hold it back — which is what lets it be called after a run of blended draws.
//
// THE DEPTH ASPECT ONLY, AND THE SAME VALUE THE LOAD OP USES. Naming the colour
// attachment here would throw away the world's picture, which is the one way
// this call can be badly wrong, and VOE_RENDER_DEPTH_CLEAR is read from the same
// constant voe_render_open_rendering (pass.c) reads so the two cannot drift
// apart.
//
// THAT ONE IS voe_render_frame_clear_depth. Card 024 added it: a caller that
// wants a group of objects to be in front of everything it has already drawn
// clears depth between the two, inside the pass's rendering block. It is the only
// command recorded between draws that is not itself a draw, and its own comment
// says why it is not a second rendering block.
void voe_render_frame_clear_depth(voe_render_device *device)
{
	VkClearAttachment attachment = {
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	// The whole target, which is the rect the scissor is already set to.
	// Framebuffer coordinates, so the viewport's Y flip takes no part in it.
	// Its extent is filled in below rather than here, because reading it is
	// already a dereference and the assert has not run yet.
	VkClearRect rect = { .baseArrayLayer = 0, .layerCount = 1 };

	VOE_BASE_ASSERT(device != NULL, "clearing depth on no device");
	VOE_BASE_ASSERT(device->pass_open,
			"clearing depth with no pass open — the clear is recorded into a pass's rendering block");
	VOE_BASE_ASSERT(device->pass_camera,
			"clearing depth in a pass opened with no camera — nothing in such a pass writes depth to clear");
	VOE_BASE_ASSERT(!device->pass_shadow,
			"clearing depth in a shadow pass — its depth is the map being drawn");
	VOE_BASE_ASSERT(!device->pass_bounce,
			"clearing depth in a bounce pass — its depth is the map being drawn");

	rect.rect.extent = device->pass_extent;

	voe_render_vk.cmd_clear_attachments(voe_render_frame_at(device, device->slot)->commands,
					    1, &attachment, 1, &rect);
}

uint32_t voe_render_frame_draw_count(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device how many draws it made");

	// No assert on `recording`: the number is worth reading after _end, and
	// that is where a caller reporting what a frame cost will read it.
	return device->draw_commands;
}
