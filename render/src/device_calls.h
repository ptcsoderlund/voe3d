// The calls one file of render/src makes into another, grouped by the file that
// owns each: swapchain.c's, target.c's and the rest, through probe.c's draw. The
// device struct they take is in device_internal.h, which includes this file at
// its end so every file that includes it sees the same declarations; this file
// is not included on its own. A new call between files is declared here, beside
// its owner's group.
#pragma once

#include "device_parts.h"
#include "loader.h"

#include <base/arena.h>
#include <platform/window.h>
#include <render/device.h>

// swapchain.c. Both are safe to call on a device whose swapchain was never
// built, and _destroy waits for the device to go idle before it takes anything
// away. Neither does anything on a headless device.
[[nodiscard]] bool voe_render_swapchain_build(voe_render_device *device,
					      voe_platform_size size);
void voe_render_swapchain_teardown(voe_render_device *device);

// target.c. The offscreen images — colour and depth both, one pair per frame
// slot, all built at size. Same contract as the pair above: safe on a device
// that never had any, and idle before anything is taken away.
[[nodiscard]] bool voe_render_target_build(voe_render_device *device,
					   voe_platform_size size);
void voe_render_target_teardown(voe_render_device *device);

// target.c, and used by target_own.c as well: one device-local image, its memory
// and its view, with `what` naming both and the messages; false with a message.
// _teardown gives back whatever a build made, is safe on a zeroed struct and on
// one whose build stopped part way, and leaves it zeroed. Neither waits.
[[nodiscard]] bool
voe_render_target_image_build(voe_render_device *device,
			      struct voe_render_allocated_image *out,
			      VkExtent2D extent, VkFormat format,
			      VkImageUsageFlags usage, VkImageAspectFlags aspect,
			      const char *what);
void voe_render_target_image_teardown(voe_render_device *device,
				      struct voe_render_allocated_image *image);

// target.c, for target_own.c too. _depth_copy_build makes one D32 sampled copy
// image (transfer-dst, depth-aspect view). _settle submits `count` barriers in a
// one-shot command buffer and waits idle; _settle_copy is the barrier that moves
// a copy from UNDEFINED to SHADER_READ_ONLY_OPTIMAL, where it rests. _free_texture
// is the lowest unclaimed texture slot from 1 that is not `skip`, or 0 if none.
[[nodiscard]] bool
voe_render_target_depth_copy_build(voe_render_device *device,
				   struct voe_render_allocated_image *out,
				   VkExtent2D extent, const char *what);
[[nodiscard]] bool voe_render_target_settle(voe_render_device *device,
					    const VkImageMemoryBarrier2 *barriers,
					    uint32_t count);
[[nodiscard]] VkImageMemoryBarrier2 voe_render_target_settle_copy(VkImage image);
[[nodiscard]] uint32_t
voe_render_target_free_texture(const voe_render_device *device, uint32_t skip);

// target.c: _settle's one-shot submit, then each of `clears` cleared to nought
// in GENERAL, and idle. False with a message.
[[nodiscard]] bool voe_render_target_settle_cleared(
	voe_render_device *device, const VkImageMemoryBarrier2 *barriers,
	uint32_t count, const VkImage *clears, uint32_t clear_count);

// bounce_volume.c. _build makes one probe volume's images, cleared in GENERAL,
// and idles; false with a message, nothing left behind. _teardown is safe on an
// unbuilt volume, leaves it zeroed and does not wait. _apply, at the top of a
// frame beside voe_render_targets_apply_resizes, builds every wanted volume and
// frees every one with no begin for VOE_RENDER_BOUNCE_IDLE frames, over the
// window and every live target, idling once only when one does; false when a
// build failed.
#define VOE_RENDER_BOUNCE_IDLE 300
[[nodiscard]] bool
voe_render_bounce_volume_build(voe_render_device *device,
			       struct voe_render_bounce_volume *volume);
void voe_render_bounce_volume_teardown(voe_render_device *device,
				       struct voe_render_bounce_volume *volume);
