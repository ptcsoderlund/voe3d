// The innards of voe_render_device, shared by the four files that make one:
// device.c starts it, target.c makes the images the scene is drawn into,
// swapchain.c builds the images the window is made of, and frame.c draws.
// Nothing outside render/src sees this.
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
// VOE_RENDER_DEPTH_FORMAT and voe_render_cube_projection are the two places the
// convention is actually spelled out.
#pragma once

#include "loader.h"

#include <base/arena.h>
#include <math/float4x4.h>
#include <platform/window.h>
#include <render/device.h>

// The most images a swapchain here may have. FIFO presentation hands back three
// or four on every driver measured; the number exists so that the per-image
// arrays are members and a resize allocates and frees nothing. A driver that
// wants more than this is refused with a message rather than quietly clamped —
// clamping would leave images we never made a view for and an acquire that
// returns an index we cannot draw to.
#define VOE_RENDER_MAX_IMAGES 8

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

// A buffer and the memory under it, which in this engine are always made and
// thrown away together. One allocation per buffer, exactly as target.c makes one
// per image, and the same note applies: an engine that made many of these would
// sub-allocate out of a few large blocks instead. This one makes four.
struct voe_render_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
};

// One vertex of the cube, and the only vertex format in the engine. Position and
// colour, both float3, because there are no texture coordinates yet — the card
// that adds them rewrites this struct, the cube's data and the two attribute
// descriptions in device.c together.
//
// THE FIELD ORDER IS THE ATTRIBUTE ORDER AND BOTH ARE STATED, NOT COUNTED.
// cube.slang gives its inputs vk::location 0 and 1 explicitly and device.c's
// VkVertexInputAttributeDescription names the same two numbers with offsetof, so
// a field inserted in the middle of this struct moves one offset and breaks
// nothing silently.
struct voe_render_vertex {
	voe_math_float3 position;
	voe_math_float3 colour;
};

// The camera, and everything a whole frame shares. This struct is memcpy'd into
// a mapped uniform buffer and read on the other side as two float4x4, which is a
// straight copy for two reasons that both have to hold: voe_math_float4x4 is
// row-major and slangc is invoked with -matrix-layout-row-major, and two 64-byte
// members packed end to end already satisfy the 16-byte alignment a uniform
// block wants, so there is no padding to declare.
//
// THE MODEL MATRIX IS NOT IN HERE, AND THAT IS WHAT MAKES TWO OBJECTS POSSIBLE.
// Everything in this struct is written once per frame and read by every draw in
// it; a per-object matrix is the opposite of that, and it goes through
// voe_render_push below. What is left is exactly "the camera", which is why the
// struct no longer names a cube.
//
// render/tests/matrix.c IS WHAT KEEPS THE ROW-MAJOR CLAIM TRUE. Remove the
// slangc flag and every transform comes out transposed with nothing failing to
// compile; that test uploads a known matrix through this struct — through view,
// which is the first member — and makes the shader say what it read.
struct voe_render_uniforms {
	voe_math_float4x4 view;
	voe_math_float4x4 projection;
};

// The per-object matrix, and the whole of what one draw is told that the draw
// beside it is not. Pushed into the command buffer between two draws rather than
// written to a buffer, and the reasons are in cube.c's header where the two
// cubes are.
//
// SIXTY-FOUR BYTES, WHICH IS WHY THERE IS NOTHING TO QUERY. Vulkan requires
// maxPushConstantsSize to be at least 128, so one 4x4 matrix fits on every
// implementation there is and device.c does not have to ask. A second member
// here is a decision — 128 is the floor, not the typical limit — and the day
// something wants one, that is the day this gets a comment about what was
// checked.
struct voe_render_push {
	voe_math_float4x4 model;
};

