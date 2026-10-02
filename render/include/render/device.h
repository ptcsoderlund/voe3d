// The GPU, opened onto a window. Create one, hand it geometry, textures and
// shading records at startup, then ask it for a frame every time round the loop
// and tell it what to draw.
//
//     voe_base_arena *scratch = voe_base_arena_new(64 * 1024);
//     voe_base_error error;
//     voe_render_capacities room = {
//             .vertices = 1 << 16, .indices = 1 << 17,
//             .geometries = 64, .objects = 256, .shadings = 64, .passes = 4,
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
//             if (!voe_render_frame_begin(gpu, size, &drawing))
//                     break;
//             if (drawing) {
//                     voe_render_pass_camera camera = { .view = view, .light = sun };
//                     if (voe_render_pass_begin(gpu, VOE_RENDER_TARGET_WINDOW, &camera)) {
//                             voe_render_frame_draw(gpu, cube, (voe_render_object){
//                                     .world = matrix, .shading = shading.index,
//                                     .colour = { 1, 1, 1, 1 } });
//                             voe_render_pass_end(gpu);
//                     }
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
// A frame's camera passes come after the sun's shadow cascades and its bounce
// pass; VOE_RENDER_BOUNCE_* size the bounce grid, voe_render_bounce_update
// refreshes a target's grid from the bounce map, a pass may copy its depth
// (voe_render_frame_copy_depth), and a shading record may be water, with waves
// and sky in the object record.
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
//
// THE THREE transient_ NUMBERS ARE THE SECOND KIND OF GEOMETRY — see
// voe_render_geometry_create_transient — and they are the one group that may be
// nought. A program that builds no geometry inside a frame asks for none and
// pays for none; the first transient create on such a device is refused with a
// message rather than asserting. They are per frame slot, like objects: the
// pool is emptied at the top of every frame, so the numbers bound what one
// frame may build and not what a program may build over its life. Slots are
// shared, so transient_geometries is how many transient ranges one frame may
// name.
//
// elements IS THE SECOND NUMBER THAT MAY BE NOUGHT, and it is per frame slot for
// the same reason objects is — see voe_render_element. A program that draws no
// elements asks for none and pays for none; the first submit on such a device is
// refused with a message rather than asserting.
//
// passes IS PER FRAME SLOT AND AT LEAST ONE. It bounds how many passes one frame
// may open, and each costs one camera-and-sun block in the slot's uniform buffer.
// A frame with shadows spends one per cascade, one on the bounce map and one per
// camera pass. Nought asserts: a device that can open no pass can draw nothing.
//
// AND IT IS SPENT ON LETTERS AS WELL AS ON FILLS, WHICH IS WHAT MAKES IT LARGER
// THAN IT LOOKS. A glyph is an element, so a forty-character label is forty of
// them and a space is none. An interface is therefore counted in characters
// rather than in widgets: a screen with a few dozen labels on it wants
// thousands, not hundreds, and the exhibit in dev/ spends half its eighty on two
// short lines of writing. Eighty bytes each, so a thousand is eighty kilobytes a
// frame slot — the number to be generous with, not careful about.
//
// targets IS THE THIRD NUMBER THAT MAY BE NOUGHT, and unlike the others it is
// not per frame slot: it is how many voe_render_target_create may make over the
// device's life, because nothing destroys one. A device made with none refuses
// the first create with a message. Each target costs two texture slots of the
// 1024 — its picture and its depth copy (ADR-0305) — as well as its images, and
// the window's depth copy takes one more. Each target also costs a probe grid
// of 3 × 32³ × 8 bytes, about 786 kB (ADR-0308), and the window has one too.
//
// shadow_size IS THE FOURTH THAT MAY BE NOUGHT: texels a side of each of the
// sun's VOE_RENDER_SHADOW_CASCADES depth maps, per frame slot (ADR-0258). It
// costs shadow_size² × 4 bytes × cascades × frame slots — 2048 is 128 MiB over
// two slots. Nought is no shadow pass at all; one texel stays for the binding.
typedef struct {
	uint32_t vertices;
	uint32_t indices;
	uint32_t geometries;
	uint32_t objects;
	uint32_t shadings;
	uint32_t transient_vertices;
	uint32_t transient_indices;
	uint32_t transient_geometries;
	uint32_t elements;
	uint32_t passes;
	uint32_t targets;
	uint32_t shadow_size;
} voe_render_capacities;

// How many depth maps the sun renders into, near to far: the layers of one frame
// slot's shadow image, and the range voe_render_shadow_pass_begin's cascade is in.
#define VOE_RENDER_SHADOW_CASCADES 4

// Texels a side of the sun's bounce map, per frame slot (ADR-0308): what
// voe_render_bounce_pass_begin draws into. 512² × 20 bytes × frame slots.
#define VOE_RENDER_BOUNCE_TEXELS 512

// Probes a side of a target's bounce grid, addressed toroidally (ADR-0308).
#define VOE_RENDER_BOUNCE_PROBES 32
// Metres between neighbouring probes of the bounce grid.
#define VOE_RENDER_BOUNCE_SPACING 2.0f
// Probes one bounce update refreshes beyond the cells a scroll brings in.
#define VOE_RENDER_BOUNCE_BUDGET 4096
// How much of a refreshed probe is new light, for one not just brought in.
#define VOE_RENDER_BOUNCE_BLEND 0.5f

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
//
// EVERYTHING IS NAMED BY A GENERATIONAL ID AND THE INDEX HALF IS THE NUMBER THE
// SHADER USES (ADR-0018). A texture id indexes the array the fragment stage
// samples; a shading id indexes the record buffer it reads. There is no table
// between a caller's id and the GPU's view of it, so there is nothing that can
// fall out of step — and a component that holds one of these holds two integers
// rather than a Vulkan handle, which is what lets it be written out as text,
// copied, sorted and read by a thread that may not touch Vulkan.
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

