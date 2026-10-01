// The records voe_render_device is built of, each one a part of something
// larger: the pass block, the buffers and pools and the geometry slots are
// geometry.c's and descriptors.c's; the shading and texture slots are
// shading.c's and texture.c's; the shadow map is shadow.c's; the swapchain
// image is swapchain.c's; the
// allocated image, the target and the target slot are target.c's; and
// voe_render_frame is one frame slot, frame.c's, holding a target and buffers.
//
// Only the records live here. The device struct, the constants the whole folder
// reads and the calls between files stay in device_internal.h, which is the one
// file that includes this one, after it has defined VOE_RENDER_FRAMES_IN_FLIGHT
// that the target slot and the frame are sized by. Include device_internal.h,
// never this. Nothing outside render/src sees either.
#pragma once

#include "loader.h"

#include <render/device.h>

// Everything one pass is drawn with that is not a per-object record: the camera,
// the sun and the sun's shadow record, in one block. There is one block per pass per frame slot, all of
// them in the slot's one uniform buffer, `pass_stride` bytes apart.
//
// IT IS ONE BLOCK AND NOT TWO BINDINGS BECAUSE THEY HAVE THE SAME LIFETIME.
// Both are written once by voe_render_pass_begin and read by both stages for
// every draw in the pass, so splitting them would be a second buffer, a second
// descriptor and a second pool entry to say what one memcpy says. It is internal
// only in name: voe_render_pass_camera is the same three members in the same
// order, and a pass's block is a copy of it.
//
// ONE BUFFER AND A DYNAMIC OFFSET, NOT A SET PER PASS. Binding 0 is a dynamic
// uniform buffer, so opening a pass binds the slot's one set with the offset of
// that pass's block. A set per pass would be `passes` copies of the texture
// array's 1024 descriptors, every one rewritten whenever a texture is made.
//
// draw.slang declares the same three structs in the same order at binding 0.
// descriptors.c asserts on the sizes and the offsets, so a member that moves is
// a build error rather than a frame lit from the wrong direction.
struct voe_render_frame_block {
	voe_render_view camera;
	voe_render_light light;
	voe_render_shadow shadow;
};

// A buffer and the memory under it, which in this engine are always made and
// thrown away together. One allocation per buffer, exactly as target.c makes one
// per image, and the same note applies: an engine that made many of these would
// sub-allocate out of a few large blocks instead. This one makes four.
struct voe_render_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
};

// A range given back to a static pool: `count` elements from `offset`.
struct voe_render_free_range {
	uint32_t offset;
	uint32_t count;
};

// A pool: one buffer, how much of it is spent, and the ranges given back.
//
// TWO OF THESE ARE THE WHOLE OF THIS ENGINE'S GEOMETRY. Every mesh's vertices go
// into one pool and every mesh's indices into another, so a mesh is a range and
// not a buffer of its own — which is what lets one bind serve every draw in a
// frame, and what lets many draws become one indirect call in a later card.
//
// `used` IS THE HIGH-WATER MARK AND `holes` THE RANGES UNDER IT (0278). The list is
// sorted by offset, no two entries touch, and none touches `used` — a range
// given back there lowers `used` instead. So there are never more holes than
// live ranges, and `hole_room` is the static band's slot count. There is no
// compaction: a pool whose holes are too small is a returned failure. A
// transient pool has no list; it is emptied whole every frame.
struct voe_render_pool {
	struct voe_render_buffer buffer;
	// Elements, not bytes: vertices in one pool and indices in the other.
	uint32_t capacity;
	uint32_t used;
	struct voe_render_free_range *holes;
	uint32_t hole_count;
	uint32_t hole_room;
};

// A transient pool: the same bookkeeping over a host-visible buffer that stays
// mapped, one per frame slot, filled from the top every frame. See
// voe_render_geometry_create_transient for what it is for.
//
// `used` GOES BACK TO NOUGHT AT THE TOP OF EVERY FRAME, and that is the whole
// difference from the pool above. The fence voe_render_frame_begin waits on says
// the card has finished reading what this slot held two frames ago, so emptying
// and rewriting it needs no barrier and no wait of its own — the same reason the
// object buffer beside it in voe_render_frame is safe to overwrite. A barrier
// appearing here would mean the pools had stopped being per slot, and that would
// be the bug.
struct voe_render_transient_pool {
	struct voe_render_pool pool;
	// Where pool.buffer's memory is mapped, for the lifetime of the buffer.
	// Written through and never read back: host-visible memory may be
	// write-combined, and a read out of it crawls.
	void *mapped;
};

