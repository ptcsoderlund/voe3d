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
//             if (!voe_render_frame_begin(gpu, size, view, sun, &drawing))
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
// world matrix, the matrix its normals want and which shading record to use are
// written by _draw as it records, and the shader reads its record by object
// number. Card 014's push constant for the model matrix is gone; the only push
// constant left is the object number itself.
//
// IT DRAWS TO AN IMAGE OF ITS OWN AND COPIES THAT TO THE WINDOW. The frame is
// cleared and drawn into an offscreen colour target and putting that on screen
// is a separate last step — which is what any post process, a render resolution
// the window is not, and an editor viewport all need in order to be possible at
// all.
//
// THERE IS A DEPTH BUFFER BEHIND THESE CALLS, ONE PER FRAME SLOT. This engine's
// depth runs backwards — near at 1.0, far at 0.0 — and nothing outside this
// folder has to know that. Opaque and cutout draws may therefore be issued in
// any order without changing the image.
//
// BLENDED DRAWS ARE THE EXCEPTION AND THEY ARE A SECOND CALL. voe_render_frame_draw
// writes depth; voe_render_frame_draw_blended tests it and does not write it,
// and blends what it draws over what is already there. Depth writes being off is
// what makes the order the caller issues them in load-bearing: this folder does
// not sort anything and cannot, because it does not know where anything is. The
// caller draws every opaque and cutout object first and then its blended ones
// furthest first — see voe_3d_draw_system_run, which is the one caller.
//
// THE COLOUR TARGET HOLDS PREMULTIPLIED COLOUR, AND ANYTHING THAT WRITES INTO IT
// OUTPUTS PREMULTIPLIED COLOUR. A fragment's rgb is already multiplied by its
// own alpha by the time it leaves a shader, and the blend is therefore
// source-one, destination-one-minus-source-alpha for colour and alpha both.
// Nothing enforces this — there is no validation layer for it and no test that
// can see it — so the next shader author reading this sentence is the whole
// mechanism. shaders/draw.slang says the same thing at the site that does the
// multiply.
//
// SIZE IS PASSED IN, EVERY FRAME, AND IT IS THE WINDOW'S ANSWER. This folder
// never asks a window how big it is: `platform` owns that truth and the caller
// already has it. A window that has changed size is not an event here — the size
// simply differs from the one the targets were built at, and the next frame
// rebuilds them.
//
// THE CAMERA IS TWO MATRICES AND WHERE THE EYE IS. Where the camera is, how fast
// it moves and what a mouse does to it are `scene`'s; how a field of view
// becomes a projection is `3d`'s, because the reversed depth and the clip-space
// conventions are this folder's business and `3d` is the folder allowed to know
// both. What arrives here is the answer. The eye is beside the matrices because
// a specular highlight is a function of where the surface is being looked from,
// and digging it back out of the inverse of the view matrix in a shader would be
// arithmetic to recover a number the caller already had.
//
// ONE DIRECTIONAL LIGHT, HANDED OVER WITH THE CAMERA, ONCE A FRAME. It is the
// sun: a direction, a colour and a strength, the same for every draw in the
// frame. There is no light list and no second light — many lights is a later
// card and it is the card that decides how they are gathered — and there is no
// shadow: nothing here tests whether anything is in the way.
//
// EVERY COLOUR THAT CROSSES THIS BOUNDARY IS LINEAR, AND sRGB LIVES AT THE TWO
// ENDS. A picture full of colour is uploaded as VOE_RENDER_TEXTURE_COLOUR and
// the hardware decodes it on every read; the frame is drawn in linear light and
// encoded once, by the target's own format, on the way to the window. So a
// factor, a light's colour and a clear colour are all linear numbers, and the
// only two places an sRGB curve is applied are inside the GPU where nobody has
// to write it down. A picture that holds numbers rather than colour —
// metalness, roughness, occlusion, a normal map — is uploaded as
// VOE_RENDER_TEXTURE_DATA and is read exactly as it was written.
//
// NOTHING IS TONE MAPPED, SO BRIGHT VALUES CLIP. A light strong enough to push a
// surface past one is clamped by the target's format and the highlight goes
// flat white. That is expected and it is not a bug to work around at a call
// site by keeping intensities low; the card that maps a high-dynamic-range
// target down to a screen is the card that fixes it, and card 013's offscreen
// target is what makes it possible.
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
	// The surface's normal in the model's own space, unit length. The
	// fragment stage lights with it, and what turns it into world space is
	// the normal matrix in voe_render_object below — never the world matrix.
	// A vertex whose normal is nothing is drawn unlit rather than black; see
	// shaders/draw.slang, which is where that decision is written down.
	voe_math_float3 normal;
	voe_math_float2 uv;
} voe_render_vertex;