// Where the camera is and which way it is looking, when it is being flown. The
// state that has to survive between two frames, and nothing more: the matrices
// are built from this every frame and never stored.
//
// AN ANGLE PAIR AND NOT A MATRIX, AND NOT A QUATERNION EITHER. What a mouse
// gives is a change in yaw and a change in pitch, and what a fly camera needs is
// that pitch cannot go past straight up — which is a clamp on a number and has
// no meaning on a matrix. Accumulating into a rotation instead is also how a
// camera acquires roll it was never asked for: two rotations composed in the
// order the mouse happened to move leave the horizon tilted, and every frame
// after that makes it worse. Two floats cannot do that.
//
// SO THERE IS NO ROLL IN HERE AND THAT IS THE POINT RATHER THAN A GAP. A camera
// a person flies wants the horizon level; something that wants to barrel-roll is
// a different camera and it stores a rotation.
//
// flying IS THE PREVIOUS FRAME'S ANSWER AND IT IS WHAT MAKES THE HANDOVER
// SMOOTH. The frame the caller first asks to fly, eye, yaw and pitch are seeded
// from wherever the orbit had reached, so the view does not jump; that needs to
// know that last frame was not flying, which is the only thing this field is
// for.
struct voe_render_camera {
	voe_math_float3 eye;
	// Radians. Zero looks along -Z, and a positive angle turns towards -X,
	// which is a left turn — the engine's handedness, the same direction
	// math/tests/quat.c proves a positive rotation about +Y goes.
	float yaw;
	// Radians. Positive looks up, and it is clamped short of straight up
	// where a look-at's up vector stops meaning anything.
	float pitch;
	bool flying;
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
// NEITHER HALF IS THE DEFAULT ONE. Colour was the only image here until the cube
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
	// Written through as a struct voe_render_uniforms and never read back.
	void *uniforms_mapped;
	// Points at uniforms.buffer, allocated from device->descriptor_pool and
	// freed with it. A set is not destroyed on its own anywhere in here.
	VkDescriptorSet descriptor;
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
	VkSurfaceFormatKHR format;

	// The cube's pipeline, and the layout it needs in order to exist.
	// Startup's, not the swapchain's: the viewport and the scissor are
	// dynamic state, so a resize changes neither of these and there is
	// nothing here to rebuild.
	//
	// The layout is no longer empty — it names descriptor_layout below, and
	// so does the probe's pipeline, which is why the layout is the device's
	// and not the pipeline's private business.
	VkPipelineLayout layout;
	VkPipeline pipeline;

	// Set 0, binding 0: one uniform buffer, read by the vertex stage. One
	// layout describes every slot's set, and the pool below is sized for
	// exactly VOE_RENDER_FRAMES_IN_FLIGHT of them and never grows — sets are
	// allocated once at startup and freed by destroying the pool.
	VkDescriptorSetLayout descriptor_layout;
	VkDescriptorPool descriptor_pool;

	// The cube: one vertex buffer, one index buffer, both device-local and
	// both filled once at startup through a staging buffer that is gone
	// before the first frame. Startup's, and untouched by a resize.
	//
	// indices IS THE DRAW'S COUNT AND NOT THE BUFFER'S SIZE. Two numbers that
	// are 36 and 72 respectively, and reading one for the other draws either
	// twice the cube or half of it.
	struct voe_render_buffer vertices;
	struct voe_render_buffer indices;
	uint32_t index_count;

	// How long the engine has been drawing, in seconds, and the only moving
	// part of the scene in cube.c. Advanced once per recorded frame in
	// frame.c and read nowhere else.
	//
	// IT IS NOT MEASURED, IT IS COUNTED, AND THAT IS THIS CARD'S ONE
	// PLACEHOLDER. There is no clock in this engine yet — platform will own
	// one and card 020 is the card that brings it — so a frame adds a
	// nominal frame's worth of seconds rather than asking how long the last
	// one took. Everything downstream is already in seconds, so replacing
	// this with a measured delta is one line in frame.c and nothing else.
	// What it costs meanwhile is that the orbit's speed follows the refresh
	// rate.
	float seconds;