// How a texture is read, which is the second thing about a texture this API
// needs told and is entirely independent of the first.
//
// A KIND IS NOT A MODE AND NEITHER IMPLIES THE OTHER. A data texture is not
// automatically FIELD — a metallic-roughness or occlusion map is numbers and is
// still SMOOTH, because it is a picture on a surface like any other. The two
// questions are "what do these bytes mean" and "how is this texture read", and
// only the second one is here.
//
// NO PICTURE IN THIS ENGINE IS FILTERED, AND THAT IS NOT WHAT THESE NAMES
// SUGGEST. SMOOTH and SHARP are both NEAREST over one level, with no mipmap
// chain anywhere (card 026). What is genuinely left to choose between them is
// what happens outside 0..1 — REPEAT for a picture on a surface, CLAMP_TO_EDGE
// for a sheet something indexes into. The two names are older than that and
// promise more than they deliver; read the values below, not the words.
//
// FIELD IS THE ONE FILTERED MODE AND IT IS NOT A PICTURE. Interpolating between
// two colours is a blur, and a blur is the antialiasing card 026 removed.
// Interpolating between two distances is reconstruction of where the edge is,
// which is the whole of what makes a signed distance field a field rather than
// a grid of plateaus. That distinction is the rule: a texture asking for FIELD
// is asserting its texels are numbers on a continuum, and nothing whose texels
// are colours may ask for it.
//
// SMOOTH IS 0, SO A CALLER THAT MEANT NOTHING IN PARTICULAR GETS WHAT EVERY
// TEXTURE IN THIS ENGINE HAD BEFORE THERE WAS A CHOICE.
typedef enum {
	// NEAREST magnification and minification of one level, REPEAT. A
	// picture on a surface in the world, which is nearly everything.
	VOE_RENDER_SAMPLING_SMOOTH = 0,
	// NEAREST magnification and minification of one level, CLAMP_TO_EDGE.
	// A sheet something indexes into: it is not tiled, and REPEAT lets a
	// coordinate a hair outside one glyph's box wrap to the far side of the
	// atlas and fetch a different glyph entirely.
	VOE_RENDER_SAMPLING_SHARP,
	// LINEAR magnification and minification of one level, CLAMP_TO_EDGE.
	// For texture data that is numbers rather than a picture: a signed
	// distance field, and nothing else so far.
	//
	// POINT-SAMPLED, A FIELD IS A PLATEAU ACROSS EACH TEXEL AND ITS 0.5
	// CROSSING CAN ONLY FALL ON A TEXEL BOUNDARY. A stem two and a half
	// texels wide then comes out two or three depending on its phase
	// against the grid — one side of an O thicker than the other, at every
	// size including one that fills the screen. Filtered, the crossing
	// falls where the distances say it falls.
	//
	// IT SOFTENS NOTHING. What reads a field turns the distance into a
	// coverage — a hard cut for world text, one screen pixel of ramp for an
	// interface glyph (ADR-0269) — and the filter only makes that distance
	// true. This mode moves the edge; it does not blur it.
	VOE_RENDER_SAMPLING_FIELD,
} voe_render_sampling;

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
	// material references none. All five are sampled: the normal map tilts
	// a lit surface's normal through a tangent frame built per pixel from
	// screen derivatives (glTF's convention, no tangent attribute), and the
	// emissive map times `emissive` is added to a lit surface, unshadowed
	// (ADR-0278).
	//
	// A COLOUR TEXTURE AND A DATA TEXTURE ARE NOT INTERCHANGEABLE HERE. The
	// base colour and the emissive ones are uploaded as
	// VOE_RENDER_TEXTURE_COLOUR and the other three as
	// VOE_RENDER_TEXTURE_DATA, because the first pair holds sRGB-encoded
	// colour and the rest hold numbers. Putting an id from the wrong kind in
	// one of these slots is a picture that is subtly too dark or too flat
	// and nothing that fails.
	//
	// THE BASE COLOUR SLOT HAS EXACTLY ONE EXCEPTION AND THE FLAG BELOW
	// GOVERNS IT. A distance-field sheet is numbers and is uploaded as
	// VOE_RENDER_TEXTURE_DATA even though it sits in the base colour slot;
	// that is not the mistake this paragraph warns about, it is a different
	// thing being sampled. Anything else in this slot is still COLOUR.
	uint32_t base_colour_texture;
	uint32_t metallic_roughness_texture;
	uint32_t normal_texture;
	uint32_t occlusion_texture;
	uint32_t emissive_texture;
	// Non-zero: the base colour texture is not a picture at all. It holds
	// three signed distances per texel, and the fragment stage takes their
	// median, recovers how wide the edge is on screen from its own
	// derivative, and computes alpha from that; the colour is the base
	// colour factor, untouched by the sheet. Zero is an ordinary picture, so
	// a zeroed record is one.
	//
	// IT IS HERE RATHER THAN AT THE END BECAUSE THIS IS THE FIRST OF THE
	// THREE WORDS THAT USED TO BE reserved_c — same offsets, same size, and
	// the asserts in render/src/descriptors.c are unchanged. Cards 021a and
	// 021b did the same with reserved_a and reserved_b.
	//
	// WHAT IT DOES NOT SAY IS HOW THE SHEET IS FILTERED. That is
	// voe_render_sampling, chosen when the texture was created; a sheet that
	// asked for SMOOTH would be read out of a mipmap chain and the field
	// would be averaged into mush. The two go together and nothing checks
	// it.
	uint32_t base_colour_distance_field;
	// Non-zero: the water path (ADR-0305). The surface is a plane whose
	// normal is bent by four sine waves out of the object record's `waves`,
	// lit by the pass's sun with roughness 0.05, reflecting toward the
	// object's `sky` and faded by its thickness over the pass's depth copy.
	// Zero is every other surface, drawn as before. The second word that
	// used to be reserved_c; draw it blended.
	uint32_t water;
	uint32_t reserved_c;
	// Which rectangle of the base colour texture this record reads: `xy` is
	// the offset added and `zw` the scale multiplied, so the fragment stage
	// samples `uv * zw + xy`. One multiply-add, applied to the base colour
	// texture and to nothing else — the other four maps are read at the
	// vertex's own coordinates.
	//
	// IT IS WHAT MAKES A SHEET OF FRAMES ONE GEOMETRY AND ONE TEXTURE. The
	// quad's own coordinates are 0..1 and every frame in a sprite sheet
	// picks its own corner of the picture out with this; a static geometry
	// cannot change after it is created (see voe_render_geometry_create),
	// so UVs baked per frame would be a sprite that can never change frame
	// — and rebuilding a quad every frame through the transient path to
	// move four texture coordinates would be the wrong tool for it.
	// Changing frame is pointing an object at a different record, which
	// costs nothing — the record index is submitted every frame already.
	//
	// THE WHOLE TEXTURE IS (0, 0, 1, 1) AND NOT A ZEROED RECT, which is the
	// one field in here a zeroed record gets wrong: a scale of nothing reads
	// one texel across the whole surface. Nothing in this folder repairs
	// that, because repairing it would cost a branch or a rule at every
	// site that builds a record; whoever builds one says what it reads. For
	// everything above `render` that is voe_3d_material_upload, where a
	// material that says nothing gets the whole texture.
	voe_math_float4 base_colour_uv_rect;
} voe_render_shading_values;

typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_render_shading;

// The camera, for one pass: where it is and what it can see, already worked out
// by whoever owns those questions.
//
// Padded like the two records below and asserted on in render/src/descriptors.c,
// because it shares a buffer with the light and the shader reads both out of one
// block.
//
// THE CAMERA IS TWO MATRICES AND WHERE THE EYE IS. Where the camera is, how fast
// it moves and what a mouse does to it are `scene`'s; how a field of view
// becomes a projection is `3d`'s, because the reversed depth and the clip-space
// conventions are this folder's business and `3d` is the folder allowed to know
// both. What arrives here is the answer. The eye is beside the matrices because
// a specular highlight is a function of where the surface is being looked from,
// and digging it back out of the inverse of the view matrix in a shader would be
// arithmetic to recover a number the caller already had.
typedef struct {
	voe_math_float4x4 view;
	voe_math_float4x4 projection;
	// Where the eye is, in world space. The fragment stage wants it for the
	// direction a surface is being looked from.
	voe_math_float3 eye;
	float reserved;
} voe_render_view;

// The sun, for one pass.
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
// intensity leaves every surface black but for its fill, which is what a frame
// given a zeroed one looks like.
//
// `fill` IS THE ONE LIGHT THAT IS NOT THE SUN (ADR-0273). Linear, the colour
// already multiplied by its strength, and added as `fill × base colour` only
// where the sun does not reach, fading out as it does (ADR-0275, ADR-0276), not
// to every lit surface: a side facing away from the sun or in its shadow reads
// the fill and not black, a sunlit one the sun alone. Zero is no fill, and a
// zeroed record draws as before it existed. An `unshaded` pass reads none.
//
// ONE DIRECTIONAL LIGHT, HANDED OVER WITH THE CAMERA, ONCE A PASS. It is the
// sun: a direction, a colour and a strength, the same for every draw in the
// pass. There is no light list and no second light — many lights is a later
// card and it is the card that decides how they are gathered. The sun casts
// shadows through the pass's voe_render_shadow record below (ADR-0258); a pass
// whose light is `unshaded` reads no shadow either, because it reads no sun.
//
// NOTHING IS TONE MAPPED, SO BRIGHT VALUES CLIP. A light strong enough to push a
// surface past one is clamped by the target's format and the highlight goes
// flat white. That is expected and it is not a bug to work around at a call
// site by keeping intensities low; the card that maps a high-dynamic-range
// target down to a screen is the card that fixes it, and card 013's offscreen
// target is what makes it possible.
//
// `unshaded` NON-ZERO MEANS THIS PASS HAS NO SUN. Every surface then draws its
// own base colour, as an unlit material does, and direction, intensity, colour
// and fill are not read. A pass that wants base colours may ask for it; no folder
// sets it today (ADR-0290). Zero keeps shading, so a zeroed light is still the
// black one above, and that is what a scene with no light draws on its lit
// surfaces (ADR-0287). It is the four bytes that used to be padding.
typedef struct {
	voe_math_float3 direction;
	float intensity;
	voe_math_float3 colour;
	uint32_t unshaded;
	voe_math_float3 fill;
	float reserved;
} voe_render_light;