[[nodiscard]] bool voe_render_bounce_volumes_apply(voe_render_device *device);
// bounce_volume.c. Volume `volume` of what `target` names, the window or a live
// target; asserts on a target id that names none or a volume past
// VOE_RENDER_BOUNCE_VOLUMES.
struct voe_render_bounce_volume *
voe_render_bounce_volume_of(voe_render_device *device, voe_render_target target,
			    uint32_t volume);

// A volume's descriptor index: slot × VOE_RENDER_BOUNCE_VOLUMES + volume, the
// window slot 0 and target n slot n (its id's index). Binding 6's entries 4 ×
// it to 4 × it + 3 and binding 10's entry it are the volume's, and so are the
// relight's set, list band and record region.
static inline uint32_t voe_render_bounce_volume_index(uint32_t slot,
						      uint32_t volume)
{
	return slot * VOE_RENDER_BOUNCE_VOLUMES + volume;
}

// bounce_capture.c. Every frame slot's capture scratch on a device with
// output_layer, none without; startup's, after the point shadow maps. False with
// a message; _shutdown is safe on a device that never got that far. _end, from
// voe_render_pass_end once the capture pass's rendering has ended, copies its
// probes' faces into their volume's atlases.
[[nodiscard]] bool voe_render_bounce_capture_startup(voe_render_device *device);
void voe_render_bounce_capture_shutdown(voe_render_device *device);
void voe_render_bounce_capture_end(voe_render_device *device);

// bounce_relight.c. The relight's pipeline, set layout, pool, sets and every
// slot's list buffer on a device with output_layer, none without; prepare's last
// step (pipeline.c). False with a message; _shutdown is safe on a device that
// never got that far or never started it.
[[nodiscard]] bool voe_render_bounce_relight_startup(voe_render_device *device);
void voe_render_bounce_relight_shutdown(voe_render_device *device);
// pipeline.c. voe_render_device_prepare until nothing is left: true once
// prepared, false when a step failed. Every pass that can use a mesh pipeline,
// and voe_render_bounce_begin, calls this first.
[[nodiscard]] bool voe_render_device_ready(voe_render_device *device);
// pipeline.c and element.c. Their embedded shaders, which pipeline_cache.c
// hashes into its header so a cache from another build is refused.
extern const unsigned char voe_render_draw_spv[];
extern const size_t voe_render_draw_spv_size;
extern const unsigned char voe_render_elements_spv[];
extern const size_t voe_render_elements_spv_size;
// pipeline_cache.c. The device's empty cache, made at open; false with a
// message. device.c destroys it at close.
[[nodiscard]] bool voe_render_pipeline_cache_create(voe_render_device *device);

// bounce_relight.c. The bouncing lights this frame's begin placed, as
// voe_render_bounce_probes_relight_needed compares them.
void voe_render_bounce_begun_lights(const voe_render_device *device,
				    voe_render_bounce_lights *lights);

// bounce_shadow.c. Every frame slot's relight sun map on a device with
// output_layer, settled where the relight reads it, none without; startup's,
// beside the capture scratch. False with a message; _shutdown is safe on a device
// that never got that far. _end, from voe_render_pass_end once the bounce shadow
// pass's rendering has ended, hands the map to a compute read.
[[nodiscard]] bool voe_render_bounce_shadow_startup(voe_render_device *device);
void voe_render_bounce_shadow_shutdown(voe_render_device *device);
void voe_render_bounce_shadow_end(voe_render_device *device);

// target_own.c. The targets of the caller's own, as distinct from the window's pair
// above: the table of them made at startup, and every image any of them holds
// given back at shutdown. _startup allocates no image; _shutdown is safe on a
// device that never got as far as _startup.
void voe_render_targets_startup(voe_render_device *device);
void voe_render_targets_shutdown(voe_render_device *device);

// shadow.c. Every frame slot's shadow map, made and settled in the layout it
// rests in, and the comparison sampler they are read through; one texel a side
// when capacities.shadow_size is nought. Startup's,
// after the frame objects and before the descriptors. False with a message;
// _shutdown is safe on a device that never got that far.
[[nodiscard]] bool voe_render_shadow_startup(voe_render_device *device);
void voe_render_shadow_shutdown(voe_render_device *device);

