// The innards of voe_render_device, shared by the files that make one: device.c
// starts it, descriptors.c builds what the shader reads, geometry.c and
// shading.c hold what a caller uploads, target.c makes the images the scene is
// drawn into, shadow.c the sun's depth maps, point_shadow.c the point lights',
// bounce_volume.c the probe volumes, bounce_capture.c their capture pass, bounce_relight.c their relight, bounce_shadow.c its sun map, swapchain.c builds the images the window is made of, element.c
// draws rectangles that are not meshes, and frame.c draws. The records the
// device is built of are in device_parts.h, included below the constants it
// reads; the device struct is here, and the calls between files are in
// device_calls.h, included at the end. Nothing outside render/src sees this.
//
// The split is by lifetime, not by subject. What is made once at startup and
// lives until shutdown is device.c's; what is thrown away and rebuilt every time
// the window changes size is target.c's and swapchain.c's; what happens between
// two presents is frame.c's. A reader chasing a leak or a resize bug knows which
// file to open from that sentence alone.
//
// NOTHING DRAWS INTO A SWAPCHAIN IMAGE. The scene goes into an offscreen colour
// image of the engine's own, one per frame slot, and copying that into the
// acquired swapchain image is a separate last step. The swapchain is therefore
// no longer where the resolution is decided — see resolution below.
//
// DEPTH RUNS BACKWARDS EVERYWHERE IN HERE, AND IT IS NOT AN OVERSIGHT. The near
// plane is 1.0, the far plane is 0.0, the buffer is cleared to 0 and the
// comparison is GREATER. Every tutorial does the opposite; CLAUDE.md says not to
// correct it, and the reason it is worth the confusion is that a float depth
// buffer has most of its precision near 0, which is where the far plane now is.
// VOE_RENDER_DEPTH_FORMAT and voe_3d_projection are the two places the
// convention is actually spelled out — the second one is in another folder now,
// because building a projection matrix is 3d's job and this folder only ever
// receives one.
//
// ONE OTHER THREAD MAY UPLOAD WHILE THE OWNER DRAWS FRAMES, AND THE GUARD IS
// WHAT MAKES THAT SAFE (ADR-0370 point 5). It excludes every call that creates,
// frees or writes a pool, the texture or shading table, a target or a descriptor
// set, or uses the queue outside a frame, from a frame open on another thread:
// a set may not change while a recorded frame binds it, so a guard per queue
// call would not do. Frame begin and end hold it for their own work too. The
// owner's own calls inside its own frame never wait. It is not fair: a waiter
// gets in between one frame's end and the next begin, or after the owner stops.
#pragma once

#include "loader.h"

#include <base/arena.h>
#include <math/float2.h>
#include <math/float4x4.h>
#include <platform/window.h>
#include <render/device.h>

#include <threads.h>

// The most images a swapchain here may have. Three or four on every driver
// measured, in both present modes; the number exists so that the per-image
// arrays are members and a resize allocates and frees nothing. A driver that
// wants more than this is refused with a message rather than quietly clamped —
// clamping would leave images we never made a view for and an acquire that
// returns an index we cannot draw to.
#define VOE_RENDER_MAX_IMAGES 8

// How many textures may exist at once, and the length of the array the fragment
// shader samples.
//
// IT IS FIXED BECAUSE THE SHADER'S ARRAY IS FIXED. A descriptor array is
// declared with a length in the layout and in the shader, and both have to agree
// with this number; growing it is changing the four places draw.slang names
// together, which is a card, not a runtime decision. 1024 is what a scene of
// models with up to four pictures each needs (0278), and card.c refuses a card
// whose descriptor limits are below it; it is not in
// voe_render_capacities with the other limits precisely because it is the
// shader's array length and not a size a caller may choose.
//
// EVERY SLOT ALWAYS HOLDS A VALID DESCRIPTOR, WHICH IS WHY THERE IS A DEFAULT
// TEXTURE. Vulkan requires every element of a descriptor array to be written
// before the set is used, whether or not the shader reads it, so the slots
// nothing has claimed point at a one-pixel white image. Sampling an unclaimed
// slot is then white rather than undefined, and there is no partially-bound
// extension to ask for.
#define VOE_RENDER_MAX_TEXTURES 1024