// The most point lights one pass carries (ADR-0320).
#define VOE_RENDER_POINT_LIGHTS 256

// One point light, for one pass (ADR-0320).
//
// `position` IS IN THE SPACE THE PASS'S DRAWS PLACE VERTICES IN — about the eye,
// as 3d's are — and `colour` is linear and already times the light's strength.
//
// IT CASTS NO SHADOW AND IT ENDS AT `range`. A surface d metres off is lit by
// colour × saturate(1 − (d/range)²)²: no inverse square, so a colour means what
// the sun's colour × intensity does, and past `range` the light is nothing.
//
// A pass carries at most VOE_RENDER_POINT_LIGHTS. Padded to 32 bytes because the
// shader reads an array of them.
typedef struct {
	voe_math_float3 position;
	float range;
	voe_math_float3 colour;
	float reserved;
} voe_render_point_light;

static_assert(sizeof(voe_render_point_light) == 32,
	      "a point light is two float4s, as the shader reads it");

// A pass's point lights: `count` of them at `lights`, at most
// VOE_RENDER_POINT_LIGHTS. NULL and nought is none.
typedef struct {
	const voe_render_point_light *lights;
	uint32_t count;
} voe_render_point_lights;

// Where the sun's shadow maps are, for one pass that reads them (ADR-0258).
//
// THE NUMBERS ARE 3d's. `cascades[i]` takes a position in the world the objects'
// matrices place things in — eye-relative, as they are — to cascade i's clip
// space: the same view × projection its shadow pass drew with. `splits[i]` is
// the far view distance cascade i covers, rising; `texels[i]` the metres one of
// its texels spans, which is how far a surface is pushed along its normal before
// it looks itself up, so it does not shadow itself. `count` is how many are in
// use, from cascade 0.
//
// NOUGHT IS NONE. A zeroed record — every existing `{ view, light }` initializer
// — reads no map and every surface is lit as it was. With `count` set, a lit
// surface picks the first cascade whose split is past its view distance, takes a
// filtered lookup there and scales the sun's direct light by it; past the last
// split it is fully lit. Unlit materials and an `unshaded` pass read nothing.
//
// Padded to sixteen bytes like the other blocks; draw.slang declares the split
// and texel arrays as float4s, which is the same sixteen bytes each.
typedef struct {
	voe_math_float4x4 cascades[VOE_RENDER_SHADOW_CASCADES];
	float splits[VOE_RENDER_SHADOW_CASCADES];
	float texels[VOE_RENDER_SHADOW_CASCADES];
	uint32_t count;
	uint32_t reserved0;
	uint32_t reserved1;
	uint32_t reserved2;
} voe_render_shadow;

// One drawn object's record: the two matrices it is drawn with, the shading
// record it wears and the colour it is tinted by. `shading` is the index half of
// a voe_render_shading id; the shader reads the record it names.
//
// `colour` IS LINEAR AND MULTIPLIES THE SHADING RECORD'S BASE COLOUR FACTOR,
// ALPHA INCLUDED. (1, 1, 1, 1) leaves the record as it is. A zeroed one draws
// black, so every call site sets it. It lives here and not in the shading record
// because this record is written per draw and a shading record never changes
// (ADR-0191).
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
//
// PER-OBJECT DATA GOES INTO ONE BUFFER PER FRAME SLOT, ONE RECORD PER DRAW. The
// world matrix, the matrix its normals want and which shading record to use are
// written by _draw as it records, and the shader reads its record by object
// number. Card 014's push constant for the model matrix is gone; the only push
// constant left is the object number itself.
typedef struct {
	voe_math_float4x4 world;
	voe_math_float4x4 normal;
	uint32_t shading;
	uint32_t reserved[3];
	voe_math_float4 colour;
	// Read only when the shading record is `water`; zero everywhere else.
	// `waves` is (height m, length m, seconds, deep m): the tallest wave's
	// height and length, the clock, and the thickness at which the water is
	// nearly opaque. The clock is kept below 60, because every wave's
	// frequency is a whole multiple of 2π/60 and so seconds 0 and 60 are the
	// same picture, while a float clock counting up for hours loses the
	// precision the waves need. `sky` is a linear rgb the surface reflects
	// toward at a grazing angle; w is reserved.
	voe_math_float4 waves;
	voe_math_float4 sky;
} voe_render_object;

// What one element is. Two kinds, and the field exists so that adding the second
// moved no other field and changed no size — which is what it was reserved for
// and what card 031 spent it on.
//
// SOLID IS 0, SO A ZEROED RECORD IS A SOLID ONE.
typedef enum {
	// A rectangle filled with the record's colour, and nothing sampled.
	VOE_RENDER_ELEMENT_SOLID = 0,
	// A rectangle whose coverage comes out of a distance-field sheet: the
	// record's `sheet` says which part of `sheet_texture` to read and the
	// fragment stage takes the median of three channels and turns it into a
	// coverage across one screen pixel (ADR-0269).
	// The record's colour is the whole of what the letter is coloured; there
	// is no material, no lighting and no shading record on this path, and
	// that is exactly what lets a letter and a fill be one draw command.
	//
	// IT IS NOT A "TEXT" KIND AND IS NOT NAMED ONE. What it does is read
	// coverage out of a sheet, which is what a glyph wants and what an icon
	// sheet would want as well.
	VOE_RENDER_ELEMENT_GLYPH,
	// A rectangle showing a picture: the record's `sheet` says which part of
	// `sheet_texture` is stretched over the element's bounds, and what is read
	// there is multiplied by the record's colour — a colour of opaque white
	// shows the picture as it is. What it is for is a view drawn into a target
	// of one's own, an icon, a thumbnail.
	//
	// IT IS NOT A TEXT KIND EITHER, AND IT IS NOT A GLYPH WITH THE COVERAGE
	// TAKEN OFF. A glyph's sheet is numbers and its colour is the record's
	// alone; an image's sheet is a picture — a COLOUR texture, or a target's —
	// and its colour is the picture's. Read NEAREST like every picture here.
	//
	// THE PICTURE'S ALPHA IS TREATED AS STRAIGHT, like the record's own colour,
	// and the product is premultiplied once at output. A target's picture is
	// premultiplied already, which makes no difference while it is opaque —
	// every target is cleared to an opaque colour — and makes a see-through
	// one too faint.
	VOE_RENDER_ELEMENT_IMAGE,
} voe_render_element_kind;