// A range in the shared pools: some vertices and the indices that walk them.
typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_render_geometry;

// What a picture holds, which is what decides the format it is uploaded in and
// the only thing about a texture this API needs told.
//
// COLOUR MEANS sRGB-ENCODED AND DATA MEANS NUMBERS, AND THE CHOICE IS NOT
// COSMETIC. A base colour or emissive map is a picture somebody looked at while
// making it, so its bytes are sRGB-encoded and the hardware has to decode them
// before anything multiplies by them — that is what COLOUR asks for. A
// metalness, roughness, occlusion or normal map is numbers that were never a
// colour, and decoding one bends every value towards zero: a roughness of 0.5
// arrives as 0.21 and the whole surface goes shiny. Getting either one wrong is
// a picture that is slightly wrong everywhere and nothing that fails.
//
// A CALLER THAT WANTS ONE PICTURE BOTH WAYS UPLOADS IT TWICE. There is one
// format per texture slot, so a file whose single picture is referenced as both
// a base colour and an ORM map costs two slots; deciding that is the importer's,
// because it is the thing that knows what each reference is for.
typedef enum {
	VOE_RENDER_TEXTURE_COLOUR,
	VOE_RENDER_TEXTURE_DATA,
} voe_render_texture_kind;

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

// What a shading record's alpha means. The engine's three words, the same three
// `assets` reads out of a glTF's `alphaMode` and the same three a text or UI
// material will use.
//
// IT DECIDES WHAT THE SHADER DOES WITH ALPHA AND NOT WHICH PIPELINE IS USED.
// Which pipeline a draw goes through is which of the two draw calls the caller
// makes; this is what the fragment stage does once it is there. The two have to
// agree — a BLENDED record drawn through voe_render_frame_draw is a
// see-through-looking colour written into a pass that does not blend — and
// keeping them in step is the caller's, because the caller is what sorted them.
//
// OPAQUE IS 0 SO THAT A ZEROED RECORD IS AN OPAQUE ONE.
typedef enum {
	// Alpha is ignored entirely and the surface is fully solid, which is
	// what the glTF specification says an opaque material's alpha means.
	// That is not a detail: a record carrying an alpha of a half would
	// otherwise have half its colour written into a pass that does not
	// blend, and the surface would simply go dark.
	VOE_RENDER_ALPHA_OPAQUE = 0,
	// A hard edge: below the cutoff nothing is drawn, above it the surface
	// is fully solid. Cutout draws through voe_render_frame_draw and writes
	// depth like any other solid thing.
	VOE_RENDER_ALPHA_CUTOUT,
	// See-through, and the only mode that belongs in
	// voe_render_frame_draw_blended.
	VOE_RENDER_ALPHA_BLENDED,
} voe_render_alpha_mode;

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
	// A voe_render_alpha_mode, and the cutoff the cutout one reads. They are
	// here rather than at the end because this is the eight bytes that used
	// to be reserved_a: same offsets, same size, and the asserts in
	// render/src/descriptors.c are unchanged. A uint and a float and not a
	// two-element vector, for the alignment reason the paragraph above gives.
	uint32_t alpha_mode;
	float alpha_cutoff;
	voe_math_float3 emissive;
	// Not lit: the fragment stage skips the whole BRDF and writes the base
	// colour as it is. Zero is lit, so a zeroed record is an ordinary
	// surface. It is here rather than at the end because this is the four
	// bytes that used to be reserved_b — same offsets, same size, and the
	// asserts in render/src/descriptors.c are unchanged.
	//
	// IT IS A PROPERTY OF THE SURFACE AND NOT OF THE PASS. Text and a user
	// interface are the things that want it: a letter is a shape somebody
	// chose the colour of, and a sun moving across it is wrong rather than
	// pretty. Nothing about which pipeline the draw goes through changes,
	// so an unlit surface may be opaque, cutout or blended like any other.
	uint32_t unlit;
	// Texture ids, index halves only — VOE_RENDER_NO_TEXTURE where the
	// material references none. The base colour, the metallic-roughness and
	// the occlusion ones are sampled; the normal and the emissive ones are
	// stored and not read, because nothing has asked for normal mapping or
	// for emission yet (rule 10) and both are a card of their own.
	//
	// A COLOUR TEXTURE AND A DATA TEXTURE ARE NOT INTERCHANGEABLE HERE. The
	// base colour and the emissive ones are uploaded as
	// VOE_RENDER_TEXTURE_COLOUR and the other three as
	// VOE_RENDER_TEXTURE_DATA, because the first pair holds sRGB-encoded
	// colour and the rest hold numbers. Putting an id from the wrong kind in
	// one of these slots is a picture that is subtly too dark or too flat
	// and nothing that fails.
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
//
// Padded like the two records below and asserted on in render/src/descriptors.c,
// because it shares a buffer with the light and the shader reads both out of one
// block.
typedef struct {
	voe_math_float4x4 view;
	voe_math_float4x4 projection;
	// Where the eye is, in world space. The fragment stage wants it for the
	// direction a surface is being looked from.
	voe_math_float3 eye;
	float reserved;
} voe_render_view;

