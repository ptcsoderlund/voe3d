// Models on the way in: a `.glb` file's bytes, engine data out. Written here
// (ADR-0023), nothing fetched, and import only — glTF is never a scene format
// (ADR-0010), so what comes out of this is the file's content and not a world.
//
// BINARY ONLY, AND THAT IS A REAL SIMPLIFICATION RATHER THAN A GAP. A `.glb` is
// one file: the JSON, the vertex data and the pictures are all chunks inside it.
// The text form would bring external file resolution, base64 data URIs and
// relative-path guessing with it, and `platform` has no file API to resolve
// anything with — so a `.gltf` is refused, by name, and says so.
//
// NOTHING HERE KNOWS ABOUT THE GPU, AN ENTITY OR A SCENE. This produces arrays
// in an arena: vertex attributes, indices, material factors, decoded pixels and
// a node tree. `3d` is what uploads them, gets ids back and writes components —
// the pipeline is file → assets → 3d → ids from render → components in tables.
//
// NO COORDINATE IS CONVERTED, EVER (ADR-0033). This engine is right-handed, +Y
// up, −Z forward, which is glTF's convention exactly — that was chosen so that
// an importer would have nothing to do here. A conversion may not be added
// later either: if a model comes in mirrored, something else is wrong.
//
// MATRIX LAYOUT IS A DIFFERENT MATTER AND IT IS TRANSPOSED (ADR-0035). glTF
// stores a matrix column-major and this engine stores one row-major, so every
// matrix below has been transposed on the way in. Layout only — never
// coordinates. Confusing the two is the bug that makes a model look mirrored and
// correct depending on what you compare it against.
//
// WHAT IS REFUSED, AND EACH OF THESE SAYS WHICH IT WAS RATHER THAN BEING GUESSED
// AT. VOE_BASE_ERROR_MALFORMED is a broken file: a bad magic number, a chunk
// header that does not fit inside the file, an accessor reaching past its buffer
// view, a node tree that is not a tree. VOE_BASE_ERROR_UNSUPPORTED is a
// well-formed file this reader will not read, and every one of them prints what
// it was: a required extension, a primitive that is not a triangle list, a
// sparse accessor, a buffer or an image that names an external file, a texture
// coordinate set other than the first, a component type an attribute may not
// have, or more JSON than the reader will hold.
//
// ANIMATION AND SKINNING ARE IGNORED RATHER THAN REFUSED. A file with them in it
// loads, and what it draws is the bind pose. Refusing would turn most models
// people have into an error message; pretending to animate them would be worse.
// The card that reads them is a later card, and the principal's decision.
//
// SO ARE A MATERIAL'S `doubleSided` AND ITS ALPHA MODE, AND THOSE TWO ARE WORTH
// KNOWING ABOUT. Blender writes `doubleSided: true` unless someone ticks its
// backface-culling box, and this engine culls back faces whatever a material
// says — which is invisible on closed geometry and takes half the faces off a
// flat one, a leaf or a sheet of cloth. Alpha is the same shape of gap: nothing
// in this engine blends yet. Both are properties this reader could carry and
// nothing downstream could yet act on, so they are not carried: a field that is
// read and ignored reads as a feature. The cards that cull differently or blend
// are the cards that add them.
#pragma once

#include <assets/image.h>
#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>

#include <stddef.h>
#include <stdint.h>

// What every "which one" field below holds when the answer is "none". A material
// with no texture, a primitive with no material, a node that draws nothing.
#define VOE_ASSETS_MODEL_NONE UINT32_MAX

// One drawable piece: a glTF primitive. A glTF mesh may hold several of them,
// each with its own material, and each one is a draw.
//
// THE ATTRIBUTES ARE SEPARATE ARRAYS AND NOT ONE INTERLEAVED VERTEX, WHICH IS
// THE FOLDER BOUNDARY MADE CONCRETE. What a vertex looks like on the GPU is
// `render`'s business — it is the layout the pipeline describes — and this folder
// may not name it. So `3d` interleaves these into whatever a vertex is when it
// uploads, and the day that layout changes, nothing here does.
//
// normals AND uvs MAY BE NULL, because a file need not have them. positions and
// indices are always there: a primitive without positions is malformed, and one
// without indices is expanded on the way in so that everything downstream has
// exactly one shape to handle.
typedef struct {
	const voe_math_float3 *positions;
	const voe_math_float3 *normals;
	const voe_math_float2 *uvs;
	uint32_t vertex_count;

	const uint32_t *indices;
	uint32_t index_count;

	// Into voe_assets_model.materials, or VOE_ASSETS_MODEL_NONE.
	uint32_t material;
} voe_assets_primitive;