// shadow.c. When voe_render_shadow_lights_ready asked for more lights than every
// slot's array holds: the device goes idle, each array is made again at that
// count and settled, and binding 5 of every set names the new view. Does nothing,
// and does not wait, otherwise. False with a message. Called by
// voe_render_frame_begin beside voe_render_bounce_volumes_apply and nowhere else.
[[nodiscard]] bool voe_render_shadow_grow(voe_render_device *device);

// shadow.c. The barriers either side of a shadow pass onto one layer of
// `frame`'s map: into the depth attachment layout before, back to where the
// shader reads it after.
void voe_render_shadow_to_attachment(const struct voe_render_frame *frame,
				     uint32_t layer);
void voe_render_shadow_to_read(const struct voe_render_frame *frame,
			       uint32_t layer);

// point_shadow.c. Every frame slot's point shadow maps, made and settled where the
// shader reads them; one texel a side when device->point_shadow_size is nought.
// Startup's, after the sun's maps and before the descriptors. False with a
// message; _shutdown is safe on a device that never got that far.
[[nodiscard]] bool voe_render_point_shadow_startup(voe_render_device *device);
void voe_render_point_shadow_shutdown(voe_render_device *device);

// target_own.c. What a target id names, or NULL when it names nothing — the window's
// id included, which is not in the table. The one place a target id is checked.
[[nodiscard]] struct voe_render_target_slot *
voe_render_target_at(voe_render_device *device, voe_render_target target);

// target_own.c. Every resize voe_render_target_resize recorded, applied: the device
// goes idle, the images of each target whose wanted size differs are made again
// and settled into the layout they rest in, and the descriptor sets are pointed
// at them. Does nothing, and does not wait, when no size differs. False when the
// card refused an image, with a message. Called by voe_render_frame_begin, where
// the window's rebuild is, and nowhere else.
[[nodiscard]] bool voe_render_targets_apply_resizes(voe_render_device *device);

// target.c, and used by render/tests/offscreen.c as well: the index of a memory
// type this card offers that is in mask and has every one of properties.
// UINT32_MAX when there is none — which is the driver's answer and so is
// reported by the caller, not asserted on here.
[[nodiscard]] uint32_t voe_render_memory_type(const voe_render_device *device,
					      uint32_t mask,
					      VkMemoryPropertyFlags properties);

// pass.c. The engine's viewport for a target of this size, and the one Y flip
// in the engine — read the comment above it in pass.c before touching it.
VkViewport voe_render_frame_viewport(VkExtent2D extent);

// frame.c. The slot the next frame will use, and the one the recording that is
// open belongs to. Reading it is how the tests get at a frame's target and its
// fence without indexing an array they would have to bound-check themselves.
const struct voe_render_frame *voe_render_frame_current(const voe_render_device *device);

// frame.c. The slot the open recording belongs to, writable, for the one thing
// another file writes into a frame slot: geometry.c's transient pools. Asserts
// if no recording is open, which is the same assert every draw makes.
struct voe_render_frame *voe_render_frame_open(voe_render_device *device);

// pass.c. Which viewport the open pass was begun with, so that a test can
// hand in its mirror image. The mirror is what gets a back face in front of the
// rasteriser without a second shader and without touching the pipeline whose
// front-face constant is the thing under test: the same geometry drawn through a
// mirrored viewport is wound the other way round in framebuffer space, so every
// face the engine would cull is drawn and every face it would draw is culled.
//
// SETTING IT IS A COMMAND AND NOT A STATE CHANGE. The viewport is dynamic state,
// so this records a vkCmdSetViewport into the open recording and everything
// drawn after it in the pass uses the new one. Asserts if no pass is open.
void voe_render_frame_set_viewport(voe_render_device *device,
				  VkViewport viewport);

// device.c, used by swapchain.c: the format the surface was opened with, decided
// once because the surface does not change when the window resizes.
[[nodiscard]] bool voe_render_device_choose_format(voe_render_device *device,
						   voe_base_arena *arena);