// What a voe_render_geometry id names: where in the two pools a mesh's vertices
// and indices sit, and how many indices to draw.
//
// first_vertex IS ADDED BY THE DRAW AND NOT BY THE UPLOAD. The indices are
// stored exactly as the caller numbered them, from zero, and vkCmdDrawIndexed's
// vertexOffset is what shifts them into the pool — so a mesh's indices never
// have to be rewritten because something was uploaded before it.
//
// index_count IS THE DRAW'S COUNT AND NOT A SIZE IN BYTES. Two numbers that
// differ by a factor of four, and reading one for the other draws either a
// quarter of the mesh or four times off the end of it.
//
// `transient` SAYS WHICH POOL THE RANGE IS IN, because a caller's id does not.
// The static pair is device-local and shared by every frame; the transient pair
// is host-visible and this frame slot's own. The draw reads the flag to know
// which buffers have to be bound before the range means anything, and it is the
// only place in the engine that distinguishes the two kinds.
//
// `live` FALSE IS A FREE SLOT, and vertex_count is kept only so a destroy knows
// how much of the vertex pool to give back.
struct voe_render_geometry_slot {
	uint32_t first_vertex;
	uint32_t first_index;
	uint32_t index_count;
	uint32_t vertex_count;
	uint32_t generation;
	bool live;
	bool transient;
};

// What a voe_render_shading id names. The record itself lives in the GPU buffer
// the fragment stage reads; this is only what the CPU needs in order to refuse a
// stale id, which is the same shape a texture slot has and for the same reason.
// `live` false is a free slot; a destroy and a create each bump the generation.
struct voe_render_shading_slot {
	uint32_t generation;
	bool live;
};

// One texture: the image, its memory, its view, and the generation that says
// which texture it is. See texture.c.
//
// `generation` COUNTS UP AND NEVER RESETS, WHICH IS THE WHOLE POINT OF A
// GENERATIONAL ID. A slot that is freed and claimed again is a different texture
// living at the same index, and an id handed out before that still names the
// index. Comparing the generation is what tells the two apart, so a stale id is
// refused rather than silently drawing whatever moved in.
struct voe_render_texture_slot {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	// The format this slot's image and view were made with, which is what
	// says whether the hardware decodes sRGB on a read. It is kept because
	// the mipmap chain has to ask the card whether it can filter this format
	// linearly, and the two textures kinds are two different formats.
	VkFormat format;
	// Which of the device's samplers this slot is read through. Kept because
	// the descriptor write happens again every time the table changes and
	// has to name the same sampler each time; a slot remembers its mode and
	// the descriptor path stays one loop.
	voe_render_sampling sampling;
	uint32_t generation;
	bool live;
	// Whether this slot is a target's picture rather than a texture of its
	// own, and which of device->targets it is. Such a slot holds no image,
	// memory or view — all three stay VK_NULL_HANDLE — because the target has
	// one image per frame slot and the descriptor write picks the frame
	// slot's. See voe_render_texture_write_descriptors.
	bool is_target;
	uint32_t target;
};

// One swapchain image and the two things that belong to it for its whole life.
//
// drawn is per image and not per frame on purpose. vkQueuePresentKHR waits on it
// and there is no fence to say when that wait finished, so the only safe moment
// to reuse it is when the image it belongs to comes back out of an acquire —
// which is exactly when this one does.
struct voe_render_image {
	VkImage image;
	VkImageView view;
	VkSemaphore drawn;
};

