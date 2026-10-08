// A pass: one rendering block onto a target, the window's or one of the
// caller's own, with its camera block written into the slot's uniform buffer.
// voe_render_pass_begin opens it inside an open frame, the draws in draw.c and
// element.c record into it, and voe_render_pass_end closes it; passes do not
// nest, and a frame may open up to capacities.passes of them.
//
// THE FIRST PASS ONTO A TARGET IN A FRAME CLEARS, EVERY LATER ONE LOADS. The
// clears are not draws — they are the load operations dynamic rendering performs
// as a block begins — so what is recorded inside a block is the draws and the
// one depth clear in draw.c that is not a load operation. device->window_cleared
// and each target's `cleared` carry that rule through the frame; frame.c resets
// them at _begin and clears the window at _end if no pass did.
//
// A SHADOW PASS (voe_render_shadow_pass_begin, ADR-0258) is a pass onto one
// layer of the slot's shadow map instead: depth only, always cleared, drawn
// through the shadow pipeline, and closed by the same _pass_end.
//
// A camera pass names its target's probe volume in its block only when this
// frame slot's frame began it and it is built (ADR-0326 point 7), at that
// begin's spacing (0332 point 3); a block naming none leaves the spacing nought.
//
// THE POINT-SHADOW PASS (ADR-0325) opens in point_shadow.c through this file's
// start and light copy, and is closed by the same _pass_end. A capture pass
// (ADR-0326) opens the same way in bounce_capture.c; _pass_end calls its copy.
// The bounce shadow pass (ADR-0329) opens in bounce_shadow.c; _pass_end calls its
// end, and the depth copy refuses it as a shadow pass's.
//
// The depth copy (ADR-0305), voe_render_frame_copy_depth, is in depth_copy.c.
//
// A CAMERA PASS'S POINT LIGHTS (ADR-0320) are copied into its region of the
// slot's light buffer and binned into its region of the bins buffer as it opens;
// its block names how many and the region, which is the pass's number. A light
// whose falloff is not finite or not above nought asserts there (ADR-0322), as
// does a shadow slot out of range or named twice, or a strength outside 0 to 1.
// On a device whose point shadows are not ready every slot is copied as 0, so
// no surface reads a map nothing drew.
//
// ITS LIGHT BLOCKERS (ADR-0347) are copied into its region at binding 11 beside
// each of its point lights' masks, worked out here, and its walls, indoors and
// sun masks (ADR-0350), and its block names how many; none is 0. A count past
// VOE_RENDER_LIGHT_BLOCKERS, NULL with a count, a row not finite, walls and
// indoors sharing a bit, or a kind or sun bit at or past the count asserts.
//
// ITS FURTHER LIGHTS (ADR-0357) go into its block's `more`, each one's blockers
// cut to the pass's count; a light not finite, a shadow slot past the lights
// ready or shared with another shadowed light asserts. Every other pass has none.
//
// THE ONE Y FLIP IN THE ENGINE IS HERE, in voe_render_frame_viewport, which
// every block opens with; voe_render_frame_set_viewport lets a test replace it.
#include "frame_internal.h"
#include "light_bins.h"

#include <base/assert.h>
#include <base/report.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

// The colour behind everything drawn. It is deliberately none of the colours a
// test's geometry wears, so that a person looking at the window can tell the
// background from the things in front of it — and so that
// render/tests/offscreen.c can take the clear colour out of a corner of the
// picture and count what differs from it.
//
// IT IS A LINEAR COLOUR AND NOT THE BYTES THAT REACH THE SCREEN. The target is
// an sRGB format, so the hardware encodes whatever is written into it — a clear
// of 0.5 arrives on screen as a byte of about 188 and not 128. These three
// numbers are the linear form of #17171A, the ink the engine's logo is drawn
// in, which is why they are not the round numbers a colour picker would give: a
// clear colour written as if it were sRGB comes out of an sRGB target visibly
// washed out. Convert, do not paste. It was a slate blue before this, and the
// same conversion applied then.
//
// THE LOGO'S OTHER COLOUR IS NOT A CANDIDATE. The mark is ink on yellow, and the
// yellow is #FFC000 — full red, two thirds green. A clear of it would sit inside
// the range the tests above measure against, where the background is assumed to
// be something no geometry wears, and render/tests/offscreen.c counts pixels per
// channel against exactly that assumption.
#define CLEAR_RED 0.00857f
#define CLEAR_GREEN 0.00857f
#define CLEAR_BLUE 0.01033f