// A glTF mesh: a run of primitives. A node points at one of these, so a node
// that draws several materials is several draws sharing one transform.
typedef struct {
	uint32_t first_primitive;
	uint32_t primitive_count;
} voe_assets_mesh;

// The metallic-roughness material glTF describes, with the factors it gives and
// which picture each channel reads.
//
// EVERY TEXTURE FIELD IS AN INDEX INTO model.images AND NOT INTO ITS textures
// ARRAY. glTF puts a sampler and an image behind a texture; this reader resolves
// that on the way in, so two textures sharing one image are one entry here —
// which is exactly the deduplication `3d` needs in order to upload each picture
// once.
//
// OCCLUSION, ROUGHNESS AND METALNESS ARRIVE PACKED THE WAY THE ENGINE WANTS
// THEM, AND THAT IS glTF'S ARRANGEMENT RATHER THAN A CONVERSION. glTF puts
// occlusion in red, roughness in green and metalness in blue of one picture, and
// its occlusion texture is very often the same picture as its
// metallic-roughness one. Nothing here repacks anything: the two fields may name
// the same image, and that is the packed texture.
//
// EMISSION IS WHERE glTF AND THE PACKING DIVERGE, AND THIS READER DOES NOT TAKE
// THAT DECISION. glTF's emission is a full RGB texture and not a strength in a
// fourth channel, so it is a field of its own here. Packing it into an alpha
// channel is a decision nobody has taken and it is not taken by parsing.
typedef struct {
	voe_math_float4 base_colour;
	float metallic;
	float roughness;
	voe_math_float3 emissive;

	uint32_t base_colour_image;
	uint32_t metallic_roughness_image;
	uint32_t normal_image;
	uint32_t occlusion_image;
	uint32_t emissive_image;
} voe_assets_material;

// One node of the file's tree.
//
// THE MATRIX IS THE NODE'S OWN AND NOT ITS WORLD TRANSFORM. Composing the tree
// is the importer's job, because the importer is what knows about entities and
// about the depth limit a file's nesting has to be refused past (rule 14). A
// node that gave translation, rotation and scale instead of a matrix has already
// had them composed into one here, so there is one shape downstream and not two.
typedef struct {
	voe_math_float4x4 local;
	// Into model.meshes, or VOE_ASSETS_MODEL_NONE for a node that only
	// carries a transform for its children.
	uint32_t mesh;
	// Into model.nodes. Every node is a child of at most one other, which
	// this reader checks — so this really is a tree and a walk of it cannot
	// loop.
	const uint32_t *children;
	uint32_t child_count;
} voe_assets_node;

// Everything the file held, in the arena the reader was handed. Freed by
// rewinding or destroying that arena and never one allocation at a time
// (rule 11).
typedef struct {
	const voe_assets_primitive *primitives;
	uint32_t primitive_count;

	const voe_assets_mesh *meshes;
	uint32_t mesh_count;

	const voe_assets_material *materials;
	uint32_t material_count;

	// Decoded pixels, one per picture the file held, already RGBA8 — see
	// assets/include/assets/image.h, including why nothing is turned the
	// right way up on the way in.
	const voe_assets_image *images;
	uint32_t image_count;

	const voe_assets_node *nodes;
	uint32_t node_count;

	// The nodes the file says to start from: the default scene's, or every
	// node nothing else claims as a child when the file names no scene.
	const uint32_t *roots;
	uint32_t root_count;
} voe_assets_model;

// Read a `.glb`. False on failure, `model` untouched when it fails, and a line
// on stderr saying exactly what was wrong. See the header for which failures are
// malformed and which are unsupported.
//
// THE BYTES ARE THE CALLER'S AND ARE NOT KEPT. Everything in `model` is copied
// or decoded into `arena`, so the file's buffer may be freed the moment this
// returns — which is what lets a `#embed`ded model and a model that arrived some
// other way be handled the same way.
[[nodiscard]] bool voe_assets_model_read_glb(const uint8_t *bytes, size_t size,
					     voe_base_arena *arena,
					     voe_assets_model *model,
					     voe_base_error *error);