// One element: a rectangle, a colour and the rectangle it is clipped to. Many of
// these are drawn by one instanced draw, and the vertex shader builds the four
// corners itself — there is no vertex buffer and no index buffer on this path.
//
// A LETTER AND A FILL ARE THE SAME DRAW COMMAND, WHICH IS THE WHOLE CLAIM. A
// GLYPH record is the same eighty bytes as a SOLID one and goes into the same
// buffer in the same submission order, so a label of forty characters and the
// panel behind it are forty-one records and one draw. What differs is where the
// fragment stage gets its coverage: a solid has one everywhere and a glyph
// reads it out of a distance-field sheet. Nothing in here lays a string out —
// where each character sits is the caller's, out of voe_text's metrics.
//
// IT IS NOT A GUI TYPE AND IS DELIBERATELY NOT NAMED LIKE ONE. A user interface
// is the first caller and not the only plausible one: debug lines and sprites
// want the same "many small things, one draw" shape, and nothing in here knows
// what a panel or a button is.
//
// THE CPU WRITES ONE OF THESE INSTEAD OF FOUR VERTICES AND SIX INDICES, AND THAT
// IS THE WHOLE POINT. Four voe_render_vertex plus six indices is a hundred and
// fifty-two bytes and a triangulation; this is eighty bytes and a struct
// assignment. And because the colour is in the record rather than in a shading
// record, every element in the draw may differ without breaking it — which is
// what makes a whole interface one draw call, and is impossible on the mesh path
// where one draw means one shading record.
//
// EVERYTHING IN HERE IS IN THE SURFACE'S OWN TWO-DIMENSIONAL SPACE, IN
// MILLIMETRES (ADR-0089: a GUI unit is 1 mm). Not pixels and not world metres.
// What turns that space into clip space is the transform handed to
// voe_render_frame_draw_elements, and voe_render_element_transform is the one
// place the conversion is written down.
//
// Y RUNS DOWN. The origin is the surface's top-left corner and y increases
// towards the bottom, which is what every interface in the world means by a
// coordinate. That is NOT what the engine's one Y flip gives on its own — the
// negative viewport height in render/src/frame.c makes +Y clip space point *up*
// the screen — so the sign lives in voe_render_element_transform and in nothing
// else. There is still exactly one Y flip in this engine and this is not a
// second one.
//
// THE LAYOUT IS PADDED AND THAT IS NOT COSMETIC, for exactly the reason
// voe_render_shading_values is: the same struct is declared in
// shaders/elements.slang and the two only agree for certain when every member
// lands on the boundary a buffer layout rule would have put it on. The padding
// is written as separate scalars where it follows a single word, because a uint3
// would be realigned and three uints are not.
//
// AND THERE IS A THIRD PIPELINE THAT DRAWS NO MESHES AT ALL. Everything above
// is one draw per thing drawn, out of the geometry pools. voe_render_element is
// the other shape: the caller writes one small record per rectangle, the vertex
// shader builds the four corners from its own vertex index, and every rectangle
// in the frame is drawn by ONE instanced draw — no vertex buffer, no index
// buffer and no shading record on that path. It is what makes a whole user
// interface one draw call, and it is not a user interface type: debug lines and
// sprites want the same "many small things, one draw" shape. See
// voe_render_frame_submit_element.
typedef struct {
	// Where the rectangle is: `xy` its top-left corner and `zw` its width
	// and height, in millimetres. A width or height of nothing draws
	// nothing, which is not checked because it is not wrong.
	voe_math_float4 bounds;
	// What the element is clipped to, in the same space and the same
	// xy-wh shape: a fragment outside it is discarded.
	//
	// IT IS HERE WITH NO CALLER YET, ON PURPOSE. A scroll area is what
	// wants it, and a clip rectangle arriving later would change this
	// record's size and every draw that had been built against it. Because
	// it is per element rather than per draw, a clipped region costs no
	// draw-call break — which is the whole reason it is a field and not a
	// scissor.
	//
	// A ZEROED RECT CLIPS EVERYTHING AWAY, and that is the one field in
	// here a zeroed record gets wrong — the same trade
	// voe_render_shading_values.base_colour_uv_rect makes. Nothing in this
	// folder repairs it, because repairing it would cost a branch at every
	// fragment to fix a caller that did not say; whoever builds a record
	// says what it is clipped to. An element that is not meant to be
	// clipped is given its own bounds, or the whole surface.
	voe_math_float4 clip;
	// Linear RGBA, straight and NOT premultiplied: the shader multiplies rgb
	// by a once, at output, because the colour target holds premultiplied
	// colour (ADR-0069). A caller that premultiplies as well gets an element
	// that is too faint, which reads as a wrong colour rather than as a bug.
	//
	// A PALETTE INDEX INSTEAD OF A COLOUR IS STILL AN OPEN QUESTION AND
	// NOTHING HAS ANSWERED IT. The two words still reserved below are where
	// one would go.
	//
	// A GLYPH'S COLOUR IS THIS AND NOTHING ELSE, multiplied by the coverage
	// the sheet produced. There is no tint from a material and no light on
	// it, because a letter is a shape somebody chose the colour of.
	voe_math_float4 colour;
	// A voe_render_element_kind.
	uint32_t kind;
	// The sheet a GLYPH reads its coverage out of, or the picture an IMAGE
	// shows: a texture id's index half
	// only, the same shape voe_render_shading_values' texture slots use.
	// A SOLID never reads it and does not have to set it.
	//
	// VOE_RENDER_NO_TEXTURE IS SLOT 0 AND SLOT 0 IS ONE WHITE PIXEL, so a
	// glyph that forgot to name its sheet samples opaque white, medians to
	// white, is fully covered and DRAWS A SOLID RECTANGLE. That is the
	// engine's standing convention for the empty texture id and nothing here
	// tests for it; it is written down because a plausible-looking rectangle
	// is a worse failure to find than a blank one, and
	// render/tests/elements.c asserts on it so it cannot change quietly.
	uint32_t sheet_texture;
	// Room to grow, and part of the reason this record is eighty bytes rather
	// than sixty-four. The same thing voe_render_shading_values did with
	// reserved_a, _b and _c, which three later cards consumed in place; card
	// 031 took one word of these three and the float4 below.
	uint32_t reserved_a[2];
	// What part of `sheet_texture` a GLYPH or an IMAGE reads, in texture
	// coordinates:
	// `xy` its top-left corner and `zw` its width and height. The record's
	// own xy-plus-wh shape, the same as `bounds` and `clip`, because one
	// struct wants one convention.
	//
	// A FONT HOLDS THIS AS A MIN/MAX PAIR AND THE CALLER CONVERTS. voe_text's
	// per-character metrics are a low corner and a high corner, and turning
	// that into a corner and a size is the caller's one line — see
	// voe_text_glyph, which already hands the sheet rectangle over in this
	// shape for exactly that reason.
	//
	// It is one rectangle for the whole element, like `clip`, and the shader
	// treats it as one. A SOLID never reads it.
	voe_math_float4 sheet;
} voe_render_element;

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
// Geometry built while drawing is the other call, below, and a different
// mechanism: it never waits and never stages, because what it writes lives one
// frame.
//
// GEOMETRY LIVES IN TWO SHARED POOLS AND A MESH IS A RANGE IN THEM. One vertex
// pool and one index pool, created with the device; voe_render_geometry_create
// takes the first range given back that fits, else appends, and
// voe_render_geometry_destroy below gives both ranges back. That layout is also
// what lets many objects be drawn from one buffer later, with one indirect call
// instead of one call each.
//
// AND THERE ARE TWO LIFETIMES OF GEOMETRY, WHICH THE ID DOES NOT TELL APART.
// voe_render_geometry_create is the startup one above: uploaded once, kept until
// destroyed. voe_render_geometry_create_transient is the other: built inside a frame,
// drawn in that frame, gone at the end of it. Both hand back a voe_render_geometry
// and both go through the same two draw calls, so what holds one never has to
// know which kind it holds — a transient id used a frame late is simply refused,
// by the same generation check that refuses any other stale id.
[[nodiscard]] bool voe_render_geometry_create(voe_render_device *device,
					      const voe_render_vertex *vertices,
					      uint32_t vertex_count,
					      const uint32_t *indices,
					      uint32_t index_count,
					      voe_render_geometry *out,
					      voe_base_error *error);

// Gives a static mesh's vertex and index ranges back to their pools and its slot
// back with the generation bumped, so every copy of the id is refused from here
// on. False when the id was already stale, which is not an error: somebody else
// got here first. A transient id is the caller's bug and asserts — it is gone at
// the next frame's begin. A startup operation: it waits for the GPU to go idle.
bool voe_render_geometry_destroy(voe_render_device *device,
				 voe_render_geometry geometry);

// The same shape, for geometry that lives one frame: copies the vertices and the
// indices into this frame's own pool and hands back an id that names them until
// voe_render_frame_end. The next frame's begin makes every id this handed out
// stale, and a draw with one is then refused like any other stale id. Nothing is
// freed and nothing needs to be — the pool is emptied and written again.
//
// IT REQUIRES AN OPEN FRAME AND ASSERTS WITHOUT ONE, WHICH IS THE MIRROR IMAGE
// OF THE CALL ABOVE. voe_render_geometry_create is a startup operation: it waits
// for the card to go idle and may not be called while a frame is being drawn.
// This one is a frame operation: it writes into the pool belonging to the slot
// voe_render_frame_begin picked, and that slot is not known — and its memory is
// not known to be free of the card — until _begin has run. Call it between
// _begin and _end, on a frame whose `drawing` came back true, and nowhere else.
//
// THE CALLER BUILDS THE ARRAYS AND THIS COPIES THEM. Handing back a pointer into
// the pool to write through would save the copy and cost every caller the rule
// that write-combined memory must never be read back; the copy here is out of
// memory the caller has just written and is cache-hot.
//
// FAILS, RETURNED AND NOT FATAL, WHEN EITHER TRANSIENT POOL IS FULL OR NO
// TRANSIENT SLOT IS LEFT FOR THIS FRAME — with VOE_BASE_ERROR_REFUSED and a
// line on stderr naming the numbers, because the answer is always that a
// transient capacity was chosen too small and never that something broke. The
// frame is otherwise untouched and every draw in it still goes through. A count
// of zero is the caller's bug and asserts, as it is above.
[[nodiscard]] bool voe_render_geometry_create_transient(
	voe_render_device *device, const voe_render_vertex *vertices,
	uint32_t vertex_count, const uint32_t *indices, uint32_t index_count,
	voe_render_geometry *out, voe_base_error *error);