// How many voe_render_sampling values there are, which is how many samplers the
// device makes. Here and not in the public enum: a caller has no use for a count
// and rule 10 says a value nothing needs is not written.
//
// IT IS HAND-WRITTEN AND NOTHING DERIVES IT FROM THE ENUM, so this number and
// that enum change together or not at all. It sizes samplers[] below and the
// infos[] initialiser in texture.c, and both creation and destruction run to
// it: left behind, a designated initialiser for the new value writes past the
// end of an array.
#define VOE_RENDER_SAMPLING_COUNT 3

// How many frames the CPU may have submitted and unfinished at once, and the
// length of every per-slot array in this engine.
//
// THE NUMBER IS NOT THE DECISION. Read this constant everywhere and never assume
// its value: a literal 2 anywhere that means "frames in flight" is a bug, and
// going to three has to be this line and nothing else.
//
// IT IS NOT THE SWAPCHAIN IMAGE COUNT AND NEVER STANDS IN FOR IT. That is a
// number the driver chooses from the surface, for reasons that have nothing to
// do with how far ahead the CPU may run. They are often both 2 or 3 and that is
// a coincidence; the first driver that reports a different minimum breaks
// anything that leant on it. See voe_render_frame below for the half of the
// synchronisation that follows this constant, and voe_render_image for the half
// that follows the images.
#define VOE_RENDER_FRAMES_IN_FLIGHT 2

// The depth format, and it is not negotiable in the way a colour format is: this
// engine reverses depth, so it wants all the precision it can get near zero and
// a normalised integer format would spend that precision in the wrong place.
// D32_SFLOAT is required of every Vulkan implementation as a depth attachment,
// so there is nothing to query and nothing to fall back to.
#define VOE_RENDER_DEPTH_FORMAT VK_FORMAT_D32_SFLOAT

// The value the depth buffer is cleared to, which is the far plane, which is 0.
// Named because a literal 0.0f in a clear is indistinguishable from a literal
// 0.0f that means "nothing here yet", and one of those is load-bearing.
#define VOE_RENDER_DEPTH_CLEAR 0.0f

#include "device_parts.h"

// The new validation messages the device keeps to name again at close
// (best_practices.c): how many distinct id names, and the bytes each row keeps of
// an id name and of its first text, the terminator included. A longer one is cut.
#define VOE_RENDER_KEPT_MESSAGES 8
#define VOE_RENDER_KEPT_ID_BYTES 128
#define VOE_RENDER_KEPT_TEXT_BYTES 384

struct voe_render_device {
	VkInstance instance;
	VkDebugUtilsMessengerEXT messenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical;
	VkDevice device;
	VkQueue queue;
	uint32_t queue_family;

	// No window, no surface and no swapchain: the shape the offscreen test
	// runs on. Everything else is the same object, so this is read in the
	// four places where a surface would otherwise be asked a question —
	// which instance and device extensions to enable, which queue families
	// can present, and where the format comes from — and nowhere else.
	bool headless;

	// Chosen once, and kept. It is the surface's format where there is a
	// surface, because the last thing a frame does is copy into a swapchain
	// image; the targets take the same one.
	//
	// THE TARGET SHARING THE SWAPCHAIN'S FORMAT IS A STARTING POINT. What
	// makes tone mapping possible at all is a target of higher precision
	// than the screen, and that is the card that introduces it. Until then
	// one format is one fewer thing to convert and the blit is a straight
	// copy.
	//
	// IT IS AN sRGB FORMAT AND BOTH IMAGES BEING THAT IS THE POINT. The
	// frame is drawn in linear light, the target's format encodes it as it
	// stores it, and the blit into a swapchain image of the same format is
	// therefore a copy of already-encoded bytes. See PREFERRED_FORMAT in
	// device.c for why, and what happens on a surface that offers no such
	// format.
	VkSurfaceFormatKHR format;

