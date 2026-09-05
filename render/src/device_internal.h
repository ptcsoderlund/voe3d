// The innards of voe_render_device, shared by the files that make one: device.c
// starts it, descriptors.c builds what the shader reads, geometry.c and
// shading.c hold what a caller uploads, target.c makes the images the scene is
// drawn into, swapchain.c builds the images the window is made of, and frame.c
// draws. Nothing outside render/src sees this.
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
#pragma once

#include "loader.h"

#include <base/arena.h>
#include <math/float2.h>
#include <math/float4x4.h>
#include <platform/window.h>
#include <render/device.h>

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
// with this number; growing it is changing all three together, which is a card,
// not a runtime decision. Sixty-four is what a model with a handful of pictures
// on it needs, with room for the next one; it is not in
// voe_render_capacities with the other limits precisely because it is the
// shader's array length and not a size a caller may choose.
//
// EVERY SLOT ALWAYS HOLDS A VALID DESCRIPTOR, WHICH IS WHY THERE IS A DEFAULT
// TEXTURE. Vulkan requires every element of a descriptor array to be written
// before the set is used, whether or not the shader reads it, so the slots
// nothing has claimed point at a one-pixel white image. Sampling an unclaimed
// slot is then white rather than undefined, and there is no partially-bound
// extension to ask for.
#define VOE_RENDER_MAX_TEXTURES 64

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

// Everything one frame is drawn with that is not a per-object record: the camera
// and the sun, in one block, in one uniform buffer per frame slot.
//
// IT IS ONE BLOCK AND NOT TWO BINDINGS BECAUSE THEY HAVE THE SAME LIFETIME.
// Both are written once by voe_render_frame_begin and read by both stages for
// every draw in the frame, so splitting them would be a second buffer, a second
// descriptor and a second pool entry to say what one memcpy says. It is internal
// because the two halves are what a caller hands over separately; only this
// folder cares that they end up adjacent.
//
// draw.slang declares the same two structs in the same order at binding 0.
// descriptors.c asserts on the sizes and the offsets, so a member that moves is
// a build error rather than a frame lit from the wrong direction.
struct voe_render_frame_block {
	voe_render_view camera;
	voe_render_light light;
};

// A buffer and the memory under it, which in this engine are always made and
// thrown away together. One allocation per buffer, exactly as target.c makes one
// per image, and the same note applies: an engine that made many of these would
// sub-allocate out of a few large blocks instead. This one makes four.
struct voe_render_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
};

// A pool: one buffer, appended to and never freed, and how much of it is spent.
//
// TWO OF THESE ARE THE WHOLE OF THIS ENGINE'S GEOMETRY. Every mesh's vertices go
// into one pool and every mesh's indices into another, so a mesh is a range and
// not a buffer of its own — which is what lets one bind serve every draw in a
// frame, and what lets many draws become one indirect call in a later card.
//
// `used` ONLY EVER GOES UP. Nothing in this engine unloads anything yet, so there
// is no free list and no compaction here; the card that unloads a model is the
// card that decides what to do about the hole it leaves. Until then a pool that
// fills up is a returned failure and not a wait.
struct voe_render_pool {
	struct voe_render_buffer buffer;
	// Elements, not bytes: vertices in one pool and indices in the other.
	uint32_t capacity;
	uint32_t used;
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
struct voe_render_geometry_slot {
	uint32_t first_vertex;
	uint32_t first_index;
	uint32_t index_count;
	uint32_t generation;
	bool live;
};

// What a voe_render_shading id names. The record itself lives in the GPU buffer
// the fragment stage reads; this is only what the CPU needs in order to refuse a
// stale id, which is the same shape a texture slot has and for the same reason.
//
// THERE IS NO DESTROY FOR ONE, so a generation here never moves past 1 today. It
// is kept because the id type has one and because the card that starts unloading
// materials should not have to change the id everything else stores.
struct voe_render_shading_slot {
	uint32_t generation;
	bool live;
};

// One swapchain image and the two things that belong to it for its whole life.
//
// drawn is per image and not per frame on purpose. vkQueuePresentKHR waits on it
// and there is no fence to say when that wait finished, so the only safe moment
// to reuse it is when the image it belongs to comes back out of an acquire —
// which is exactly when this one does.
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
	uint32_t generation;
	bool live;
};

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

	struct voe_render_buffer uniforms;
	// Where uniforms.memory is mapped, for the lifetime of the buffer.
	// Written through as a struct voe_render_frame_block and never read
	// back.
	void *uniforms_mapped;

	// One record per drawn object, this slot's own, written by
	// voe_render_frame_draw as it records. Per slot for exactly the reason
	// the uniform buffer is: the GPU may still be reading the last frame's
	// records, and the fence at the top of the frame is what says it has
	// finished with this slot's.
	struct voe_render_buffer objects;
	void *objects_mapped;

	// Points at this slot's uniform buffer, this slot's object buffer, the
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

	// The one pipeline, and the layout it needs in order to exist.
	// Startup's, not the swapchain's: the viewport and the scissor are
	// dynamic state, so a resize changes neither of these and there is
	// nothing here to rebuild.
	//
	// The layout names descriptor_layout below and the one push constant a
	// draw uses, and the probe's pipeline shares it — which is why the
	// layout is the device's and not the pipeline's private business.
	VkPipelineLayout layout;
	VkPipeline pipeline;

	// How much room the caller asked for, kept because every _create below
	// compares against it and because a full pool has to say what it was
	// full of.
	voe_render_capacities capacities;

	// The textures and the one sampler they are all read through. One
	// sampler because nothing yet wants two filtering rules; the card that
	// wants point sampling is the card that makes this an array as well.
	//
	// SLOT 0 IS THE ONE-PIXEL WHITE DEFAULT AND IS NEVER HANDED OUT. Every
	// element of the descriptor array has to be a valid descriptor whether
	// or not the shader samples it, so unclaimed slots point at slot 0 —
	// which is also what makes VOE_RENDER_NO_TEXTURE sample to white
	// instead of to something undefined.
	struct voe_render_texture_slot textures[VOE_RENDER_MAX_TEXTURES];
	VkSampler sampler;

	VkDescriptorSetLayout descriptor_layout;
	VkDescriptorPool descriptor_pool;

	// The geometry: one vertex pool, one index pool, both device-local and
	// both filled through a staging buffer that is gone before the frame
	// that uses them. Startup's, and untouched by a resize.
	//
	// geometries is capacities.geometries long and is calloc'd with the
	// device rather than being a member, because how many ranges a program
	// may name is the caller's number and not this header's.
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

	// The frame slots, and the one pool every command buffer in them comes
	// out of. Startup's, apart from the target inside each one, which a
	// resize rebuilds.
	//
	// slot is the one the next frame will use, advanced modulo the constant
	// the moment a submit succeeds — because a submit is what puts a slot in
	// flight, and a frame that returns before submitting must come back to
	// the same slot with its fence still signalled.
	VkCommandPool pool;
	struct voe_render_frame frames[VOE_RENDER_FRAMES_IN_FLIGHT];
	uint32_t slot;

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
};

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