// ---------------------------------------------------------------- textures

// Upload width * height RGBA8 pixels and hand back the id that names them.
// `kind` says whether the bytes are colour or numbers — see
// voe_render_texture_kind, which is the one thing here that is easy to get wrong
// and impossible to see. `sampling` says how the picture is addressed
// outside 0..1 — see voe_render_sampling. Every texture is one level sampled
// NEAREST whichever is passed; there are no mipmaps in this engine and no linear
// filtering anywhere.
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
					     voe_render_sampling sampling,
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
//
// A TARGET'S TEXTURE IS NOT ONE OF THESE TO GIVE BACK, and handing its id here
// asserts: the target owns the images the slot reads, and nothing destroys a
// target.
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

// Gives a record's slot back with the generation bumped, so the id is refused
// from here on. False for a stale id, as voe_render_texture_destroy. A startup
// operation: it waits for the GPU to go idle.
bool voe_render_shading_destroy(voe_render_device *device,
				voe_render_shading shading);

// ------------------------------------------------------------------ frames

// Starts the frame: rebuilds what a resize invalidated, waits for the slot this
// frame will use, takes a swapchain image, and begins recording. It draws nothing
// and binds no camera — every draw is inside a pass, below.
//
// `drawing` COMES BACK FALSE WHEN THERE IS NOTHING TO DRAW INTO — a window with
// no area, or a swapchain that has just gone stale. That is not an error: no
// passes may be opened, no draws issued, _end must not be called, and the next
// frame will find it has come back. It also does not wait, so a loop that does
// nothing else will spin.
//
// False means this device cannot draw any more and the program should stop
// asking. Everything a frame can hit that a retry fixes is handled here.
//
// IT DRAWS TO AN IMAGE OF ITS OWN AND COPIES THAT TO THE WINDOW. The frame is
// cleared and drawn into an offscreen colour target and putting that on screen
// is a separate last step — which is what any post process, a render resolution
// the window is not, and an editor viewport all need in order to be possible at
// all.
//
// SIZE IS PASSED IN, EVERY FRAME, AND IT IS THE WINDOW'S ANSWER. This folder
// never asks a window how big it is: `platform` owns that truth and the caller
// already has it. A window that has changed size is not an event here — the size
// simply differs from the one the targets were built at, and the next frame
// rebuilds them.
//
// A FRAME IS A SEQUENCE OF PASSES, AND THE CAMERA BELONGS TO THE PASS (ADR-0148).
// voe_render_frame_begin opens the frame — the slot, the swapchain image, the
// recording — and draws nothing; every draw happens inside a pass, opened onto a
// target with voe_render_pass_begin and closed with _pass_end. The camera moved
// off the frame because one frame may want to look at the world more than once —
// two views of one scene, a picture drawn for somewhere other than the window —
// and a camera fixed for the whole frame makes every one of those impossible. A
// pass draws into the window's target, VOE_RENDER_TARGET_WINDOW, or into a target
// of the caller's own made with voe_render_target_create.
[[nodiscard]] bool voe_render_frame_begin(voe_render_device *device,
					  voe_platform_size size, bool *drawing);

// What a pass draws into. The shape of the other ids; the zeroed one is the
// window's, and every other one came out of voe_render_target_create.
typedef struct {
	uint32_t index;
	uint32_t generation;
} voe_render_target;

#define VOE_RENDER_TARGET_WINDOW ((voe_render_target){ 0 })

// ----------------------------------------------------------------- targets

// Makes a target of `width` by `height` pixels that a pass can draw into, and
// hands back two ids: the target, for voe_render_pass_begin, and an ordinary
// colour texture that shows its picture — on an element of kind
// VOE_RENDER_ELEMENT_IMAGE, or in a shading record's base colour slot.
//
// A TARGET IS KEPT AND NOT ASKED FOR PER FRAME. What draws into one — a second
// view of a scene, a thumbnail — is drawn every frame into the same place, and
// what shows it holds a texture id in a record or a component that was written
// once. A target made per frame would be an allocation and a descriptor rewrite
// every frame, both of which wait for the card, and a texture id that changed
// under everything holding it.
//
// THE TEXTURE ID NEVER CHANGES, AND WHICH IMAGE IT READS IS THIS FOLDER'S
// BOOKKEEPING. A target is one colour-and-depth pair per frame slot, for the
// reason the window's is: the frame before last may still be reading its own.
// The id names one texture slot, and a frame reads its own slot's image through
// it; nothing outside this folder can tell, across frames in flight or resizes.
//
// ITS PICTURE IS UNDEFINED UNTIL A PASS DRAWS INTO IT, AND AGAIN AFTER A RESIZE.
// Showing a target nothing has drawn shows whatever the card had. Nothing is
// cleared on creation, because the first pass onto it clears it.
//
// A TARGET IS CLEARED BY THE FIRST PASS ONTO IT IN A FRAME AND LOADED BY EVERY
// LATER ONE, colour and depth both — the window's rule, and the same clear
// colour. A frame that opens no pass onto a target leaves it holding what it
// held, which is what the frame slot's own image held: the picture of the last
// frame that ran on that slot — frames in flight back, as
// voe_render_frame_gpu_time's reading is — and not the last frame's. A target
// that is shown is drawn every frame it is shown.
//
// SHOWING A TARGET IN A PASS THAT DRAWS INTO IT ASSERTS, IN A DEBUG BUILD. A
// mesh whose shading record names the open pass's target texture in any of its
// texture slots, or an element range any of whose GLYPH or IMAGE records does,
// is a picture reading itself while it is written, which Vulkan leaves
// undefined. It is a debug check: a release build reads the records nowhere
// and draws whatever the card does.
//
// FALSE, WITH A LINE ON stderr, WHEN `targets` ARE ALL TAKEN — including on a
// device made with none — WHEN TWO TEXTURE SLOTS ARE NOT LEFT, OR WHEN THE CARD REFUSES
// THE IMAGES. A width or height of zero is the caller's bug and asserts.
//
// IT WAITS FOR THE GPU TO GO IDLE, SO IT IS A STARTUP OPERATION, for the reason
// voe_render_texture_create is: the descriptor sets it rewrites may not be
// touched while a frame is reading them. Inside an open frame it asserts.
[[nodiscard]] bool voe_render_target_create(voe_render_device *device,
					    uint32_t width, uint32_t height,
					    voe_render_target *out_target,
					    voe_render_texture *out_texture,
					    voe_base_error *error);

// Asks for `target` to be `width` by `height`. Nothing happens here: the size is
// recorded and applied at the top of the next voe_render_frame_begin, where the
// window's own rebuild happens — so calling this inside a frame is allowed and
// takes effect next frame, and a size asked for twice before then is the second
// one. The size it already has does nothing.
//
// A RESIZE WAITS FOR THE GPU AS THE WINDOW'S DOES, because every slot's images
// are thrown away and made again and the descriptor sets pointed at the new
// ones. The texture id is unchanged and the picture is undefined until a pass
// draws into it again. A card that refuses the new images fails that
// _frame_begin, as a window that cannot be rebuilt does.
//
// A width or height of zero, or an id that names no target, asserts.
void voe_render_target_resize(voe_render_device *device, voe_render_target target,
			      uint32_t width, uint32_t height);