// The sun, for one frame.
//
// `direction` IS WHERE THE LIGHT GOES AND NOT WHERE THE SUN IS. A sun overhead
// travels downwards: (0, -1, 0). It must be unit length — the shader does not
// normalize it, because whoever owns the light is where a direction of nothing
// is a bug worth hearing about, and here it would be twelve floats of arithmetic
// per fragment to fix a caller's mistake.
//
// `colour` IS LINEAR AND `intensity` HAS NO UNIT. A colour of (1, 1, 1) tints
// nothing; the intensity is a multiplier that is whatever looks right, because
// there is no exposure and no tone mapping in this engine yet. A light of no
// intensity leaves every surface black, which is what a frame given a zeroed one
// looks like.
typedef struct {
	voe_math_float3 direction;
	float intensity;
	voe_math_float3 colour;
	float reserved;
} voe_render_light;

// One drawn object's record: the two matrices it is drawn with and the shading
// record it wears. `shading` is the index half of a voe_render_shading id; the
// shader reads the record it names.
//
// THE NORMAL MATRIX IS NOT THE WORLD MATRIX AND THE DIFFERENCE IS VISIBLE. A
// normal is not carried by a transform the way a point is: under a non-uniform
// scale, the world matrix tilts a normal off the surface it belongs to, and the
// result looks like broken lighting rather than a broken matrix — the surface
// stays where it is and the shading slides across it. What a normal wants is the
// inverse transpose of the world matrix, which is the same thing for a rotation
// and a uniform scale and something else entirely otherwise. This folder does
// not compute it: whoever built the world matrix is what hands one over, and
// that is voe_3d_normal_matrix.
//
// Padded for the same reason voe_render_shading_values is, and asserted on in
// render/src/descriptors.c.
typedef struct {
	voe_math_float4x4 world;
	voe_math_float4x4 normal;
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
// `kind` says whether the bytes are colour or numbers — see
// voe_render_texture_kind, which is the one thing here that is easy to get wrong
// and impossible to see.
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
					     voe_render_texture_kind kind,
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
// the camera and `light` as the sun for everything drawn until _end.
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
					  voe_render_view view,
					  voe_render_light light,
					  bool *drawing);

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

// The same draw through the blended pipeline: the depth test still runs, so
// something behind an opaque object is still hidden, but nothing is written to
// the depth buffer and what is drawn is blended over what is already there.
//
// DEPTH WRITES BEING OFF IS WHAT MAKES THE CALLER'S ORDER MATTER, AND IT IS NOT
// A DETAIL. With them on, two blended objects would hide each other and the
// caller's sort would appear to work while doing nothing. Draw every blended
// object after every opaque one, furthest away first.
//
// THE RECORD IT NAMES SHOULD BE A VOE_RENDER_ALPHA_BLENDED ONE. Nothing checks
// it: a cutout record through here draws a hard edge that writes no depth, and
// an opaque one draws solid. Both are the caller's mistake and neither fails.
//
// False for the same two reasons voe_render_frame_draw is, and with the same
// asserts.
[[nodiscard]] bool voe_render_frame_draw_blended(voe_render_device *device,
						 voe_render_geometry geometry,
						 voe_render_object object);

// Clears the depth buffer where the frame stands, without touching what has
// already been drawn into the colour one. Everything drawn after this call
// therefore sees an empty depth buffer and is in front of everything drawn
// before it, while still testing and occluding normally among itself.
//
// THIS IS THE WHOLE OF WHAT A LAYER IS, AND IT IS ONE CALL RATHER THAN A SECOND
// PASS. The caller draws a group, clears depth, and draws the next group; the
// frame still opens exactly one rendering block and there is nothing here to
// tear down or resume. See voe_3d_draw_system_run, which is the one caller, and
// which calls this once between the world and the overlay.
//
// IT TAKES NO CLEAR VALUE AND WILL NOT BE GIVEN ONE. Depth runs backwards in
// this engine — far is 0, near is 1.0, the test is GREATER — and that number
// lives in this folder and nowhere else. A caller-supplied value is a way to get
// the convention wrong that this signature simply does not offer; the value used
// is the same one the frame's own load op clears to.
//
// THE COLOUR ATTACHMENT IS NOT TOUCHED. Only the depth aspect is cleared, over
// the whole render area, so the picture built so far survives and the next group
// is drawn on top of it.
//
// Calling this without a _begin that set `drawing`, or after _end, is the
// caller's bug and asserts — the same mistake voe_render_frame_draw asserts on.
void voe_render_frame_clear_depth(voe_render_device *device);

