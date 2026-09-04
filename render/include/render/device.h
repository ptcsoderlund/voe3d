// The GPU, opened onto a window. Create one, hand it geometry, textures and
// shading records at startup, then ask it for a frame every time round the loop
// and tell it what to draw.
//
//     voe_base_arena *scratch = voe_base_arena_new(64 * 1024);
//     voe_base_error error;
//     voe_render_capacities room = {
//             .vertices = 1 << 16, .indices = 1 << 17,
//             .geometries = 64, .objects = 256, .shadings = 64,
//     };
//     voe_render_device *gpu = voe_render_device_new(scratch,
//                                                    voe_platform_window_native(window),
//                                                    voe_platform_window_size(window),
//                                                    room, &error);
//     voe_base_arena_destroy(scratch);        // the device kept nothing from it
//
//     voe_render_geometry cube;
//     voe_render_geometry_create(gpu, vertices, 24, indices, 36, &cube, &error);
//
//     while (!voe_platform_window_should_close(window)) {
//             bool drawing;
//             voe_platform_window_poll(window);
//             if (!voe_render_frame_begin(gpu, size, view, &drawing))
//                     break;
//             if (drawing) {
//                     voe_render_frame_draw(gpu, cube, (voe_render_object){
//                             .world = matrix, .shading = shading.index });
//                     if (!voe_render_frame_end(gpu))
//                             break;
//             }
//     }
//     voe_render_device_destroy(gpu);
//
// NOTHING IN HERE KNOWS ABOUT A SCENE, AN ENTITY OR A FILE, AND THAT IS THE
// BOUNDARY THIS FOLDER IS FOR. It holds buffers, images, a pipeline and a frame;
// what a thing is, where it came from and why it is being drawn is `3d`'s. Every
// function below exists because a line in `3d` calls it, and a gap in this API
// is filled here rather than reached around.
//
// EVERYTHING IS NAMED BY A GENERATIONAL ID AND THE INDEX HALF IS THE NUMBER THE
// SHADER USES (ADR-0018). A texture id indexes the array the fragment stage
// samples; a shading id indexes the record buffer it reads. There is no table
// between a caller's id and the GPU's view of it, so there is nothing that can
// fall out of step — and a component that holds one of these holds two integers
// rather than a Vulkan handle, which is what lets it be written out as text,
// copied, sorted and read by a thread that may not touch Vulkan.
//
// GEOMETRY LIVES IN TWO SHARED POOLS AND A MESH IS A RANGE IN THEM. One vertex
// pool and one index pool, created with the device, appended to by
// voe_render_geometry_create and NEVER FREED — there is no _destroy for one on
// purpose, because nothing in the engine unloads anything yet and a free list
// arrives with the card that does. That layout is also what lets many objects be
// drawn from one buffer later, with one indirect call instead of one call each.
//
// PER-OBJECT DATA GOES INTO ONE BUFFER PER FRAME SLOT, ONE RECORD PER DRAW. The
// world matrix and which shading record to use are written by _draw as it
// records, and the shader reads its record by object number. Card 014's push
// constant for the model matrix is gone; the only push constant left is the
// object number itself.
//
// IT DRAWS TO AN IMAGE OF ITS OWN AND COPIES THAT TO THE WINDOW. The frame is
// cleared and drawn into an offscreen colour target and putting that on screen
// is a separate last step — which is what any post process, a render resolution
// the window is not, and an editor viewport all need in order to be possible at
// all.
//
// IT IS SOLID, WHICH MEANS THERE IS A DEPTH BUFFER BEHIND THESE CALLS, ONE PER
// FRAME SLOT. This engine's depth runs backwards — near at 1.0, far at 0.0 — and
// nothing outside this folder has to know that. Opaque draws may therefore be
// issued in any order without changing the image.
//
// SIZE IS PASSED IN, EVERY FRAME, AND IT IS THE WINDOW'S ANSWER. This folder
// never asks a window how big it is: `platform` owns that truth and the caller
// already has it. A window that has changed size is not an event here — the size
// simply differs from the one the targets were built at, and the next frame
// rebuilds them.
//
// THE CAMERA IS TWO MATRICES AND NOTHING ELSE. Where the camera is, how fast it
// moves and what a mouse does to it are `scene`'s; how a field of view becomes a
// projection is `3d`'s, because the reversed depth and the clip-space
// conventions are this folder's business and `3d` is the folder allowed to know
// both. What arrives here is the answer.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <stdint.h>

typedef struct voe_render_device voe_render_device;

// How much room the device makes for everything it holds. All of it is fixed at
// creation: nothing here grows, and asking for one more than was asked for is a
// returned failure at the call that asked.
//
// vertices and indices are counts, not bytes. objects is per frame slot and
// bounds how many draws one frame may contain.
typedef struct {
	uint32_t vertices;
	uint32_t indices;
	uint32_t geometries;
	uint32_t objects;
	uint32_t shadings;
} voe_render_capacities;