// THE ONE Y FLIP IN THIS ENGINE IS voe_render_frame_viewport. Vulkan's
// clip space has +Y pointing down the screen and this engine has +Y up, and the
// whole of the reconciliation is a negative viewport height there. Never a
// negated row in a projection matrix — voe_3d_projection deliberately does not
// have one — and never both: flipping twice looks exactly like flipping none
// until something is culled, and then it is a bug nobody can see. The front-face
// constant that goes with this flip is set on the pipeline in device.c, and the
// pair of them is proven by render/tests/offscreen.c.
VkViewport voe_render_frame_viewport(VkExtent2D extent)
{
	// y at the bottom and a negative height: the flip, and the only one.
	//
	// The depth range stays the plain 0..1 identity, and it is not where this
	// engine's reversed depth lives. That comes out of the projection matrix,
	// which puts the near plane at 1.0 and the far plane at 0.0; the viewport
	// maps clip depth to the range a depth buffer stores, and 0..1 is the
	// whole of that range. Reversing it here as well would reverse it twice.
	VkViewport viewport = {
		.y = (float)extent.height,
		.width = (float)extent.width,
		.height = -(float)extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	return viewport;
}

void voe_render_frame_set_viewport(voe_render_device *device,
				   VkViewport viewport)
{
	VOE_BASE_ASSERT(device != NULL, "setting a viewport on no device");
	VOE_BASE_ASSERT(device->pass_open,
			"setting a viewport with no pass open — the next pass would set its own over it");

	voe_render_vk.cmd_set_viewport(voe_render_frame_at(device, device->slot)->commands,
				       0, 1, &viewport);
}

// The rendering block a pass draws in, onto `images` at `extent` — the window's
// pair or one frame slot's pair of a target of the caller's own: the barriers
// and the clears when `clear` says this is the first pass onto it this frame,
// the load operations otherwise, and the viewport and scissor either way.
//
// NO BARRIER WHEN LOADING THE WINDOW, AND THAT IS NOT AN OMISSION. The first
// block moved both images into their attachment layouts, ending a rendering
// block leaves them there, and nothing is recorded between two passes that moves
// them. A barrier out of UNDEFINED here would tell the driver it may throw the
// picture away, which is exactly what a load must not do.
//
// A TARGET'S COLOUR IMAGE LIVES IN GENERAL, AND `own` SAYS WHICH KIND THIS IS.
// It is an attachment here and a sampled image in every descriptor set, and
// GENERAL is the one layout that is valid for both, so the image never changes
// layout after target_own.c settled it and loading it needs no barrier either.
// See target_own.c for why one layout rather than a barrier each way.
//
// DEPTH RUNS BACKWARDS AND THE CLEAR IS THE HALF OF IT THAT LIVES HERE. The
// buffer is cleared to VOE_RENDER_DEPTH_CLEAR, which is 0, which is this
// engine's far plane; the comparison is GREATER, set on the pipeline in device.c;
// and the near plane is at 1.0, which comes out of the projection matrix in `3d`.
// Three files, one convention, and clearing to 1 instead — the habit from every
// tutorial — leaves a depth test that rejects everything. Both clears — this one
// and voe_render_frame_clear_depth's in draw.c — read that one constant, and the
// number never leaves this folder: no caller supplies it and none is told it.
//
// THE DEPTH IMAGE IS STORED. Its storeOp is STORE because a later pass onto the
// same target in the same frame loads it — that is what lets something drawn in
// the first pass hide something drawn in the second — and because
// voe_render_frame_copy_depth reopens a block that loads it. Nothing reads it
// after the frame: it is rebuilt from the clear on the next.
void voe_render_open_rendering(VkCommandBuffer commands,
			       const struct voe_render_target *images,
			       VkExtent2D extent, bool clear, bool own)
{
	// Two images into the layouts the rendering needs.
	VkImageMemoryBarrier2 barriers[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = images->colour.image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		},
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			// EARLY_FRAGMENT_TESTS is where the depth clear and the
			// depth test happen, so it is the stage that has to wait
			// for this transition rather than the colour output one.
			.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = images->depth.image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		},
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 2,
		.pImageMemoryBarriers = barriers,
	};
	VkAttachmentLoadOp load = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
					  VK_ATTACHMENT_LOAD_OP_LOAD;
	VkRenderingAttachmentInfo colour = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = images->colour.view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = load,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .color = { .float32 = { CLEAR_RED, CLEAR_GREEN,
						       CLEAR_BLUE, 1.0f } } },
	};
	// Cleared to the far plane, which is 0 here, and stored for the next
	// pass to load.
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = images->depth.view,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = load,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = { .extent = extent },
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colour,
		.pDepthAttachment = &depth,
	};
	// The scissor is the whole target and takes no part in the flip. It is
	// in framebuffer coordinates, which have no sign to get wrong.
	VkRect2D scissor = { .extent = extent };
	VkViewport viewport = voe_render_frame_viewport(extent);

	if (own) {
		barriers[0].newLayout = VK_IMAGE_LAYOUT_GENERAL;
		colour.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}
	if (clear)
		voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);

	// Both clears are load operations, so they have already happened by the
	// time the first command inside is recorded.
	voe_render_vk.cmd_begin_rendering(commands, &rendering);
	voe_render_vk.cmd_set_viewport(commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(commands, 0, 1, &scissor);
}