// device.c. The guard (device_internal.h): _take locks it, first waiting while
// a frame is open on a thread other than the caller's; _give wakes every waiter
// and unlocks. A frame open on the caller's own thread does not wait. Each take
// has its give on every path out, and neither nests.
void voe_render_device_guard_take(voe_render_device *device);
void voe_render_device_guard_give(voe_render_device *device);

// The frame's own pair of timestamps: one as its command buffer starts and one
// as it finishes, at 0 and 1 of the slot's pool. Named so the read, the buffer
// read into and the first pass's index say the same 2.
#define VOE_RENDER_FRAME_TIMESTAMPS 2

// pass_timing.c's group (ADR-0367 point 2). How many passes a frame may time,
// `passes` and one for the relight, and so how many timestamps one frame's pool
// holds: the frame's pair, a pair per timed pass, then a pair per span.
static inline uint32_t voe_render_timed_passes(const voe_render_device *device)
{
	return device->capacities.passes + 1;
}

static inline uint32_t
voe_render_timestamps_per_frame(const voe_render_device *device)
{
	return VOE_RENDER_FRAME_TIMESTAMPS + 2 * voe_render_timed_passes(device) +
	       2 * VOE_RENDER_FRAME_SPANS;
}

// pass_timing.c. _open labels a pass `name` and writes its first timestamp;
// _close writes its second and ends the label; a pass past the room, or on a
// card without timestamps, is labelled and not timed. _read, after the slot's
// fence, turns `frame`'s pairs into device->pass_times. _seconds is the time
// between two readings, masked to the valid bits and wrapped, in seconds.
void voe_render_pass_timing_open(voe_render_device *device,
				 struct voe_render_frame *frame, const char *name);
void voe_render_pass_timing_close(voe_render_device *device,
				  struct voe_render_frame *frame);
void voe_render_pass_timing_read(voe_render_device *device,
				 const struct voe_render_frame *frame);
[[nodiscard]] double voe_render_timestamp_seconds(const voe_render_device *device,
						  uint64_t start, uint64_t end);

// debug_names.c. An object's name and a command buffer's labelled span, for a
// capture tool to read; each does nothing when VK_EXT_debug_utils is absent.
// Begin and end pair inside one command buffer.
void voe_render_debug_name(const voe_render_device *device, VkObjectType type,
			   uint64_t handle, const char *name);
void voe_render_debug_label_begin(VkCommandBuffer commands, const char *name);
void voe_render_debug_label_end(VkCommandBuffer commands);

// best_practices.c (ADR-0367 points 4, 5 and 7). _allowed: `id_name` is on the
// allowlist. _other_vendor: `id_name` carries a vendor tag (NVIDIA, AMD, Arm,
// IMG) that is not `vendor_id`'s. _classify: what the messenger does with one
// message, an error or a warning — another vendor's dropped, an allowed warning
// allowed, anything else counted in device->new_messages as new and kept by its
// id name with the first `message` text (may be NULL). _announce: the one line
// saying the checks are on or missing, the vendor and the list's length.
// _list_new: at close, when new_messages is not nought, one line per kept id —
// error or warning, the id, how many times, the first text — and one for the
// new messages no row had room for, printed before the count line. _wanted
// (ADR-0398): whether a device with the layer turns the checks on — always when
// headless, on a window only when `setting` (VOE_RENDER_BEST_PRACTICES, or NULL)
// is exactly "1". Pure: no device and no environment.
enum voe_render_message_verdict {
	VOE_RENDER_MESSAGE_DROPPED,
	VOE_RENDER_MESSAGE_ALLOWED,
	VOE_RENDER_MESSAGE_NEW,
};
[[nodiscard]] bool voe_render_best_practices_allowed(const char *id_name);
[[nodiscard]] bool voe_render_best_practices_other_vendor(uint32_t vendor_id,
							  const char *id_name);
[[nodiscard]] enum voe_render_message_verdict
voe_render_best_practices_classify(voe_render_device *device, bool error,
				   const char *id_name, const char *message);
[[nodiscard]] bool voe_render_best_practices_wanted(bool headless,
						    const char *setting);