	// The flown camera, when it is being flown. Zeroed at startup, which is
	// not flying, which is the orbit — so a device nobody has handed any
	// input to draws exactly what card 015 drew, and the two tests in this
	// folder that draw a frame never mention it.
	struct voe_render_camera camera;

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

// frame.c. Writes this slot's camera, clears its colour and depth, draws every
// cube into it, and leaves the colour image in TRANSFER_SRC_OPTIMAL ready to be
// copied somewhere. The command buffer is the slot's and must already have been
// begun.
//
// WHAT IT DRAWS IS device->seconds AND NOTHING IS PASSED IN. A caller that
// wanted a particular moment would be a second way to say what the scene is, and
// render has one. A test calling this on a device it never asked for a frame
// from therefore sees the scene at zero, which is deterministic and is what
// render/tests/offscreen.c relies on.
//
// IT WRITES THE UNIFORM BUFFER AS WELL AS RECORDING, AND THAT IS SAFE BECAUSE OF
// THE FENCE. The matrices go into this slot's mapped buffer here, which is a CPU
// write to memory the GPU may have been reading up until the fence at the top of
// the frame was signalled. Every caller has waited on that fence.
//
// THE VIEWPORT IS A PARAMETER BECAUSE THE TWO CALLERS HAND IN DIFFERENT ONES. A
// frame hands in voe_render_frame_viewport(); render/tests/offscreen.c hands in
// that one and then its mirror image, because the same geometry drawn through a
// mirrored viewport is wound the other way round in framebuffer space — so every
// face the engine would cull is drawn and every face it would draw is culled.
// That gets a back face in front of the rasteriser without a second shader and
// without touching the pipeline whose front-face constant is the thing under
// test.
void voe_render_frame_draw(voe_render_device *device,
			   const struct voe_render_frame *frame,
			   VkViewport viewport);

// device.c, used by swapchain.c: the format the surface was opened with, decided
// once because the surface does not change when the window resizes.
[[nodiscard]] bool voe_render_device_choose_format(voe_render_device *device,
						   voe_base_arena *arena);

// device.c. A device with no window: no surface, no swapchain, and neither of
// the extensions that need one. Everything else — the graphics card, the logical
// device, the pipeline, the frame slots and their targets — is the same code the
// windowed device runs, which is the whole point of it: a test on this is a test
// of what ships.
//
// It is here and not in render/device.h on purpose. Drawing into a target and
// reading it back needs no window and no compositor, which is what lets
// render/tests/offscreen.c run in ctest on a machine with no display; nothing
// outside render has asked to open a windowless device, and rule 10 says not to
// offer one until something does.
[[nodiscard]] voe_render_device *
voe_render_device_new_headless(voe_base_arena *arena, voe_platform_size size,
			       voe_base_error *error);

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
// not four lines inside cube.c.
//
// IT IS STARTUP'S AND IT BLOCKS. The staging buffer is made, filled, copied and
// destroyed inside one call, which means waiting for the copy to finish before
// the staging buffer can go away. That is the right trade for data uploaded once
// before the first frame and the wrong one for anything uploaded per frame; the
// day something needs the second, it needs a different function and not a flag
// on this one.
[[nodiscard]] bool voe_render_buffer_upload(voe_render_device *device,
					    const struct voe_render_buffer *buffer,
					    const void *data, VkDeviceSize size);

// cube.c. Everything the cube needs that a resize does not touch: the descriptor
// set layout, the pool, one set and one mapped uniform buffer per frame slot,
// and the two device-local buffers holding the geometry. Startup's, and the
// counterpart tears down whatever was built before a failure.
[[nodiscard]] bool voe_render_cube_build(voe_render_device *device);
void voe_render_cube_teardown(voe_render_device *device);

// cube.c. How many cubes there are, which is how many draws a frame records and
// how many model matrices it pushes. Two: one turning on its own axis and one
// standing still, which is the arrangement that makes an orbiting camera
// distinguishable from a rotating object. See cube.c's header.
#define VOE_RENDER_CUBE_COUNT 2

// cube.c. The camera's two matrices for a target of this size, ready to be
// copied into a slot's uniform buffer.
//
// TWO CAMERAS, AND camera->flying CHOOSES. Flying, the view comes from where the
// camera is and which way it is looking; not flying, it comes from the hardcoded
// orbit at this many seconds in, which is what card 015 built and what the tests
// here see. seconds is read only in the second case and the projection is the
// same either way.
void voe_render_cube_uniforms_fill(struct voe_render_uniforms *uniforms,
				   VkExtent2D extent,
				   const struct voe_render_camera *camera,
				   float seconds);

// cube.c. Moves and turns the camera by what the caller says the person did,
// over `dt` seconds. Called once per frame, before the matrices are built.
//
// IT IS WHERE THE HANDOVER FROM THE ORBIT HAPPENS. The first frame look.fly is
// true, the camera is placed where the orbit had reached at `seconds` and
// pointed the way the orbit was pointing, so taking control does not jump; the
// first frame it is false again, nothing is stored and the orbit resumes from
// its own clock, which never stopped. Handing back therefore does jump, and
// that is the honest half of it — the orbit is a function of time and it does
// not wait.
void voe_render_cube_camera_step(struct voe_render_camera *camera,
				 voe_render_camera_input input, float seconds,
				 float dt);

// cube.c. Where cube `index` is at this many seconds in, as the matrix a draw
// pushes. index is below VOE_RENDER_CUBE_COUNT and asserts if it is not.
voe_math_float4x4 voe_render_cube_model(uint32_t index, float seconds);

// cube.c, and named here because it is the one place this engine's reversed depth
// is written down as arithmetic rather than as a comparison constant. Near plane
// at 1.0, far plane at 0.0, an infinite far distance, and no Y negation anywhere
// in it — the viewport owns the flip. render/tests/matrix.c checks all three of
// those claims on the CPU, where no graphics card is needed to disagree.
voe_math_float4x4 voe_render_cube_projection(VkExtent2D extent);

// probe.c. The pipeline that reads a matrix and reports what it saw, built on
// demand and owned by the caller — VK_NULL_HANDLE on failure, and destroyed with
// voe_render_vk.destroy_pipeline. It shares the device's pipeline layout, so it
// reads the same descriptor the cube does.
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