// Ends the recording, submits it, and — where there is a window — copies the
// target into the acquired swapchain image and presents it.
//
// False means the driver refused something no retry will fix. A swapchain that
// went stale is handled here and returns true, with the rebuild happening at the
// top of the next frame.
[[nodiscard]] bool voe_render_frame_end(voe_render_device *device);

// ------------------------------------------------------------------ timing

// How long the graphics card spent on a frame, in seconds, from the card's own
// clock. False when there is no number to hand back, which is two situations and
// a caller need not tell them apart: the card or its queue cannot write
// timestamps, or no frame has finished yet. `seconds` is untouched then.
//
// IT IS NOT THE FRAME YOU JUST ENDED AND IT CANNOT BE. A timestamp is read out
// of a query pool, and a query pool may not be read until the card has finished
// writing it — which is what the fence at the top of a frame waits for. So the
// newest measurement that exists is the one from the last frame to run on the
// slot this frame is about to reuse, which is VOE_RENDER_FRAMES_IN_FLIGHT frames
// back. Asking the card sooner would mean waiting for it, and a program that
// waits for the GPU in order to time the GPU has changed the thing it is timing.
// Average enough of them and the lag does not matter; read one and expect it to
// describe the frame on screen and it will mislead.
//
// IT IS THE CARD'S WORK AND NOT THE WAIT FOR THE DISPLAY. The two timestamps are
// written at the top and the bottom of one frame's command buffer, so what is
// between them is submitted work executing. Presenting, and everything the
// presentation engine does with the result afterwards, is outside them.
[[nodiscard]] bool voe_render_frame_gpu_time(const voe_render_device *device,
					     double *seconds);

// ----------------------------------------------------------------- present

// How a finished frame reaches the display.
//
// A DEVICE OPENS ON MAILBOX, AND THAT IS THIS ENGINE'S DEFAULT BECAUSE IT IS THE
// UNCAPPED ONE. The rule is performance by default: nothing here waits for
// anything it does not have to, and what costs time — post-processing, real-time
// global illumination, whatever comes next — is added deliberately by whoever
// wants it rather than being paid for by everyone who does not. Card 020
// measured both and the principal decided this one.
//
// SO FIFO IS THE ONE A CALLER ASKS FOR, and there are real reasons to: a laptop
// on a battery, a scene the display is the only limit on, or anything that would
// rather not draw fifteen frames for every one a person sees.
typedef enum {
	// Wait for the display. Every frame drawn is a frame shown, in order,
	// and the queue blocks once it is full — so the program runs at the
	// refresh rate and cannot run faster. No tearing. Every driver supports
	// it and none may refuse it, which is why it is what everything else
	// falls back to, and why it is 0: a zeroed voe_render_present is the
	// mode that always works, whatever a caller forgot to fill in.
	VOE_RENDER_PRESENT_FIFO = 0,

	// Do not wait. A frame finished while another is waiting to be shown
	// replaces it rather than queueing behind it, so what reaches the
	// display is always the newest frame there is and the older one is
	// thrown away. No tearing either — the swap still happens at the
	// refresh — and the gain is latency, at the cost of drawing frames
	// nobody ever sees, which is a real cost in power and heat.
	//
	// IT IS OPTIONAL AND A DRIVER MAY NOT OFFER IT, WHICH IS WHY THE DEFAULT
	// BEING THIS ONE IS SAFE. Asking for it on a surface that does not have
	// it is not a failure: the device falls back to FIFO and _get says so.
	// Ask, then ask what you got — never assume a device is presenting the
	// way it was opened.
	VOE_RENDER_PRESENT_MAILBOX,
} voe_render_present;

// Ask for a mode. It takes effect on the next frame — the swapchain has to be
// built again for it, and that happens where every other rebuild does, at the
// top of a frame — so _get will still report the old one until then, and will
// report FIFO for ever if this surface has no mailbox.
//
// A device already wants MAILBOX when it opens, so this is for asking for FIFO
// and for going back again; it is not something a caller has to call to get the
// engine's default.
void voe_render_present_set(voe_render_device *device, voe_render_present mode);

// What the swapchain that exists was actually built with, which is the answer to
// what is happening and not to what was asked for — a device opened wanting
// MAILBOX on a surface that has none reports FIFO here, and that is the honest
// answer rather than a failure. A headless device presents nothing and reports
// FIFO.
voe_render_present voe_render_present_get(const voe_render_device *device);