// A target's picture in memory: `width` by `height` pixels, RGBA8, `pixels`
// pointing at width * height * 4 bytes.
typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t *pixels;
} voe_render_picture;

// Copies what `target` holds into `arena` and describes it in `out`. This is the
// way back from the card: a pass draws a frame into a target and nothing else
// brings those pixels to memory.
//
// RGBA8, IN THAT BYTE ORDER, WHATEVER THE IMAGE IS MADE OF. The colour image is
// an sRGB four-byte format whose channel order is the surface's business — it is
// B8G8R8A8 on most cards — and the swap into RGBA happens here because this is
// the only folder that knows which it got. RGBA8 is also what `assets` decodes a
// picture to, so the two halves of saving a picture meet with nothing between
// them.
//
// ROW ZERO IS THE TOP ONE, which is what PNG, Vulkan and glTF already agree on.
//
// STRAIGHT ALPHA, NOT PREMULTIPLIED. The colour target holds premultiplied
// colour (see voe_render_frame_draw_blended) and PNG holds straight, so each channel is
// divided by its alpha here, at the one place that knows which convention the
// image is in. A pixel whose alpha is 0 comes back as transparent black rather
// than as a division by nothing. The clear colour is opaque, so in practice
// every pixel divides by one.
//
// NOTHING APPLIES A GAMMA. The bytes are already sRGB-encoded — the format
// carries the curve — so the colours handed back are the colours drawn, and a
// file written from them is the picture that was on the screen.
//
// IT WAITS FOR THE CARD TO GO IDLE, SO IT IS NOT A PER-FRAME CALL. It is
// voe_render_target_create's kind of operation: taking a picture stalls the
// pipeline, which is what a capture made between frames can afford and what a
// capture made every frame cannot. Calling it inside an open frame asserts, as
// does a target id that names no target, a NULL arena and a NULL `out`.
//
// IT READS THE SLOT THE FRAME THAT ENDED LAST DREW INTO, so a caller gets the
// frame it has just drawn. Before any frame has ended, and after a resize, the
// picture is undefined — the rule a target's picture already has — and the call
// still succeeds.
//
// VOE_RENDER_TARGET_WINDOW WORKS HERE LIKE ANY OTHER TARGET. On a windowed
// device that is the offscreen pair the frame is blitted to the window from, and
// on a headless device it is the same pair with nothing to blit to, which is
// what makes a saved picture and a shown one the same picture rather than two
// paths that ought to agree. Its size is the resolution the engine draws at,
// which is the window size its images were last built for.
//
// FALSE, WITH A LINE ON stderr AND VOE_BASE_ERROR_REFUSED, WHEN THE CARD REFUSES
// THE STAGING BUFFER OR THE COPY — and when the target has no pixels at all,
// which is what the window's is while its area is nought.
[[nodiscard]] bool voe_render_target_read(voe_render_device *device,
					  voe_render_target target,
					  voe_base_arena *arena,
					  voe_render_picture *out,
					  voe_base_error *error);

// The camera, the sun and the sun's shadow one pass draws with. Handed over
// together because they land in one block the shader reads, and a pass that has
// one has all three; a zeroed `shadow` is none.
typedef struct {
	voe_render_view view;
	voe_render_light light;
	voe_render_shadow shadow;
} voe_render_pass_camera;

// Opens a pass onto `target`, drawn with `camera` — which may be NULL for a pass
// that draws only elements; see below. The target's colour and
// depth are cleared if this is the first pass onto it this frame and loaded
// otherwise. The camera is copied and the caller's is its own again the moment
// this returns. The viewport and the scissor are the target's own size, the
// window's or the one voe_render_target_create was given.
//
// FALSE WHEN THIS FRAME HAS ALREADY OPENED `passes` PASSES, with a line naming
// the numbers — a capacity chosen too small, on the same terms as every other
// one. No pass is open then and the frame is otherwise untouched; the next frame
// starts counting again.
//
// Calling this outside a frame whose `drawing` came back true, with a pass
// already open, or with a target id that names no target, is the caller's bug
// and asserts.
//
// THE CAMERA MAY BE NULL, BECAUSE NOT EVERY PASS LOOKS AT A WORLD. A pass that
// draws only elements — an interface filling the window — has no eye and no sun,
// and inventing a zeroed pair for it would be a camera that means nothing held in
// a buffer the shader reads. The mesh draws and the depth clear assert on a pass
// with no camera; the element draw does not need one.
//
// THE WINDOW'S TARGET IS CLEARED BY THE FIRST PASS ONTO IT IN A FRAME AND LOADED
// BY EVERY LATER ONE, colour and depth both. So a second pass draws over the
// first and is hidden by whatever the first drew nearer — which is what makes two
// passes onto one target a sequence and not two separate pictures. A frame that
// opens no pass onto the window still presents the clear colour.
//
// PASSES DO NOT NEST. One is open at a time: a _pass_begin with one open asserts,
// and so does a _frame_end — except an open bounce pass, which either closes.
[[nodiscard]] bool voe_render_pass_begin(voe_render_device *device,
					 voe_render_target target,
					 const voe_render_pass_camera *camera);

// Opens a shadow pass onto cascade `cascade` of this frame slot's shadow map: its
// depth cleared to the far plane, no colour, the viewport the map's size with the
// same one Y flip every pass has. `light` is the camera block's view — the sun's
// view and projection — and the block's sun is zeroed. Mesh draws in it write
// depth only; closed by voe_render_pass_end like any pass (ADR-0258).
//
// IT IS A PASS AND COUNTS AGAINST `passes`, its draws against `objects`. False,
// with a line, when the frame's passes are spent; nothing is open then.
//
// Calling this outside a frame, with a pass already open, with a cascade not below
// VOE_RENDER_SHADOW_CASCADES, or on a device whose shadow_size is nought is the
// caller's bug and asserts.
[[nodiscard]] bool voe_render_shadow_pass_begin(voe_render_device *device,
						uint32_t cascade,
						const voe_render_view *light);

// Opens a bounce pass: one more pass of the sun, after the cascades, onto this
// frame slot's bounce map (ADR-0308), VOE_RENDER_BOUNCE_TEXELS square, flux,
// normal and depth cleared. `light` is the view the map is drawn from and `sun`
// the light whose flux it holds. The draws in it are the casters, recorded as in
// a shadow pass with the same voe_render_frame_draw, each writing the sun's colour
// × intensity × its base colour and its world normal; they count against
// `objects`, and the pass against `passes`.
//
// THE NEXT PASS OR THE FRAME'S END CLOSES IT; voe_render_pass_end may as well.
// The last one this frame is what a bounce update reads.
//
// False, with a line, inside no frame or when the frame's passes are spent;
// nothing is open then. With another kind of pass open it asserts.
[[nodiscard]] bool voe_render_bounce_pass_begin(voe_render_device *device,
						const voe_render_view *light,
						const voe_render_light *sun);

// Closes the open pass, a shadow or bounce pass included. Without one open it
// asserts.
void voe_render_pass_end(voe_render_device *device);

// What a bounce update needs from the caller (ADR-0308 point 4). `cell` is the
// grid's lowest world cell at VOE_RENDER_BOUNCE_SPACING, `corner` that cell's
// lowest corner about the eye; `stale` holds `stale_count` spheres about the
// eye, xyz centre and w radius, whose probes are refreshed early. A struct tag
// and no typedef, because the call below has the name and C has one name space
// for both.
struct voe_render_bounce_update {
	int32_t cell[3];
	voe_math_float3 corner;
	const voe_math_float4 *stale;
	uint32_t stale_count;
};

// Refreshes `target`'s bounce grid from this frame's last bounce pass: the map
// reduced to virtual point lights once a frame, then the probes the grid's
// schedule lists gathered from them and blended in. Recorded between passes;
// an open bounce pass is closed first.
//
// A CAMERA PASS ON THAT TARGET LATER THIS FRAME READS THE GRID; one on a target
// with no update this frame reads no bounce.
//
// False, with a line, outside a frame, inside a camera or shadow pass, with no
// bounce pass this frame, for a target that is not live, and on a second call
// for one target in one frame: at most once a target a frame.
[[nodiscard]] bool voe_render_bounce_update(voe_render_device *device,
					    voe_render_target target,
					    const struct voe_render_bounce_update *update);