// What every pass does once its rendering block is open: `block` into this
// pass's own block of the slot's uniform buffer, `pipeline` bound, the slot's set
// bound at that block's offset, no pools bound yet, and the pass counted.
//
// THE BLOCK IS SAFE TO WRITE because the fence at the top of the frame says the
// GPU has finished reading what was in here two frames ago. The offset is the
// whole of how the shader comes to read this pass's camera and not another's.
//
// NO POOL IS BOUND AS A PASS OPENS (ADR-0385). The first mesh draw binds the pair
// it needs (draw_with, draw.c), and `bound_transient` keeps that to one bind per
// run. Binding the static pair here left a pass of elements alone, an interface,
// with a vertex buffer bound that nothing read, which the Best Practices layer
// reports at the end of the command buffer (bug 04).
//
// THE PASS IS TIMED AND LABELLED HERE (pass_timing.c), its first stamp written
// before anything it binds, and closed by voe_render_pass_end.
void voe_render_pass_start(voe_render_device *device,
			   struct voe_render_frame *frame,
			   const struct voe_render_frame_block *block,
			   VkPipeline pipeline, const char *name)
{
	const uint32_t offset = device->pass_count * (uint32_t)device->pass_stride;

	VOE_BASE_DEBUG_ASSERT(frame->uniforms_mapped != NULL,
			      "opening a pass whose uniform buffer is not mapped");
	voe_render_pass_timing_open(device, frame, name);
	memcpy((unsigned char *)frame->uniforms_mapped + offset, block,
	       sizeof(*block));

	voe_render_vk.cmd_bind_pipeline(frame->commands,
					VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	device->bound = pipeline;
	voe_render_vk.cmd_bind_descriptor_sets(frame->commands,
					       VK_PIPELINE_BIND_POINT_GRAPHICS,
					       device->layout, 0, 1,
					       &frame->descriptor, 1, &offset);
	device->pools_bound = false;

	device->pass_open = true;
	device->pass_count++;
	device->pass_point_shadow = false;
	device->pass_capture = false;
	device->pass_bounce_shadow = false;
}

// The bounce record of a camera pass onto the target owning `volume`, at
// descriptor index `index`: its first entry at binding 6, the corner, the
// lowest cell wrapped per axis and the spacing of frame slot `slot`'s begin,
// when that begin
// happened and the volume is built; left at VOE_RENDER_NO_BOUNCE otherwise
// (ADR-0326 point 7).
static void name_volume(struct voe_render_frame_bounce *bounce,
			const struct voe_render_bounce_volume *volume,
			uint32_t index, uint32_t slot)
{
	const uint32_t size[3] = { VOE_RENDER_BOUNCE_PROBES_XZ,
				   VOE_RENDER_BOUNCE_PROBES_Y,
				   VOE_RENDER_BOUNCE_PROBES_XZ };
	const struct voe_render_bounce_begun *begun;

	VOE_BASE_DEBUG_ASSERT(bounce != NULL && volume != NULL,
			      "naming no probe volume");
	VOE_BASE_DEBUG_ASSERT(slot < VOE_RENDER_FRAMES_IN_FLIGHT,
			      "naming a probe volume for no frame slot");
	begun = &volume->begun[slot];
	if (!begun->begun || !volume->built)
		return;
	bounce->grid = 4 * index;
	bounce->spacing = begun->spacing;
	for (uint32_t a = 0; a < 3; a++) {
		bounce->corner[a] = begun->corner[a];
		bounce->cell[a] =
			voe_render_bounce_probe_wrap(begun->cell[a], size[a]);
	}
}

// `count` lights into pass `region`'s records at binding 7 from index `first`:
// the one copy every pass's lights take, the point-shadow pass's by slot.
void voe_render_pass_copy_lights(const struct voe_render_frame *frame,
				 uint32_t region, uint32_t first,
				 const voe_render_point_light *lights,
				 uint32_t count)
{
	VOE_BASE_DEBUG_ASSERT(frame->point_lights_mapped != NULL,
			      "copying point lights into a buffer that is not mapped");
	VOE_BASE_DEBUG_ASSERT(first + count <= VOE_RENDER_POINT_LIGHTS,
			      "copying point lights past a pass's region");
	memcpy((voe_render_point_light *)frame->point_lights_mapped +
		       (size_t)region * VOE_RENDER_POINT_LIGHTS + first,
	       lights, count * sizeof(*lights));
}

// `points` into pass `region`'s records at binding 7 and its bins at binding 8,
// and the count and region into `block`; none writes 0 and bins nothing. The
// bins are filled on the stack and copied whole, because the fill reads back
// what it ORs into and the mapped memory may be write-combined. Without
// `shadows` every copied record's slot is then written 0, never read back.
static void place_lights(const struct voe_render_frame *frame, uint32_t region,
			 const voe_render_view *view,
			 voe_render_point_lights points, bool shadows,
			 struct voe_render_frame_block *block)
{
	voe_render_point_light *copied;

	struct voe_render_light_bins bins;
	bool slots[VOE_RENDER_POINT_SHADOWS + 1] = { false };

	VOE_BASE_ASSERT(points.count <= VOE_RENDER_POINT_LIGHTS,
			"a pass with more point lights than VOE_RENDER_POINT_LIGHTS");
	VOE_BASE_ASSERT(points.lights != NULL || points.count == 0,
			"a pass with a point light count and no lights");
	for (uint32_t i = 0; i < points.count; i++) {
		const voe_render_point_light *light = &points.lights[i];

		VOE_BASE_ASSERT(isfinite(light->falloff) && light->falloff > 0.0f,
				"a point light whose falloff is not finite or not above nought");
		VOE_BASE_ASSERT(light->shadow <= VOE_RENDER_POINT_SHADOWS,
				"a point light whose shadow slot is past VOE_RENDER_POINT_SHADOWS");
		VOE_BASE_ASSERT(light->shadow == 0 || !slots[light->shadow],
				"two point lights in one pass naming the same shadow slot");
		VOE_BASE_ASSERT(isfinite(light->shadow_strength) &&
					light->shadow_strength >= 0.0f &&
					light->shadow_strength <= 1.0f,
				"a point light whose shadow strength is not finite or not 0 to 1");
		VOE_BASE_ASSERT(light->bounces <= VOE_RENDER_BOUNCES_MAX,
				"a point light whose bounces are past VOE_RENDER_BOUNCES_MAX");
		VOE_BASE_ASSERT(isfinite(light->bounce_strength) &&
					light->bounce_strength >= 0.0f,
				"a point light whose bounce strength is not finite or below nought");
		if (light->shadow != 0 && light->shadow <= VOE_RENDER_POINT_SHADOWS)
			slots[light->shadow] = true;
	}
	block->region = region;
	block->lights = points.count;
	if (points.count == 0)
		return;
	VOE_BASE_DEBUG_ASSERT(frame->light_bins_mapped != NULL,
			      "placing point lights in a bin buffer that is not mapped");
	voe_render_pass_copy_lights(frame, region, 0, points.lights,
				    points.count);
	copied = (voe_render_point_light *)frame->point_lights_mapped +
		 (size_t)region * VOE_RENDER_POINT_LIGHTS;
	for (uint32_t i = 0; !shadows && i < points.count; i++)
		copied[i].shadow = 0;
	voe_render_light_bins_fill(view, points.lights, points.count, &bins);
	memcpy((struct voe_render_light_bins *)frame->light_bins_mapped + region,
	       &bins, sizeof(bins));
}

// `blockers` into pass `region`'s struct voe_render_frame_blockers at binding
// 11, each of `points`' masks beside them, the kinds and the sun's mask after,
// and the count into `block`. The masks are written even with no blockers, as
// 0, so no light reads a stale one.
static void place_blockers(const struct voe_render_frame *frame,
			   uint32_t region, voe_render_light_blockers blockers,
			   voe_render_point_lights points,
			   struct voe_render_frame_block *block)
{
	struct voe_render_frame_blockers *placed;
	const uint32_t past = blockers.count >= 32 ? 0u :
				~((1u << blockers.count) - 1u);

	VOE_BASE_ASSERT(blockers.count <= VOE_RENDER_LIGHT_BLOCKERS,
			"a pass with more light blockers than VOE_RENDER_LIGHT_BLOCKERS");
	VOE_BASE_ASSERT(blockers.blockers != NULL || blockers.count == 0,
			"a pass with a light blocker count and no blockers");
	VOE_BASE_ASSERT((blockers.walls & blockers.indoors) == 0,
			"a light blocker that is both a Wall and Indoors");
	VOE_BASE_ASSERT(((blockers.walls | blockers.indoors | blockers.sun) &
			 past) == 0,
			"a light blocker kind or sun bit at or past the blocker count");
	for (uint32_t i = 0; i < blockers.count; i++)
		for (uint32_t r = 0; r < 3; r++) {
			const voe_math_float4 row = blockers.blockers[i].rows[r];

			VOE_BASE_ASSERT(isfinite(row.x) && isfinite(row.y) &&
						isfinite(row.z) && isfinite(row.w),
					"a light blocker whose row is not finite");
		}
	VOE_BASE_DEBUG_ASSERT(frame->light_blockers_mapped != NULL,
			      "placing light blockers in a buffer that is not mapped");
	placed = (struct voe_render_frame_blockers *)frame->light_blockers_mapped +
		 region;
	if (blockers.count > 0)
		memcpy(placed->blockers, blockers.blockers,
		       blockers.count * sizeof(*blockers.blockers));
	for (uint32_t i = 0; i < points.count; i++)
		placed->masks[i] = voe_render_light_blockers_mask(
			blockers.blockers, blockers.count,
			points.lights[i].position);
	placed->walls = blockers.walls;
	placed->indoors = blockers.indoors;
	placed->sun = blockers.sun;
	block->blockers = blockers.count;
}

static bool light_is_finite(voe_render_light light)
{
	const float values[] = { light.direction.x, light.direction.y,
				 light.direction.z, light.intensity,
				 light.colour.x,    light.colour.y,
				 light.colour.z,    light.fill.x,
				 light.fill.y,      light.fill.z };

	static_assert(sizeof(values) / sizeof(values[0]) == 10,
		      "a light is ten floats");
	for (uint32_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
		if (!isfinite(values[i]))
			return false;
	return true;
}

// `camera`'s lights after the first into `block->more` (ADR-0357 point 1),
// each one's blockers cut to the pass's blocker count; checked as the first is,
// against the lights the shadow array holds, and no two shadowed lights, the
// first included, on one slot.
static void place_further_lights(const voe_render_device *device,
				 const voe_render_pass_camera *camera,
				 struct voe_render_frame_block *block)
{
	const voe_render_directional_lights more = camera->more;
	const uint32_t count = camera->blockers.count;
	const uint32_t held = count >= 32 ? ~0u : (1u << count) - 1u;
	bool slots[VOE_RENDER_DIRECTIONAL_LIGHTS] = { false };

	VOE_BASE_ASSERT(more.count <= VOE_RENDER_DIRECTIONAL_LIGHTS - 1,
			"a pass with more directional lights than VOE_RENDER_DIRECTIONAL_LIGHTS");
	VOE_BASE_ASSERT(more.lights != NULL || more.count == 0,
			"a pass with a further light count and no lights");
	if (camera->shadow.count > 0 &&
	    camera->shadow.slot < VOE_RENDER_DIRECTIONAL_LIGHTS)
		slots[camera->shadow.slot] = true;
	for (uint32_t i = 0; i < more.count; i++) {
		const voe_render_directional_light *light = &more.lights[i];
		struct voe_render_frame_light *placed = &block->more[i];

		VOE_BASE_ASSERT(light_is_finite(light->light),
				"a further directional light that is not finite");
		if (light->shadow.count > 0) {
			VOE_BASE_ASSERT(light->shadow.slot < device->shadow_lights,
					"a further light whose shadow slot is past the lights the shadow array holds; ask voe_render_shadow_lights_ready first");
			VOE_BASE_ASSERT(!slots[light->shadow.slot],
					"two directional lights in one pass on the same shadow slot");
			slots[light->shadow.slot] = true;
		}
		placed->light = light->light;
		placed->shadow = light->shadow;
		placed->blockers = light->blockers & held;
	}
	block->more_count = more.count;
	VOE_BASE_DEBUG_ASSERT(block->more_count < VOE_RENDER_DIRECTIONAL_LIGHTS,
			      "placed more further lights than the block holds");
}

// A PASS IS ONE RENDERING BLOCK AND THE FRAME IS ANY NUMBER OF THEM (ADR-0148).
// _begin records nothing that draws; _pass_begin opens the block, writes the
// pass's camera into its own block of the slot's uniform buffer and binds the set
// at that block's offset; _pass_end ends the block. The first pass onto the
// window in a frame clears colour and depth and moves both images out of
// UNDEFINED; every later one loads both, and needs no barrier because nothing
// between two passes moves either image out of its attachment layout. That is
// also why depth is stored: a later pass loads it. The colour image leaves its
// attachment layout once, at _end, on its way to the blit.
//
// A PASS ONTO A TARGET OF THE CALLER'S OWN FOLLOWS THE SAME CLEAR RULE. It draws
// into this frame slot's pair of that target, at that target's size, which is
// what device->pass_extent carries to everything that used to read the window's
// resolution inside a pass. Its colour image is in GENERAL for its whole life, as
// an attachment and as the picture any descriptor set shows, so the one barrier
// is the clearing pass's out of UNDEFINED and _pass_end records none. A target no
// pass opens this frame is not touched at all: nothing clears it at _end the way
// the window is, so it keeps what its slot's image held.
//
// THE CAMERA AND THE SUN ARE ONE BLOCK PER PASS IN ONE BUFFER, AND _pass_begin IS
// WHERE THEY LAND. The pass's number picks the block, and the same number times
// device->pass_stride is the dynamic offset the set is bound with, so the shader
// reads the block of the pass it is drawn in. A pass with no camera writes a
// zeroed block that nothing reads. See struct voe_render_frame_block in
// device_internal.h.
bool voe_render_pass_begin(voe_render_device *device, voe_render_target target,
			   const voe_render_pass_camera *camera)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { 0 };
	struct voe_render_target_slot *own = NULL;
	char name[VOE_RENDER_PASS_NAME];

	VOE_BASE_ASSERT(device != NULL, "opening a pass on no device");
	VOE_BASE_ASSERT(device->recording,
			"opening a pass with no frame open — voe_render_frame_begin said there was nothing to draw into, or _end has already run");
	VOE_BASE_ASSERT(!device->pass_open,
			"opening a pass while one is already open — passes do not nest, and every _pass_begin needs its _pass_end");
	if (target.index != VOE_RENDER_TARGET_WINDOW.index ||
	    target.generation != VOE_RENDER_TARGET_WINDOW.generation) {
		own = voe_render_target_at(device, target);
		VOE_BASE_ASSERT(own != NULL,
				"opening a pass onto a target id that names no target — it did not come out of voe_render_target_create on this device");
	}

	if (device->pass_count >= device->capacities.passes) {
		VOE_BASE_ERROR("render",
			       "this frame has already opened %u of %u passes; `passes` is too small for what this frame draws",
			       device->pass_count, device->capacities.passes);
		return false;
	}
	// A camera pass may draw a mesh, so the device is prepared first; a pass
	// with none draws elements alone and builds nothing.
	if (camera != NULL && !voe_render_device_ready(device))
		return false;

	frame = voe_render_frame_at(device, device->slot);

	// The camera, the sun and its shadow, into this pass's own block of
	// this slot's buffer. Safe because the fence at the top of the frame says
	// the GPU has finished reading what was in here two frames ago. A pass with no
	// camera writes a zeroed block, which no draw it may make reads.
	if (camera != NULL) {
		block.camera = camera->view;
		block.light = camera->light;
		VOE_BASE_ASSERT(camera->shadow.slot < device->shadow_lights,
				"a camera whose shadow slot is past the lights the shadow array holds; ask voe_render_shadow_lights_ready first");
		block.shadow = camera->shadow;
	}
	block.depth_copy = VOE_RENDER_NO_DEPTH_COPY;
	block.bounce.grid = VOE_RENDER_NO_BOUNCE;
	if (camera != NULL) {
		name_volume(&block.bounce,
			    own != NULL ? &own->volume[0] :
					  &device->window_volume[0],
			    voe_render_bounce_volume_index(
				    own != NULL ?
					    (uint32_t)(own - device->targets) + 1 :
					    0,
				    0),
			    device->slot);
		place_lights(frame, device->pass_count, &camera->view,
			     camera->points,
			     voe_render_point_shadows_ready(device), &block);
		place_blockers(frame, device->pass_count, camera->blockers,
			       camera->points, &block);
		place_further_lights(device, camera, &block);
	}

	// The window's pair or this frame slot's pair of the target, each with
	// its own clear rule and its own size. A frame in slot n draws into slot
	// n's image, which is the image slot n's descriptor set shows.
	if (own == NULL) {
		voe_render_open_rendering(frame->commands, &frame->target,
					  device->resolution,
					  !device->window_cleared, false);
		device->window_cleared = true;
		device->pass_extent = device->resolution;
	} else {
		voe_render_open_rendering(frame->commands,
					  &own->images[device->slot],
					  own->extent, !own->cleared, true);
		own->cleared = true;
		device->pass_extent = own->extent;
	}
	device->pass_target = own;

	// The name from the kind (ADR-0367 point 1): a camera pass is a view, one
	// with none an interface, onto the window or target N.
	if (own == NULL)
		(void)snprintf(name, sizeof(name), "%s window",
			       camera != NULL ? "view" : "interface");
	else
		(void)snprintf(name, sizeof(name), "%s target %u",
			       camera != NULL ? "view" : "interface",
			       target.index);

	// The solid pipeline, because a pass's opaque and cutout draws come
	// first; a blended draw binds the other one and `bound` is what keeps a
	// run of either kind to a single bind. A pass with no camera draws only
	// elements, on a device that may not have the solid one yet.
	voe_render_pass_start(device, frame, &block,
			      camera != NULL ? device->pipeline :
					       device->pipeline_elements,
			      name);
	device->pass_camera = camera != NULL;
	device->pass_shadow = false;
	return true;
}

// A SHADOW PASS IS A PASS ONTO ONE LAYER OF THIS SLOT'S SHADOW MAP (ADR-0258).
// It counts against `passes` and writes a camera block like any other — the
// light's view, the sun zeroed because nothing in it is lit — but its rendering
// block has depth alone, always cleared, and its draws go through the shadow
// pipeline. The layer comes out of the layout the shader reads it in and goes
// back at _pass_end (shadow.c's barriers), so a later pass in the frame may read
// it and a later shadow pass may draw it again.
//
// THE VIEWPORT IS THE ENGINE'S ONE, flip and all. The light's projection is built
// by the same rules a camera's is, and card 04 reads the map back through the
// same flip, so drawing it unflipped would put every shadow upside down.
bool voe_render_shadow_pass_begin(voe_render_device *device, uint32_t layer,
				  const voe_render_view *light)
{
	struct voe_render_frame *frame;
	struct voe_render_frame_block block = { 0 };
	VkExtent2D extent;
	VkRenderingAttachmentInfo depth = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .depthStencil = { .depth = VOE_RENDER_DEPTH_CLEAR } },
	};
	VkRenderingInfo rendering = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.layerCount = 1,
		.pDepthAttachment = &depth,
	};
	VkViewport viewport;
	VkRect2D scissor = { 0 };
	char name[VOE_RENDER_PASS_NAME];

	VOE_BASE_ASSERT(device != NULL, "opening a shadow pass on no device");
	VOE_BASE_ASSERT(light != NULL, "opening a shadow pass with no light view");
	VOE_BASE_ASSERT(device->recording,
			"opening a shadow pass with no frame open");
	VOE_BASE_ASSERT(!device->pass_open,
			"opening a shadow pass while a pass is already open — passes do not nest");
	VOE_BASE_ASSERT(layer < VOE_RENDER_SHADOW_CASCADES * device->shadow_lights,
			"opening a shadow pass onto a layer the map does not have; ask voe_render_shadow_lights_ready first");
	VOE_BASE_ASSERT(device->capacities.shadow_size > 0,
			"opening a shadow pass on a device made with shadow_size nought");

	if (device->pass_count >= device->capacities.passes) {
		VOE_BASE_ERROR("render",
			       "this frame has already opened %u of %u passes, so a shadow pass does not fit; `passes` is too small for what this frame draws",
			       device->pass_count, device->capacities.passes);
		return false;
	}

	if (!voe_render_device_ready(device))
		return false;

	frame = voe_render_frame_at(device, device->slot);
	extent = (VkExtent2D){ device->capacities.shadow_size,
			       device->capacities.shadow_size };
	block.camera = *light;
	block.depth_copy = VOE_RENDER_NO_DEPTH_COPY;
	block.bounce.grid = VOE_RENDER_NO_BOUNCE;
	depth.imageView = frame->shadow.layers[layer];
	rendering.renderArea.extent = extent;
	scissor.extent = extent;
	viewport = voe_render_frame_viewport(extent);

	voe_render_shadow_to_attachment(frame, layer);
	voe_render_vk.cmd_begin_rendering(frame->commands, &rendering);
	voe_render_vk.cmd_set_viewport(frame->commands, 0, 1, &viewport);
	voe_render_vk.cmd_set_scissor(frame->commands, 0, 1, &scissor);

	device->pass_target = NULL;
	device->pass_extent = extent;
	(void)snprintf(name, sizeof(name), "shadow light %u cascade %u",
		       layer / VOE_RENDER_SHADOW_CASCADES,
		       layer % VOE_RENDER_SHADOW_CASCADES);
	voe_render_pass_start(device, frame, &block, device->pipeline_shadow,
			      name);
	device->pass_camera = true;
	device->pass_shadow = true;
	device->pass_layer = layer;
	return true;
}