void voe_render_best_practices_announce(const voe_render_device *device);
void voe_render_best_practices_list_new(const voe_render_device *device);

// buffer.c. A buffer of size with usage, in memory that has properties, and the
// one allocation under it, the buffer named `name` (debug_names.c).
// build/teardown rather than new/destroy because the struct is the caller's and only what is inside it belongs to these — the same
// shape voe_render_target_build has, for the same reason.
//
// Teardown is safe on a zeroed struct and on one whose build failed part way,
// and it leaves the struct zeroed. It does not wait for the device to go idle:
// unlike a target, a buffer here is either startup's or a slot's, and both of
// those already know when the GPU has finished with them.
[[nodiscard]] bool voe_render_buffer_build(voe_render_device *device,
					   struct voe_render_buffer *buffer,
					   VkDeviceSize size,
					   VkBufferUsageFlags usage,
					   VkMemoryPropertyFlags properties,
					   const char *name);
void voe_render_buffer_teardown(voe_render_device *device,
				struct voe_render_buffer *buffer);

// buffer.c. Fills a device-local buffer by copying bytes through a host-visible
// staging buffer, and this is the pattern every later upload follows — a texture
// and a loaded mesh both arrive this way, which is why it is a function here and
// not four lines inside geometry.c.
//
// IT IS STARTUP'S AND IT BLOCKS. The staging buffer is made, filled, copied and
// destroyed inside one call, which means waiting for the copy to finish before
// the staging buffer can go away. That is the right trade for data uploaded once
// before the first frame and the wrong one for anything uploaded per frame; the
// day something needs the second, it needs a different function and not a flag
// on this one.
// `offset` IS IN BYTES AND IS WHAT MAKES A POOL POSSIBLE. Appending a mesh is an
// upload into the middle of a buffer that already holds other meshes, so the
// destination offset is a parameter rather than always zero.
[[nodiscard]] bool voe_render_buffer_upload(voe_render_device *device,
					    const struct voe_render_buffer *buffer,
					    VkDeviceSize offset,
					    const void *data, VkDeviceSize size);

// descriptors.c. Everything the shader reads that a resize does not touch: the
// descriptor set layout, the pool, and one set, one mapped uniform buffer and
// one mapped object buffer per frame slot. Startup's, and the counterpart tears
// down whatever was built before a failure.
[[nodiscard]] bool voe_render_descriptors_build(voe_render_device *device);
void voe_render_descriptors_teardown(voe_render_device *device);

// descriptors.c. Names built `volume`'s sum and validity at binding 6's entries
// 4 × index to 4 × index + 3, and its moments atlas at binding 10's entry
// `index`, of every slot's set: `index` its descriptor index
// (voe_render_bounce_volume_index). No frame may be reading the sets: a volume
// is built outside one, and its build idles.
void voe_render_descriptors_write_volume(
	voe_render_device *device, const struct voe_render_bounce_volume *volume,
	uint32_t index);

// descriptors.c. Points a slot's set at the shared shading buffer. Called once
// per slot at startup, after the buffer exists; a record written later needs no
// second write, because the descriptor names the buffer and not its contents.
void voe_render_descriptors_write_shadings(voe_render_device *device,
					   VkDescriptorSet set);

// geometry.c. The two static pools, the transient pair in every frame slot, and
// the table of ranges into all of them. Startup's; the frames have to exist
// before this runs, because the transient pools live in them.
[[nodiscard]] bool voe_render_geometry_startup(voe_render_device *device);
void voe_render_geometry_shutdown(voe_render_device *device);

// geometry.c. The top of a frame for the transient half: every live transient
// slot stops being live and moves its generation on, and `frame`'s two transient
// pools go back to empty. Called by voe_render_frame_begin once the slot's fence
// has been waited on and nowhere else — that fence is what makes emptying the
// pools safe.
void voe_render_geometry_frame_reset(voe_render_device *device,
				     struct voe_render_frame *frame);

// geometry.c. What an id names, or NULL when it names nothing — a slot that was
// never claimed, or one whose generation has moved on. The one place a geometry
// id is turned into a range, so it is the one place that check is made.
const struct voe_render_geometry_slot *
voe_render_geometry_at(const voe_render_device *device,
		       voe_render_geometry geometry);