// Whether a pass is open: true from a _pass_begin that returned true until its
// _pass_end. It exists so that a caller which issues draws on behalf of another —
// the draw system in `3d` — can assert the same thing every draw here asserts,
// before it has walked anything.
[[nodiscard]] bool voe_render_pass_is_open(const voe_render_device *device);

// Draws one range with one object record, in the order the calls are made. The
// record is written into this slot's object buffer and the object's number is
// what the shader is told; both are this folder's bookkeeping and neither is a
// number the caller has to keep.
//
// False when this frame already holds as many objects as the device was made
// for, or when the geometry id names nothing. Neither is retryable inside this
// frame.
//
// Calling this with no pass open, or in a pass opened with no camera, is the
// caller's bug and asserts.
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
// IN A SHADOW PASS IT DRAWS DEPTH ONLY, through the shadow pipeline: the same
// vertex stage, no fragment stage, nothing culled, depth biased away from the
// sun. The record's world matrix is all that is read. IN A BOUNCE PASS it draws
// through the bounce pipeline, nothing culled, into the bounce map.
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
// asserts — and one more: in a shadow or bounce pass it asserts, because nothing
// see-through casts.
//
// THE COLOUR TARGET HOLDS PREMULTIPLIED COLOUR, AND ANYTHING THAT WRITES INTO IT
// OUTPUTS PREMULTIPLIED COLOUR. A fragment's rgb is already multiplied by its
// own alpha by the time it leaves a shader, and the blend is therefore
// source-one, destination-one-minus-source-alpha for colour and alpha both.
// Nothing enforces this — there is no validation layer for it and no test that
// can see it — so the next shader author reading this sentence is the whole
// mechanism. shaders/draw.slang says the same thing at the site that does the
// multiply.
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
// pass still has exactly one rendering block and there is nothing here to tear
// down or resume. See voe_3d_draw_system_run, which is still the one caller,
// and which calls this once between the world and the overlay and again before
// the gizmo it draws last — each clear one more group in front of everything
// before it.
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
// Calling this with no pass open, or in a pass opened with no camera, is the
// caller's bug and asserts — the same mistakes voe_render_frame_draw asserts on.
// So is calling it in a shadow or bounce pass, whose depth is the map being drawn.
void voe_render_frame_clear_depth(voe_render_device *device);

// Copies the open pass's depth, as it stands, into the sampled depth copy every
// target keeps (ADR-0305), and names the copy's texture slot in the pass's
// camera block; until a pass copies, that word is ~0u.
//
// ONLY INSIDE AN OPEN CAMERA PASS. With no pass open, in a pass opened with no
// camera, or in a shadow or bounce pass — whose depth is the map itself — it draws
// nothing, writes a line on stderr and returns false.
//
// IT SPLITS THE PASS'S RENDERING BLOCK. It ends the block, copies with a barrier
// on either side, and resumes in a block that loads colour and depth, so
// everything drawn before it stays and keeps occluding. Draws after it read the
// copy; a draw does not read the depth it is writing.
//
// CALLING IT AT MOST ONCE A PASS IS THE CALLER'S BUSINESS. Nothing stops a
// second call, and each call costs a full copy of the depth image.
[[nodiscard]] bool voe_render_frame_copy_depth(voe_render_device *device);

// Whether a frame is open for drawing: true from a _begin whose `drawing` came
// back true until its _end. Not a way to ask whether to begin a frame: the loop
// that owns the frame already knows.
[[nodiscard]] bool voe_render_frame_is_open(const voe_render_device *device);

// Ends the recording, submits it, and — where there is a window — copies the
// target into the acquired swapchain image and presents it. An open bounce pass
// is closed first; any other pass still open is the caller's bug and asserts.
//
// False means the driver refused something no retry will fix. A swapchain that
// went stale is handled here and returns true, with the rebuild happening at the
// top of the next frame.
[[nodiscard]] bool voe_render_frame_end(voe_render_device *device);

// How many draw commands the open recording holds — or, once _end has run, how
// many the frame that was just submitted held. Reset by every _begin.
//
// IT EXISTS TO BE READ RATHER THAN TRUSTED. "Many small things in one draw" is a
// claim about the number of draw commands and nothing else, so a program that
// makes it says this number rather than saying it drew one instanced draw.
// Every mesh drawn is one command; every element draw is one command whatever
// it holds.
[[nodiscard]] uint32_t
voe_render_frame_draw_count(const voe_render_device *device);

// ---------------------------------------------------------------- elements

// Puts one element into this frame's own buffer, to be drawn by the call below.
// The record is copied and the caller's is its own again the moment this
// returns.
//
// ORDER IS PAINT ORDER. Elements blend in the order they were submitted, so a
// caller that wants one on top of another submits it second. That is a
// guarantee this path makes and the mesh path does not — see
// voe_render_frame_draw_blended, where the caller has to sort by distance
// because depth decides.
//
// False when this frame already holds as many elements as the device was made
// for, with a line on stderr naming the numbers — which is also the answer on a
// device opened with `elements` at nought. The frame is otherwise untouched:
// every element that fitted is still drawn, and the next frame starts empty.
//
// Calling this without a _begin that set `drawing`, or after _end, is the
// caller's bug and asserts.
//
// ELEMENT SUBMISSION IS FRAME-WIDE AND ELEMENT DRAWING IS PER PASS. The records
// go into one buffer that belongs to the frame, not to any pass, so a range read
// with voe_render_frame_elements_submitted may be drawn in whichever pass wants
// it — submitted before the first pass opens, or inside another one.
[[nodiscard]] bool voe_render_frame_submit_element(voe_render_device *device,
						   voe_render_element element);

// How many elements have been submitted to the open frame so far — or, once
// _end has run, how many the frame that was just submitted held. Every _begin
// puts it back to nought, exactly as it empties the buffer it counts.
//
// IT IS HOW A CALLER LEARNS ITS OWN RANGE, AND IT IS THE WHOLE PROTOCOL. Read
// it, submit one surface's records, read it again: the first number is `first`
// and the difference is `count`, which is what the draw below takes. There is
// no id, no handle and nothing allocated — a range is two numbers about one
// buffer that belongs to one frame and is empty again in the next.
//
// IT IS NOT voe_render_frame_draw_count AND THE TWO ARE NEVER THE SAME NUMBER.
// This counts records submitted; that one counts draw commands recorded. Forty
// rectangles in one draw is forty here and one there, and that gap is the whole
// claim of this path — reading the wrong one of the two turns the claim into
// something trivially true.
[[nodiscard]] uint32_t
voe_render_frame_elements_submitted(const voe_render_device *device);

// Draws `count` of this frame's elements, starting at `first`, in one instanced
// draw, with `transform` turning the elements' millimetres into clip space. A
// count of nought records nothing.
//
// A RANGE AND NOT THE WHOLE BUFFER, BECAUSE ONE BUFFER HOLDS EVERY SURFACE THE
// FRAME DRAWS. Two panels in two places are two ranges of the one buffer, two
// matrices and two draw commands; the ranges come from
// voe_render_frame_elements_submitted, read either side of the submissions
// belonging to one surface. Nothing about a range survives the frame it was
// taken in.
//
// `transform` IS THE ONE PLACE THE SURFACE'S GEOMETRY LIVES, AND IT IS A
// PARAMETER BECAUSE THIS FOLDER CANNOT KNOW IT. What size the surface is, where
// it is and whether it is flat against the screen or standing in the world is a
// question about a panel, and a panel is not a thing `render` has ever heard of.
// voe_render_element_transform below builds the one this engine wants today.
//
// IT IS BLENDED, TESTS DEPTH AND WRITES NONE, exactly as
// voe_render_frame_draw_blended does — so an element behind an opaque object is
// hidden by it, and two elements never hide each other.
//
// False in two situations. The pipeline this needs was never built, which is a
// device that could not be opened rather than anything a caller did. Or the
// range runs past what has been submitted to this frame — which is returned and
// not asserted, because ordinary staleness reaches it: a caller holding a range
// from a frame that has already gone is a surface nobody rebuilt this frame, and
// costing it one draw is better than stopping the program. The line on stderr
// names the numbers.
//
// It needs a pass open and does not need that pass to have a camera: the
// transform is the whole of what places the elements. Calling it with no pass
// open, or in a shadow or bounce pass, which has no colour to blend into, is the
// caller's bug and asserts.
[[nodiscard]] bool
voe_render_frame_draw_elements(voe_render_device *device,
			       voe_math_float4x4 transform, uint32_t first,
			       uint32_t count);