// Any kind of pass: the rendering block ended, and a shadow pass's layer or
// the point-shadow pass's maps handed back to where a shader reads it, or a
// capture pass's pictures copied into its atlases.
void voe_render_pass_end(voe_render_device *device)
{
	struct voe_render_frame *frame;

	VOE_BASE_ASSERT(device != NULL, "closing a pass on no device");
	VOE_BASE_ASSERT(device->pass_open,
			"closing a pass that is not open — _pass_begin returned false, or _pass_end has already run");

	frame = voe_render_frame_at(device, device->slot);
	voe_render_vk.cmd_end_rendering(frame->commands);
	if (device->pass_shadow)
		voe_render_shadow_to_read(frame, device->pass_layer);
	if (device->pass_point_shadow)
		voe_render_point_shadow_to_read(frame);
	if (device->pass_capture)
		voe_render_bounce_capture_end(device);
	if (device->pass_bounce_shadow)
		voe_render_bounce_shadow_end(device);
	// After the hand-backs, so a pass's time holds its whole cost.
	voe_render_pass_timing_close(device, frame);
	device->pass_open = false;
	device->pass_camera = false;
	device->pass_shadow = false;
	device->pass_point_shadow = false;
	device->pass_capture = false;
	device->pass_bounce_shadow = false;
	device->pass_target = NULL;
}

bool voe_render_pass_is_open(const voe_render_device *device)
{
	VOE_BASE_ASSERT(device != NULL, "asking no device whether a pass is open");
	return device->pass_open;
}