// shading.c. The record buffer and the slots that name its rows. Startup's.
[[nodiscard]] bool voe_render_shading_startup(voe_render_device *device);
void voe_render_shading_shutdown(voe_render_device *device);

// element.c. The element pipeline and nothing else — the per-slot record buffers
// are descriptors.c's, because they are things the shader reads. Startup's, and
// it has to run after voe_render_pipeline_layout_create because it shares
// device->layout.
[[nodiscard]] bool voe_render_element_startup(voe_render_device *device);
void voe_render_element_shutdown(voe_render_device *device);

// texture.c. The samplers and the one-pixel white texture every unclaimed slot
// points at, made once at startup. False with a message on failure.
[[nodiscard]] bool voe_render_texture_startup(voe_render_device *device);

// texture.c. Every texture, every sampler, and nothing else. Safe on a device
// that never got as far as making them.
void voe_render_texture_shutdown(voe_render_device *device);

// texture.c. Point one frame slot's descriptor set at every texture in the
// table. Called when a slot's set is built and again whenever the table changes.
//
// IT TAKES THE FRAME SLOT AND NOT THE SET, because a texture slot that shows a
// target names a different image in every frame slot — that frame slot's own —
// and the set alone does not say which frame slot it belongs to.
void voe_render_texture_write_descriptors(voe_render_device *device,
					  uint32_t slot);

// texture.c, for texture_heights.c too. Builds free `slot`'s image of `format`
// and `levels`, uploads `size` bytes of `pixels` into level 0 (the chain blitted
// when `levels` > 1), leaves it shader-readable and idles. False with a message
// and the slot's handles given back; the caller claims the slot.
[[nodiscard]] bool voe_render_texture_fill(voe_render_device *device,
					   struct voe_render_texture_slot *slot,
					   VkFormat format, uint32_t width,
					   uint32_t height, uint32_t levels,
					   const void *pixels, VkDeviceSize size);

// texture_heights.c. Every frame slot's heights staging, capacities.heights_texels
// floats, mapped; none when nought. Startup's, false with a message; _shutdown is
// safe on a device that never got that far.
[[nodiscard]] bool voe_render_texture_heights_startup(voe_render_device *device);
void voe_render_texture_heights_shutdown(voe_render_device *device);

// texture_levels.c. Record the blits that fill levels 1 .. level_count - 1 of
// a texture from level 0, into the upload's own command buffer. Level 0 and
// every other level arrive in TRANSFER_DST and all leave in SHADER_READ_ONLY.
void voe_render_texture_levels_record(voe_render_device *device,
				      VkCommandBuffer commands, VkImage image,
				      uint32_t width, uint32_t height,
				      uint32_t level_count);

// probe.c. The pipeline that reads a matrix and reports what it saw, built on
// demand and owned by the caller — VK_NULL_HANDLE on failure, and destroyed with
// voe_render_vk.destroy_pipeline. It shares the device's pipeline layout, so it
// reads the same descriptor a drawn object does.
//
// IT IS BUILT ON DEMAND AND NOT AT STARTUP, WHICH IS THE WHOLE REASON IT IS A
// FUNCTION. Only render/tests/matrix.c ever asks for it; a shipping device that
// created it would be paying for a pipeline nothing draws, which is what rule 10
// is about. The compiled shader is in the binary either way, because
// --embed-dir is private to this folder's library and a test cannot #embed.
[[nodiscard]] VkPipeline voe_render_probe_pipeline_new(voe_render_device *device);

// probe.c. Draws the probe over the whole of a slot's colour target and leaves it
// in TRANSFER_SRC_OPTIMAL, exactly as voe_render_frame_draw leaves it. No depth
// attachment and no culling: what is under test is a matrix, and a probe that
// could be culled or depth-rejected would report nothing.
//
// The matrix it reports is whatever the caller has already put in the slot's
// mapped uniform buffer. This writes nothing there.
void voe_render_probe_draw(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   VkPipeline pipeline, VkViewport viewport);