// An image the engine allocated, with the memory under it and a view onto it.
// The three always arrive together and always go away together, which is the
// whole reason they are one struct: target.c builds two of these and the code
// that builds either one is the same code.
//
// IT IS ALSO WHAT KEEPS target.c CLEAR OF `VkImage *`. Every Vulkan handle is a
// pointer behind a typedef, so a helper taking one out by address would be the
// double dereference rule 6 forbids and specifically forbids hiding behind a
// typedef. Handing the helper this struct instead is one level, and the only
// place a handle's address is taken is the Vulkan call that demands it — which
// guidelines.md allows, because conforming to the library is the exception.
struct voe_render_allocated_image {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

// What one frame slot draws into: a colour image and a depth image. This is what
// the engine renders to; the swapchain image is only where the colour half is
// copied at the end, and the depth half never leaves the graphics card at all.
//
// NEITHER HALF IS THE DEFAULT ONE. Colour was the only image here until solid
// arrived, and leaving it unqualified would have left `image` meaning one of two
// images. Both are named and neither is implied.
//
// There is no extent in here because there is only ever one: every slot's target
// is built at the same size, and that size is device->resolution below. Two
// copies of one number is two chances for them to disagree.
struct voe_render_target {
	struct voe_render_allocated_image colour;
	struct voe_render_allocated_image depth;
};

// One frame slot's shadow map, shadow.c's: one D32 image of
// VOE_RENDER_SHADOW_CASCADES layers, the memory under it, a view of every layer
// for the shader to read, and a view of each layer for a shadow pass to draw into.
// Rests in SHADER_READ_ONLY_OPTIMAL outside a shadow pass.
struct voe_render_shadow_map {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView array;
	VkImageView layers[VOE_RENDER_SHADOW_CASCADES];
};

// What a voe_render_target id names: a target of the caller's own, which is the
// window's pair above made once per frame slot at a size of its own, plus the
// texture slot that shows it. See target_own.c.
//
// ITS COLOUR IMAGES ARE IN GENERAL AND THE WINDOW'S ARE NOT. A target's colour
// image is an attachment and a sampled image at once, and target_own.c says why it is
// given the one layout valid for both rather than moved between two. The depth
// images stay in their attachment layout once a pass has put them there, as the
// window's do.
struct voe_render_target_slot {
	struct voe_render_target images[VOE_RENDER_FRAMES_IN_FLIGHT];
	// The size every one of `images` is built at, and the size a resize has
	// asked for. They differ from voe_render_target_resize until the top of
	// the next frame, which builds `wanted` and makes the two agree.
	VkExtent2D extent;
	VkExtent2D wanted;
	// Which of device->textures shows this target. Never changes.
	uint32_t texture;
	uint32_t generation;
	bool live;
	// The clear rule: false until the first pass onto this target in a
	// frame, which clears it. Reset by voe_render_frame_begin.
	bool cleared;
};

// Everything with a one-frame lifetime, in one struct, one per frame slot. This
// is the shape: a per-frame resource — a uniform buffer, a descriptor set, a
// staging buffer — becomes a field here and is reached through the slot, and it
// needs no new array and no new index. The target below is the first thing to
// arrive that way.
//
// THE TWO SEMAPHORE KINDS HAVE DIFFERENT LIFETIMES AND MUST NOT BE FLATTENED
// INTO ONE. acquired is here, per slot, because the fence beside it is what says
// the submit that last waited on it has finished — that is the only thing that
// makes it safe to hand to another acquire. drawn is not here: it is per image,
// on voe_render_image above, because present is what waits on it and present
// hands back no fence to say when it stopped. Moving either one to the other's
// array is a race the validation layers do not reliably catch — an intermittent
// hang on one driver and never on the machine it was written on.
//
// submitted is the fence for this slot's last submit, and waiting on it at the
// top of a frame is waiting for the frame VOE_RENDER_FRAMES_IN_FLIGHT ago, not
// the previous one. That gap is the whole of the overlap.
//
// The target is per slot for exactly the reason the command buffer is: the GPU
// may still be reading the frame before last, so a single target shared by every
// slot would be written by one frame while another was still blitting it.
//
// THE UNIFORM BUFFER IS PER SLOT FOR THAT SAME REASON, AND ONE SHARED BUFFER
// WOULD BE A RACE RATHER THAN A SAVING. The matrices are written by the CPU at
// the top of a frame and read by the GPU some time later; a single buffer would
// be rewritten while a frame still in flight was reading it, and the symptom is
// one frame drawn with another frame's camera — occasional, invisible on a still
// image, and impossible to attribute. The fence at the top of the frame is what
// makes this slot's buffer safe to write, and it guards nothing else's.
//
// IT STAYS MAPPED FOR ITS WHOLE LIFE. Uniforms are host-visible and coherent and
// are written every frame, so mapping and unmapping around each write would be
// two driver calls to say what one pointer already says. The pointer is kept
// here; there is no second place that knows it.
struct voe_render_frame {
	VkCommandBuffer commands;
	VkSemaphore acquired;
	VkFence submitted;
	struct voe_render_target target;