// Element space onto the surface's own plane: millimetres in, metres out, the
// surface centred on its own origin and Y the right way up.
//
// IT IS THE PIECE OF A PANEL'S MATRIX THAT BELONGS TO THIS FOLDER, AND ONLY
// THAT PIECE. Where a surface stands, which way it faces and what camera looks
// at it are questions about a scene, which this folder has never heard of; what
// element space is and which way its Y runs is this folder's alone. So a caller
// drawing a surface standing in the world composes projection × view × model ×
// this and hands the product to the draw above — see 3d/panel_component.h,
// which is the first caller that does.
//
// IT OWNS THE ELEMENT PATH'S Y NEGATION AND THERE IS NO OTHER. Element space
// runs y downwards from the surface's top-left corner, because that is what
// every interface in the world means by a coordinate, and the engine's world
// runs +Y up. The two meet here, once. voe_render_element_transform below is
// built on top of this rather than beside it, precisely so that the sign is
// written down in one function; there is still exactly one Y flip in this
// engine, in the viewport, and neither of these is a second one.
//
// ONE MILLIMETRE IS ONE MILLIMETRE (ADR-0089), so a 240 mm surface is 0.24 m
// across before the caller's own model matrix says anything. That factor is the
// only number in here, and a panel that came out the size of a wall or too small
// to find is it inverted.
//
// THE SURFACE IS CENTRED ON ITS ORIGIN, AND THAT IS WHY THIS TAKES A SIZE. The
// millimetre-to-metre step on its own is a scale and would need nothing; putting
// the middle of the surface at the entity's position is what makes a panel
// behave like every other flat thing placed by a transform, and it is why the
// size is a field on the panel component rather than something read off the
// elements — forty rectangles do not say how big the paper is.
//
// A size with a zero in it is the caller's bug and asserts, for the reason the
// transform below gives.
[[nodiscard]] voe_math_float4x4
voe_render_element_surface_matrix(voe_math_float2 size);

// The transform an element surface of `size` millimetres wants in order to fill
// the whole render target: millimetres in, clip space out.
//
// IT IS THE SURFACE MATRIX ABOVE WITH THE TARGET MAPPED ONTO IT, AND NOT A
// SECOND ANSWER. Element space becomes the surface's own metres up there, and
// this composes onto that the one further step of covering the target with the
// whole surface. Both scales in that step are positive: everything about the Y
// direction has already happened, in one function, and a minus sign appearing
// here as well would be the classic double flip that looks correct until
// something is culled.
//
// Two decisions of its own remain, and each is a thing to get wrong silently:
//
//   - z IS JUST INSIDE THE NEAR PLANE AND NOT ON IT. Depth runs backwards in
//     this engine, so the near plane is 1.0 and this is 0.9999; the test is
//     GREATER, so an element still passes in front of anything already drawn.
//     It writes no depth, so this decides nothing about the elements among
//     themselves. The retreat from the boundary is bug 001 and ADR-0111 — a
//     surface at exactly z == w stands on the clip boundary, which is where two
//     conformant drivers may legitimately disagree and one of them discarded the
//     lot. What it costs is stated at the constant in src/element.c: with the
//     dev camera's planes, only geometry inside the hundredth-of-a-millimetre
//     shell immediately beyond the near plane can now draw over the surface.
//   - IT IS THE WHOLE TARGET AND NOT PART OF IT. A surface standing in the world
//     is the composition described above; this one is what a surface filling the
//     window wants, and such a surface has no position and never reaches the
//     draw system.
//
// A MILLIMETRE ON THIS SURFACE IS NOT A MILLIMETRE ANYWHERE ELSE, AND THIS IS
// WHERE A READER FINDS THAT OUT. `size` is stretched over the whole target, so
// an authored millimetre here is a proportion of the surface's authored height
// and nothing physical; it becomes a size a ruler would agree with only through
// the window and whatever calibration the caller applied — see
// voe_render_element_surface_size, which is where that calibration is a
// parameter. On the surface matrix above, standing in the world, a millimetre is
// a real millimetre and the window has nothing to say about it.
//
// A size with a zero in it is the caller's bug and asserts: the reciprocal of
// nothing is what would reach the shader.
[[nodiscard]] voe_math_float4x4
voe_render_element_transform(voe_math_float2 size);

// How many millimetres across a screen-filling surface is: the target's pixel
// size divided by `pixels_per_millimetre`, on both axes. The answer is what the
// surface is laid out in and what the transform above is given.
//
// ONE SCALE ON BOTH AXES IS THE WHOLE OF WHY IT EXISTS. A surface whose
// millimetre size is authored once and then stretched onto whatever shape the
// window happens to be deforms everything on it — a square becomes an oblong and
// a letter becomes a wider or narrower letter at every size, which is the sharp
// text of cards 025 and 027 undone by a matrix. Divide both axes by one number
// and the surface keeps its shape: what a window's shape changes is how many
// millimetres there are, never what a millimetre looks like. Content laid out at
// fixed millimetre positions therefore falls off the edge of a narrow window
// rather than squeezing, which is correct and is what wrapping answers later.
//
// AND THE NUMBER IS A PARAMETER BECAUSE THIS FOLDER CANNOT KNOW IT. How many
// pixels a millimetre is worth is a fact about a display and about what the
// person in front of it chose, and there is no way to ask either from here. Both
// modes anybody wants are one multiplication at the call site: a caller that
// means physical millimetres passes the display's pixels per millimetre, and a
// caller that means a proportion of the window passes the target's height
// divided by the millimetres it wants that height to hold. Nothing in here has
// a policy, a breakpoint or an opinion about what kind of device it is on.
//
// A scale of nought or less is the caller's bug and asserts: dividing by it is
// an infinity that would reach a matrix and then a shader, where it is a blank
// window rather than anything that says what happened.
[[nodiscard]] voe_math_float2
voe_render_element_surface_size(voe_platform_size target,
				float pixels_per_millimetre);

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
// A DEVICE OPENS ON FIFO, AND MAILBOX IS A REQUEST. A caller that asks for
// nothing waits for the display and draws no frame that nobody sees. A caller
// that wants the uncapped mode asks for it with voe_render_present_set, at
// startup or whenever it likes, and then asks _get what it got.
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
	// IT IS OPTIONAL AND A DRIVER MAY NOT OFFER IT, WHICH IS WHY IT IS A
	// REQUEST AND NOT A PROMISE. Asking for it on a surface that does not
	// have it is not a failure: the device falls back to FIFO and _get says
	// so. Ask, then ask what you got — never assume a device is presenting
	// the way it was asked to.
	VOE_RENDER_PRESENT_MAILBOX,
} voe_render_present;

// Ask for a mode. It takes effect on the next frame — the swapchain has to be
// built again for it, and that happens where every other rebuild does, at the
// top of a frame — so _get will still report the old one until then, and will
// report FIFO for ever if this surface has no mailbox. That includes a request
// made straight after opening: the swapchain a device opens with is already
// built, on FIFO, and the first frame is where the request is acted on.
//
// A device wants FIFO when it opens, so this is for asking for MAILBOX and for
// going back again; it is not something a caller has to call to get the engine's
// default.
void voe_render_present_set(voe_render_device *device, voe_render_present mode);

// What the swapchain that exists was actually built with, which is the answer to
// what is happening and not to what was asked for — a device asked for MAILBOX
// on a surface that has none reports FIFO here, and that is the honest
// answer rather than a failure. A headless device presents nothing and reports
// FIFO.
voe_render_present voe_render_present_get(const voe_render_device *device);