	// The two pipelines, and the layout both of them need in order to exist.
	// Startup's, not the swapchain's: the viewport and the scissor are
	// dynamic state, so a resize changes none of these and there is nothing
	// here to rebuild.
	//
	// The layout names descriptor_layout below and the one push constant a
	// draw uses, and the probe's pipeline shares it too — which is why the
	// layout is the device's and not any one pipeline's private business.
	//
	// THE TWO DIFFER IN DEPTH WRITES AND IN BLENDING AND IN NOTHING ELSE.
	// `pipeline` writes depth and does not blend; `pipeline_blended` tests
	// depth, writes none, and blends premultiplied. device.c builds both from
	// one description so the rest cannot drift.
	//
	// THE THIRD ONE IS NOT A VARIANT OF THE OTHER TWO. `pipeline_elements`
	// draws element records rather than meshes: no vertex input state at
	// all, a triangle strip, nothing culled, and its own shader. It shares
	// `layout` — and therefore the descriptor set bound at the top of the
	// frame — which is the whole reason the push constant range below is big
	// enough for both. element.c builds it.
	VkPipelineLayout layout;
	VkPipeline pipeline;
	VkPipeline pipeline_blended;
	VkPipeline pipeline_elements;
	// The shadow pass's: the solid one's vertex stage and nothing after it
	// but a biased depth write. pipeline.c builds it with the other two.
	VkPipeline pipeline_shadow;
	// The point-shadow pass's: its own vertex stage writing the layer, then
	// the shadow one's biased depth write. VK_NULL_HANDLE without output_layer.
	VkPipeline pipeline_point_shadow;
	// The capture pass's: its own vertex stage writing the layer, a fragment
	// writing albedo and normal with distance, nothing culled, no bias.
	// VK_NULL_HANDLE without output_layer.
	VkPipeline pipeline_capture;
	// The relight's, bounce_relight.c's: its own set layout, layout, pool and
	// settle, relight and sum pipelines; per frame slot a mapped list buffer,
	// a mapped record buffer of a region per volume, each at its descriptor
	// index (voe_render_bounce_volume_index) ×
	// `relight_record_stride` (the record's size rounded up to the card's
	// uniform offset alignment, as `pass_stride` is), and one set per volume
	// (calloc'd); and how many dispatches it has recorded. All nought without
	// output_layer.
	VkDescriptorSetLayout relight_set_layout;
	VkPipelineLayout relight_layout;
	VkDescriptorPool relight_pool;
	VkPipeline relight_settle;
	VkPipeline relight_levels;
	VkPipeline relight_sum;
	struct voe_render_buffer relight_lists[VOE_RENDER_FRAMES_IN_FLIGHT];
	void *relight_mapped[VOE_RENDER_FRAMES_IN_FLIGHT];
	struct voe_render_buffer relight_records[VOE_RENDER_FRAMES_IN_FLIGHT];
	void *relight_records_mapped[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkDeviceSize relight_record_stride;
	VkDescriptorSet *relight_sets[VOE_RENDER_FRAMES_IN_FLIGHT];
	uint32_t relight_dispatches;
	// Whether the relight's startup has finished, which prepare does
	// (pipeline.c); until then the relight does nothing. Never true
	// without output_layer.
	bool relight_started;
	// Whether a prepare step failed: every later prepare answers FAILED.
	bool prepare_failed;
	// What every mesh pipeline build passes, pipeline_cache.c's: made empty at
	// open, replaced by a seeded one before the first prepare step, destroyed
	// at close.
	VkPipelineCache pipeline_cache;

	// How much room the caller asked for, kept because every _create below
	// compares against it and because a full pool has to say what it was
	// full of. capacities.shadow_size is the shadow maps' side, nought for
	// none, and is read from here.
	voe_render_capacities capacities;
	// Whether the card had shaderOutputLayer and create_device enabled it, and
	// the point shadow maps' side: capacities.point_shadow_size, or nought
	// when the feature is missing. device.c settles both; read this side, not
	// the capacity's.
	bool output_layer;
	uint32_t point_shadow_size;

	// The textures and the samplers they are read through: one per
	// voe_render_sampling, made once at startup and indexed by a slot's own
	// mode. Two, because a picture on a surface in the world and a sheet
	// something indexes into want opposite answers about mipmaps — see
	// voe_render_sampling in render/include/render/device.h.
	//
	// SLOT 0 IS THE ONE-PIXEL WHITE DEFAULT AND IS NEVER HANDED OUT. Every
	// element of the descriptor array has to be a valid descriptor whether
	// or not the shader samples it, so unclaimed slots point at slot 0 —
	// which is also what makes VOE_RENDER_NO_TEXTURE sample to white
	// instead of to something undefined.
	struct voe_render_texture_slot textures[VOE_RENDER_MAX_TEXTURES];
	VkSampler samplers[VOE_RENDER_SAMPLING_COUNT];
	// The anisotropy the SMOOTH sampler reads at (ADR-0369): min(8, the
	// card's maxSamplerAnisotropy) where create_device enabled
	// samplerAnisotropy, 1 where the card has none.
	float max_anisotropy;
	// The comparison sampler every slot's shadow maps are read through at
	// binding 5. shadow.c makes and destroys it.
	VkSampler shadow_sampler;
	// How many lights' maps every slot's shadow array holds, from 1, and how
	// many voe_render_shadow_lights_ready last asked for; shadow.c grows the
	// first to the second at the top of a frame and never shrinks it.
	uint32_t shadow_lights;
	uint32_t shadow_lights_wanted;
	// The linear, repeating sampler the probe volumes' sums are read through
	// at binding 6, repeat because a volume is addressed toroidally, and the
	// linear, clamping one their moments are read through at binding 10
	// (ADR-0326). descriptors.c makes and destroys both.
	VkSampler bounce_sampler;
	VkSampler moments_sampler;

	VkDescriptorSetLayout descriptor_layout;
	VkDescriptorPool descriptor_pool;

	// The geometry: one vertex pool, one index pool, both device-local and
	// both filled through a staging buffer that is gone before the frame
	// that uses them. Startup's, and untouched by a resize.
	//
	// geometries is capacities.geometries + capacities.transient_geometries
	// long and is calloc'd with the device rather than being a member,
	// because how many ranges a program may name is the caller's number and
	// not this header's. The static ranges are the first band and the
	// transient ones the band above it; the two never share a slot, and
	// voe_render_geometry_at checks against the total and nothing else.
	struct voe_render_pool vertices;
	struct voe_render_pool indices;
	struct voe_render_geometry_slot *geometries;

	// The shading records: one host-visible buffer the fragment stage reads,
	// written once per record by voe_render_shading_create, and one slot per
	// record on this side so a stale id can be refused. Shared by every
	// frame slot, because nothing rewrites a record after it is made.
	struct voe_render_buffer shadings;
	void *shadings_mapped;
	struct voe_render_shading_slot *shading_slots;

	// What voe_render_frame_begin left for _draw and _end: whether a
	// recording is open, how many objects have been drawn into it, and which
	// swapchain image _end has to blit into and present.
	//
	// THEY LIVE HERE AND NOT IN A TOKEN THE CALLER HOLDS because there is
	// exactly one frame open at a time and a token would be a second place
	// for that fact to live. The asserts in frame.c read `recording` and
	// nothing else needs to.
	bool recording;
	uint32_t object_count;
	uint32_t image_index;

	// How many elements have been submitted to the open frame, which is both
	// where the next record goes and the instance count the one element draw
	// uses. Reset by _begin beside object_count and meaningless while
	// `recording` is false.
	uint32_t element_count;

	// The pass: whether one is open, whether it was opened with a camera,
	// and how many this frame has opened, which is also which block of the
	// slot's uniform buffer the next one writes. All three reset by _begin.
	//
	// `window_cleared` IS THE CLEAR RULE. False until the first pass onto the
	// window in a frame, which clears it; every pass after loads. A frame
	// that ends with it still false records one empty clearing rendering, so
	// what is presented is the clear colour and not last lap's picture.
	bool pass_open;
	bool pass_camera;
	uint32_t pass_count;
	bool window_cleared;

	// What the open pass draws into: NULL for the window's target, or the
	// target slot it named — and the size of whichever it is, which is the
	// render area, the viewport, the scissor and the depth clear's rect.
	// Meaningless while `pass_open` is false.
	struct voe_render_target_slot *pass_target;
	VkExtent2D pass_extent;

	// Whether the open pass is a shadow pass, and onto which layer: the
	// draws read the first to pick the shadow pipeline, and _pass_end both to
	// hand that layer back to the shader. Meaningless while `pass_open` is
	// false.
	bool pass_shadow;
	uint32_t pass_layer;
	// Whether the open pass is the point-shadow pass (ADR-0325): the draws read
	// it to pick that pipeline and cull by face, _pass_end to hand the maps
	// back. `pass_casters` is its lights by slot − 1, `pass_slots` bit s − 1
	// for each slot a light holds.
	bool pass_point_shadow;
	voe_render_point_light pass_casters[VOE_RENDER_POINT_SHADOWS];
	uint32_t pass_slots;
	// Whether the open pass is a capture pass (ADR-0326 point 3): the draws
	// read it to pick that pipeline and cull by face over `pass_casters`, its
	// probes by slot; _pass_end to copy `capture_count` probes, toroidal
	// indices in `capture_probes` by slot, into `capture_volume`'s atlases.
	// `capture_passes` counts this frame's, reset by voe_render_frame_begin.
	bool pass_capture;
	struct voe_render_bounce_volume *capture_volume;
	uint32_t capture_probes[VOE_RENDER_BOUNCE_CAPTURE];
	uint32_t capture_count;
	uint32_t capture_passes;
	// Whether the open pass is the bounce shadow pass (ADR-0329): the draws
	// read it to take the shadow pass's path, _pass_end to hand the map back.
	// `pass_bounce_layer` is the layer of the map the open pass's sun draws on.
	bool pass_bounce_shadow;
	uint32_t pass_bounce_layer;

	// The targets of the caller's own: capacities.targets of them, calloc'd
	// with the device like `geometries` and NULL when that is nought. A slot is
	// live from the create that claimed it until the device closes.
	struct voe_render_target_slot *targets;

	// The texture slot that shows the window's depth copy, each frame slot
	// its own; claimed by the first voe_render_target_build, never freed.
	uint32_t window_depth_texture;
	// The window's probe volumes, each built on the first begin onto it.
	struct voe_render_bounce_volume window_volume[VOE_RENDER_BOUNCE_VOLUMES];

	// This frame's last voe_render_bounce_begin, bounce_volume.c's: whether
	// there was one, its target and volume, and a copy of its record whose `stale` is
	// none and whose `points` are `bounce_lamps`, the bouncing lamps, and
	// `blockers` `bounce_blockers`, and `more` `bounce_suns`, the bouncing
	// further suns — none of any while the target's volume is not built.
	// Reset by voe_render_frame_begin.
	bool bounce_begun;
	voe_render_target bounce_target;
	uint32_t bounce_volume;
	struct voe_render_bounce_frame bounce_frame;
	voe_render_point_light bounce_lamps[VOE_RENDER_BOUNCE_LAMPS];
	voe_render_light_blocker bounce_blockers[VOE_RENDER_LIGHT_BLOCKERS];
	voe_render_directional_light bounce_suns[VOE_RENDER_DIRECTIONAL_LIGHTS - 1];
	// The begin's suns, 1 + its `more.count`, and the relight sun map layer
	// of each (ADR-0357 point 4): sun 0's 0, a further sun's its place in
	// `bounce_suns` + 1, VOE_RENDER_NO_SUN_LAYER for one that does not bounce.
	uint32_t bounce_sun_count;
	uint32_t bounce_sun_layers[VOE_RENDER_DIRECTIONAL_LIGHTS];

	// How far apart the per-pass blocks are in a slot's uniform buffer: the
	// block's size rounded up to minUniformBufferOffsetAlignment, because a
	// dynamic offset that is not a multiple of it is invalid. Chosen by
	// descriptors.c when it sizes the buffers.
	VkDeviceSize pass_stride;

	// How many draw commands the open recording holds, and after _end how
	// many the frame just submitted held. Reset by _begin. It is what
	// voe_render_frame_draw_count hands back, and it exists so that "a whole
	// interface in one draw" is a number somebody read rather than a claim
	// somebody made.
	uint32_t draw_commands;

	// Which of the two pipelines the open recording last bound, so that a
	// run of draws of one kind costs one bind and not one per draw. Set when
	// the rendering opens and every time a draw needs the other one;
	// meaningless while `recording` is false.
	VkPipeline bound;

	// Whether the open pass has bound a pool pair yet: false as a pass opens,
	// set by its first mesh draw (draw.c), so a pass of elements alone binds
	// no vertex buffer (ADR-0385). Meaningless while `recording` is false.
	bool pools_bound;
	// Which geometry pools the open recording last bound: false for the
	// static pair, true for this slot's transient pair. The same shape as
	// `bound` and for the same reason — a run of draws out of one pool costs
	// one bind, and a caller that interleaves the two is still drawn right.
	// Read only once `pools_bound` is true.
	bool bound_transient;

	// The size every slot's target is, and the resolution the engine draws
	// at. It is the window's size today and it is not the swapchain's: what
	// reconciles the two is the blit at the end of a frame, which scales.
	// That is what makes rendering at a different resolution from the window
	// a change to this one number later on.
	VkExtent2D resolution;

	VkSwapchainKHR swapchain;
	VkExtent2D extent;

	// The window size the targets and the swapchain above were built for.
	// Compared against the size handed to every frame, and it is not always
	// the same number as the swapchain's extent: where a surface dictates
	// its own extent, the extent is the surface's answer and this is the
	// question that was asked. Keeping both is what stops a clamped size
	// from rebuilding on every frame forever.
	voe_platform_size built;
	uint32_t image_count;
	struct voe_render_image images[VOE_RENDER_MAX_IMAGES];

	// The one-shot uploads' pool: their command buffers are allocated, run
	// once and freed, and each frame slot has a pool of its own (ADR-0385).
	// Made without the reset flag, because nothing in it is reset alone. The
	// guard (ADR-0370) still keeps another thread's upload apart from an
	// open frame.
	//
	// The frame slots: startup's, apart from the target inside each one,
	// which a resize rebuilds. slot is the one the next frame will use,
	// advanced modulo the constant the moment a submit succeeds — because a
	// submit is what puts a slot in flight, and a frame that returns before
	// submitting must come back to the same slot with its fence still
	// signalled.
	VkCommandPool pool;
	struct voe_render_frame frames[VOE_RENDER_FRAMES_IN_FLIGHT];
	uint32_t slot;

	// Whether a frame has ended since the window's targets were built, which
	// is what says the slot before `slot` holds a window picture that may be
	// copied out of. voe_render_frame_end leaves that slot's colour image in
	// TRANSFER_SRC_OPTIMAL; a freshly built one is in UNDEFINED, and a copy
	// naming the wrong layout is a validation error rather than merely an
	// undefined picture. voe_render_target_read is the only reader, and
	// voe_render_target_teardown — which every resize goes through — is what
	// puts it back to false.
	bool frame_ended;

	// Set when a present said the swapchain no longer matches the surface.
	// The rebuild happens at the top of the next frame rather than here,
	// because the images the present is still reading are not ours to
	// destroy until the queue is idle.
	bool rebuild;

	// ---- how a frame reaches the display. See voe_render_present.

	// Whether this surface offers MAILBOX. Asked once at startup, beside the
	// format and for the same reason: it is a property of a surface and a
	// physical device, and neither of those changes when the window is
	// resized. False on a headless device, which has no surface to ask.
	bool mailbox_offered;

	// What was asked for, and what the swapchain that exists was actually
	// built with. They differ between a _set and the rebuild that acts on
	// it, and they differ for ever on a surface with no mailbox — which is
	// why a caller is told the second one and not the first.
	voe_render_present present_wanted;
	voe_render_present present_in_force;

	// ---- what the card's own clock said. See voe_render_frame_gpu_time.

	// Whether timestamps can be written at all: true when the card reports a
	// period and the queue family this device took reports valid bits.
	// Everything else in this group is meaningless when it is false, and no
	// query pool is created.
	bool timestamps;

	// Nanoseconds per tick, from VkPhysicalDeviceLimits, and how many of a
	// timestamp's bits actually carry a value.
	//
	// THE VALID BITS ARE NOT ALWAYS 64 AND THAT IS NOT A CURIOSITY. A queue
	// is allowed to report as few as 36, and the bits above that hold
	// rubbish rather than zeroes — so a raw subtraction of two readings can
	// come out enormous or negative. frame.c masks both readings down to
	// these bits and treats an end below a start as one wrap of that
	// counter, which is the only reading of it that is right.
	float timestamp_period;
	uint32_t timestamp_valid_bits;

	// The newest measurement there is, in seconds, and whether there has
	// been one. Written at the top of a frame, out of the pool belonging to
	// the slot that frame is about to reuse — so it describes the frame
	// VOE_RENDER_FRAMES_IN_FLIGHT back and not the last one.
	double gpu_seconds;
	bool gpu_measured;
	// The same frame's passes, read beside it: voe_render_timed_passes
	// entries calloc'd at open, the first `pass_time_count` of them filled.
	voe_render_pass_time *pass_times;
	uint32_t pass_time_count;

	// ---- the Best Practices checks (ADR-0367 points 4 to 7).

	// Whether instance.c turned them on, or found a debug build without the
	// layer (never both; a release build is neither), and whether the four
	// vendor sets came with them. The chosen card's PCI vendor id, card.c's.
	// How many messages best_practices.c counted as new.
	bool checks_on;
	bool checks_missing;
	bool vendor_checks;
	uint32_t vendor_id;
	uint32_t new_messages;
	// What those new messages were, one row per distinct id name, a row free
	// while its count is nought: the id name ("(no id)" for none), whether
	// one was an error, how many came, and the first one's text. Then how
	// many new messages found no free row. Fixed and in the struct because a
	// messenger callback must not allocate or fail; written where
	// new_messages is counted, so it shares that counter's threading; read
	// once, at close.
	struct {
		char id_name[VOE_RENDER_KEPT_ID_BYTES];
		bool error;
		uint32_t count;
		char text[VOE_RENDER_KEPT_TEXT_BYTES];
	} kept[VOE_RENDER_KEPT_MESSAGES];
	uint32_t kept_overflow;

	// ---- the guard; see the top of this file.

	// Held by voe_render_device_guard_take to _give. `frame_open` is true
	// from a begin that answered drawing to its end, the frame opened on
	// `frame_thread`; a take on any other thread waits on `frame_closed`
	// while it is. Both are written only with `guard` held.
	mtx_t guard;
	cnd_t frame_closed;
	bool frame_open;
	thrd_t frame_thread;
};

#include "device_calls.h"