// What the vertex pool holds, and what the pipeline's vertex input describes. A
// caller builds an array of these and hands it over; the layout is this folder's
// because the shader has to agree with it.
//
// (0,0) IS THE TOP-LEFT OF THE TEXTURE, which is what Vulkan and glTF both mean
// by it — see assets/include/assets/image.h for why nothing turns a picture over
// on the way in.
typedef struct {
	voe_math_float3 position;
	// Not read by this card's shader; the lighting card is what consumes it.
	// It is in the layout now because geometry that arrives without one
	// would have to be uploaded again when it does.
	voe_math_float3 normal;
	voe_math_float2 uv;
} voe_render_vertex;

// A range in the shared pools: some vertices and the indices that walk them.
typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_render_geometry;

// A texture on the GPU. `index` is the subscript the fragment stage uses;
// `generation` never reaches the GPU and is what makes a stale id safe, because
// slots are reused and an id kept across a destroy would otherwise name
// whatever moved in.
typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_render_texture;

// The texture index that means "there isn't one". It names the one-pixel white
// texture every unclaimed slot points at, so a shader may test it and get the
// same answer either way: sampling it is opaque white, which leaves a material's
// factors as they are. A zeroed voe_render_texture is this, deliberately.
#define VOE_RENDER_NO_TEXTURE 0

// One record of how a surface is shaded, and the id that names it.
//
// THE LAYOUT IS PADDED AND THAT IS NOT COSMETIC. The same struct is declared in
// the shader, and the two only agree for certain when no member lands where a
// buffer layout rule would have moved it: a four-component vector wants a
// sixteen-byte boundary and a three-component one wants one too, which is where
// the reserved fields come from. Do not remove them and do not reorder anything
// without changing draw.slang with it — and note that the padding there is
// written as separate scalars for exactly this reason, because a uint3 would be
// realigned and a uint would not. render/src/descriptors.c asserts on the size,
// so a mistake in the size is a build error rather than a wrong picture; a
// mistake in the *order* is not, which is why this paragraph is here.
typedef struct {
	voe_math_float4 base_colour;
	float metallic;
	float roughness;
	float reserved_a[2];
	voe_math_float3 emissive;
	float reserved_b;
	// Texture ids, index halves only — VOE_RENDER_NO_TEXTURE where the
	// material references none. This card writes them and draws with the
	// base colour one; the lighting card is what reads the rest.
	uint32_t base_colour_texture;
	uint32_t metallic_roughness_texture;
	uint32_t normal_texture;
	uint32_t occlusion_texture;
	uint32_t emissive_texture;
	uint32_t reserved_c[3];
} voe_render_shading_values;

typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_render_shading;

// The camera, for one frame: where it is and what it can see, already worked out
// by whoever owns those questions.
typedef struct {
	voe_math_float4x4 view;
	voe_math_float4x4 projection;
} voe_render_view;

// One drawn object's record. `shading` is the index half of a voe_render_shading
// id; the shader reads the record it names.
//
// Padded for the same reason voe_render_shading_values is, and asserted on in
// render/src/descriptors.c.
typedef struct {
	voe_math_float4x4 world;
	uint32_t shading;
	uint32_t reserved[3];
} voe_render_object;

// native and size come from voe_platform_window_native() and
// voe_platform_window_size(). arena is scratch for the enumerations startup does
// — how many graphics cards, which queue families, which surface formats — and
// nothing the device keeps is allocated out of it, so it may be rewound or
// destroyed as soon as this returns.
//
// NULL on failure, with error saying which category it was and a line on stderr
// saying exactly what happened. There are three ways this fails and a caller can
// tell them apart: no Vulkan on the machine, no graphics card that meets what
// the engine requires, and a driver that refused. error may be NULL.
[[nodiscard]] voe_render_device *voe_render_device_new(voe_base_arena *arena,
						       voe_platform_native native,
						       voe_platform_size size,
						       voe_render_capacities capacities,
						       voe_base_error *error);

// The same device with no window, no surface and no swapchain, drawing into its
// offscreen target and presenting nothing.
//
// IT IS PUBLIC BECAUSE TESTS NEED A GPU AND NOT A WINDOW. Everything above
// `render` is tested against a device, and a test that opened a window would
// need a compositor, a display and someone to close it. It is not for the
// engine's own use: a program that draws for a person opens a window.
// voe_render_frame_end submits and returns on a device made this way — there is
// nothing to acquire and nothing to present — so a caller that wants the pixels
// reads the target itself.
[[nodiscard]] voe_render_device *
voe_render_device_new_headless(voe_base_arena *arena, voe_platform_size size,
			       voe_render_capacities capacities,
			       voe_base_error *error);