	// The sun's cascades, per slot for the reason the target is: the card
	// may still be reading the frame before last's. Startup's; a resize
	// leaves them alone.
	struct voe_render_shadow_map shadow;

	// capacities.passes blocks, device->pass_stride bytes apart — the
	// stride is the block rounded up to the card's uniform offset alignment.
	struct voe_render_buffer uniforms;
	// Where uniforms.memory is mapped, for the lifetime of the buffer.
	// Written through as struct voe_render_frame_block, one per pass, and
	// never read back.
	void *uniforms_mapped;

	// One record per drawn object, this slot's own, written by
	// voe_render_frame_draw as it records. Per slot for exactly the reason
	// the uniform buffer is: the GPU may still be reading the last frame's
	// records, and the fence at the top of the frame is what says it has
	// finished with this slot's.
	struct voe_render_buffer objects;
	void *objects_mapped;

	// One record per submitted element, this slot's own, written by
	// voe_render_frame_submit_element. Per slot and safe to overwrite for
	// exactly the reasons the object buffer above is.
	//
	// IT IS BUILT EVEN WHEN THE DEVICE ASKED FOR NO ELEMENTS, with room for
	// one, and that is deliberate. Vulkan wants every descriptor a set's
	// layout declares to be a valid one; a slot with no buffer would leave
	// binding 4 unwritten, and the alternative to eighty wasted bytes is a
	// conditional descriptor and a rule about when the set may be used. The
	// capacity is what refuses a submit, not whether the buffer exists.
	struct voe_render_buffer elements;
	void *elements_mapped;

	// This slot's transient geometry: the vertex pool and the index pool
	// that voe_render_geometry_create_transient writes and the frame then
	// draws from. Per slot for exactly the reason the object buffer is —
	// the card may still be reading last lap's — and emptied at the top of
	// the frame by voe_render_geometry_frame_reset, once the same fence has
	// said the card is done with them.
	//
	// UNBUILT WHEN THE DEVICE ASKED FOR NO TRANSIENT ROOM: a zeroed struct
	// with a NULL mapped pointer and a capacity of nought. geometry.c looks
	// at the capacity before it touches either.
	struct voe_render_transient_pool transient_vertices;
	struct voe_render_transient_pool transient_indices;

	// Points at this slot's uniform buffer — a dynamic binding, so every bind
	// of it names a pass's offset — this slot's object buffer, the
	// texture array and the shared shading buffer. Allocated from
	// device->descriptor_pool and freed with it; a set is not destroyed on
	// its own anywhere in here.
	VkDescriptorSet descriptor;

	// Two timestamps: one written as this slot's command buffer starts and
	// one as it finishes, which is how long the card spent on that frame.
	// Per slot for exactly the reason everything else here is — the pool is
	// written by the card while the frame is running, and reading it is only
	// safe once this slot's fence says the card has finished.
	//
	// VK_NULL_HANDLE ON A CARD THAT CANNOT WRITE TIMESTAMPS. Nothing then
	// resets it, nothing writes into it and nothing reads it; see
	// device->timestamps.
	VkQueryPool timestamps;

	// Whether this slot's pool holds a pair worth reading. False until the
	// slot has been submitted once, because a pool that has never been
	// written to has nothing in it and asking would report VK_NOT_READY on
	// every frame of the first lap round the slots.
	bool timed;
};