// target.c, and used by render/tests/offscreen.c as well: the index of a memory
// type this card offers that is in mask and has every one of properties.
// UINT32_MAX when there is none — which is the driver's answer and so is
// reported by the caller, not asserted on here.
[[nodiscard]] uint32_t voe_render_memory_type(const voe_render_device *device,
					      uint32_t mask,
					      VkMemoryPropertyFlags properties);

// frame.c. The engine's viewport for a target of this size, and the one Y flip
// in the engine — read frame.c's header before touching it.
VkViewport voe_render_frame_viewport(VkExtent2D extent);

// frame.c. The slot the next frame will use, and the one the recording that is
// open belongs to. Reading it is how the tests get at a frame's target and its
// fence without indexing an array they would have to bound-check themselves.
const struct voe_render_frame *voe_render_frame_current(const voe_render_device *device);

// frame.c. Which viewport the open recording was begun with, so that a test can
// hand in its mirror image. The mirror is what gets a back face in front of the
// rasteriser without a second shader and without touching the pipeline whose
// front-face constant is the thing under test: the same geometry drawn through a
// mirrored viewport is wound the other way round in framebuffer space, so every
// face the engine would cull is drawn and every face it would draw is culled.
//
// SETTING IT IS A COMMAND AND NOT A STATE CHANGE. The viewport is dynamic state,
// so this records a vkCmdSetViewport into the open recording and everything
// drawn after it uses the new one. Asserts if no recording is open.
void voe_render_frame_set_viewport(voe_render_device *device,
				  VkViewport viewport);

// device.c, used by swapchain.c: the format the surface was opened with, decided
// once because the surface does not change when the window resizes.
[[nodiscard]] bool voe_render_device_choose_format(voe_render_device *device,
						   voe_base_arena *arena);

// How many timestamps one frame writes: one as its command buffer starts and one
// as it finishes. Named because 2 appears in the create, in the reset, in the
// read and in the size of the buffer read into, and four literal 2s meaning the
// same thing is three chances for them to stop meaning it.
#define VOE_RENDER_TIMESTAMPS_PER_FRAME 2

// buffer.c. A buffer of size with usage, in memory that has properties, and the
// one allocation under it. build/teardown rather than new/destroy because the
// struct is the caller's and only what is inside it belongs to these — the same
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
					   VkMemoryPropertyFlags properties);
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

// descriptors.c. Points a slot's set at the shared shading buffer. Called once
// per slot at startup, after the buffer exists; a record written later needs no
// second write, because the descriptor names the buffer and not its contents.
void voe_render_descriptors_write_shadings(voe_render_device *device,
					   VkDescriptorSet set);

// geometry.c. The two pools and the table of ranges into them. Startup's.
[[nodiscard]] bool voe_render_geometry_startup(voe_render_device *device);
void voe_render_geometry_shutdown(voe_render_device *device);

// geometry.c. What an id names, or NULL when it names nothing — a slot that was
// never claimed, or one whose generation has moved on. The one place a geometry
// id is turned into a range, so it is the one place that check is made.
const struct voe_render_geometry_slot *
voe_render_geometry_at(const voe_render_device *device,
		       voe_render_geometry geometry);

// shading.c. The record buffer and the slots that name its rows. Startup's.
[[nodiscard]] bool voe_render_shading_startup(voe_render_device *device);
void voe_render_shading_shutdown(voe_render_device *device);

// texture.c. The sampler and the one-pixel white texture every unclaimed slot
// points at, made once at startup. False with a message on failure.
[[nodiscard]] bool voe_render_texture_startup(voe_render_device *device);

// texture.c. Every texture, the sampler, and nothing else. Safe on a device that
// never got as far as making them.
void voe_render_texture_shutdown(voe_render_device *device);

// texture.c. Point one frame slot's descriptor set at every texture in the
// table. Called when a slot's set is built and again whenever the table changes,
// which is why it takes the set rather than the slot index.
void voe_render_texture_write_descriptors(voe_render_device *device,
					  VkDescriptorSet set);

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