void voe_render_device_destroy(voe_render_device *device);

// --------------------------------------------------------------- geometry

// Appends the vertices and the indices to the pools and hands back the id that
// names the range.
//
// The indices are the caller's own numbering, from zero: the offset into the
// pool is added by the draw, so a mesh's indices never have to be rewritten
// because something was uploaded before it.
//
// FAILS WHEN EITHER POOL HAS NO ROOM LEFT, WHEN THERE IS NO SLOT LEFT TO NAME
// THE RANGE, OR WHEN THE CARD REFUSES THE UPLOAD. All three are returned rather
// than fatal: two of them are the caller asking for more than it said it would
// need, which is a thing a loader can report and stop. A count of zero is the
// caller's bug and asserts.
//
// IT WAITS FOR THE GPU TO GO IDLE, SO IT IS A STARTUP OPERATION. The upload is
// staged through a buffer that has to be destroyed once the copy is done.
// Streaming geometry in while drawing is a different mechanism and a different
// card.
[[nodiscard]] bool voe_render_geometry_create(voe_render_device *device,
					      const voe_render_vertex *vertices,
					      uint32_t vertex_count,
					      const uint32_t *indices,
					      uint32_t index_count,
					      voe_render_geometry *out,
					      voe_base_error *error);

// ---------------------------------------------------------------- textures

// Upload width * height RGBA8 pixels and hand back the id that names them.
//
// The pixels are copied and the caller's buffer is its own again the moment this
// returns — which is what lets the arena a decoder used be rewound immediately.
//
// FAILS WHEN THE CARD REFUSES OR THERE IS NO SLOT LEFT, and both are the world's
// doing rather than the caller's. A width or height of zero is the caller's bug
// and asserts.
//
// IT WAITS FOR THE GPU TO GO IDLE, SO IT IS A STARTUP OPERATION. The descriptor
// sets it rewrites may not be touched while a frame is reading them.
[[nodiscard]] bool voe_render_texture_create(voe_render_device *device,
					     uint32_t width, uint32_t height,
					     const uint8_t *rgba,
					     voe_render_texture *out,
					     voe_base_error *error);

// Gives the slot back and bumps its generation, so every id naming it — the
// caller's, and any copy anywhere else — is refused from here on. False when the
// id was already stale, which is not an error and needs no handling: it means
// somebody else got here first.
//
// A STARTUP AND SHUTDOWN OPERATION, for the same reason creating one is: it
// waits for the GPU to go idle and rewrites the descriptor sets.
bool voe_render_texture_destroy(voe_render_device *device,
				voe_render_texture texture);

// ---------------------------------------------------------------- shading

// Writes one shading record and hands back the id that names it. Textures the
// record references must already exist; their ids are not checked here, and an
// id that names nothing samples the white default rather than failing.
//
// FAILS WHEN THERE IS NO ROOM LEFT, which is the one way. A startup operation.
[[nodiscard]] bool voe_render_shading_create(voe_render_device *device,
					     voe_render_shading_values values,
					     voe_render_shading *out,
					     voe_base_error *error);

// ------------------------------------------------------------------ frames

// Starts the frame: rebuilds what a resize invalidated, waits for the slot this
// frame will use, takes a swapchain image, and begins recording with `view` as
// the camera for everything drawn until _end.
//
// `drawing` COMES BACK FALSE WHEN THERE IS NOTHING TO DRAW INTO — a window with
// no area, or a swapchain that has just gone stale. That is not an error: no
// draws may be issued, _end must not be called, and the next frame will find it
// has come back. It also does not wait, so a loop that does nothing else will
// spin.
//
// False means this device cannot draw any more and the program should stop
// asking. Everything a frame can hit that a retry fixes is handled here.
[[nodiscard]] bool voe_render_frame_begin(voe_render_device *device,
					  voe_platform_size size,
					  voe_render_view view, bool *drawing);

// Draws one range with one object record, in the order the calls are made. The
// record is written into this slot's object buffer and the object's number is
// what the shader is told; both are this folder's bookkeeping and neither is a
// number the caller has to keep.
//
// False when this frame already holds as many objects as the device was made
// for, or when the geometry id names nothing. Neither is retryable inside this
// frame.
//
// Calling this without a _begin that set `drawing`, or after _end, is the
// caller's bug and asserts.
[[nodiscard]] bool voe_render_frame_draw(voe_render_device *device,
					 voe_render_geometry geometry,
					 voe_render_object object);

// Ends the recording, submits it, and — where there is a window — copies the
// target into the acquired swapchain image and presents it.
//
// False means the driver refused something no retry will fix. A swapchain that
// went stale is handled here and returns true, with the rebuild happening at the
// top of the next frame.
[[nodiscard]] bool voe_render_frame_end(voe_render_device *device);
