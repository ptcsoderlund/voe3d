// The `.glb` reader: that a file's description comes back as the arrays the
// engine wants, and that every shape of broken or unreadable file is refused
// with the right one of the two answers.
//
// THE FILES ARE BUILT RATHER THAN EXPORTED, AND model_data.inc IS WHERE THEY
// ARE. An exporter's output is large, changes when the exporter does, and holds
// none of the specific things worth checking — a node with a matrix and a node
// with three parts, two textures over one picture, an index at the very edge of
// its vertex count. These are assembled to hold exactly those.
//
// THE REFUSALS ARE HALF THE POINT AND THEY ARE TWO DIFFERENT ANSWERS.
// MALFORMED is a broken file: nothing will read it. UNSUPPORTED is a file that
// is fine and that this reader will not read, and the difference matters to
// whoever is holding it — one of them can be fixed by exporting it differently.
// Most of the malformed cases below are made by breaking one byte of a file that
// works, which is what a truncated download or a bad disk actually looks like.
//
// TWO TEXTURES OVER ONE PICTURE IS THE CASE TO WATCH. glTF puts a sampler and an
// image behind a texture, and the reader resolves that so a material names an
// image: the file below has two textures with one source, and both of the
// material's channels have to come back with the same image index. That is where
// `3d`'s "upload each picture once" comes from, so a reader that handed back two
// indices would cost a texture slot per channel and nobody would notice until
// the slots ran out.
#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>

#include <testing/test.h>

#include <string.h>

#include "model_data.inc"

// Room for the decoded picture, the arrays, the JSON tokens and the filtered
// scanlines the PNG came from. A round number well above what these files need.
#define SCRATCH (4 * 1024 * 1024)

// Two roundings through a float and a quaternion.
#define TOLERANCE 1e-5f

static void check_vector(voe_math_float3 actual, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
}

// One case of a file that must not be read. The name is printed on a failure
// because a list of these is otherwise a list of line numbers.
static void refuses(const char *name, const uint8_t *bytes, size_t size,
		    voe_base_error expected)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_model model;
	voe_base_error error = VOE_BASE_OK;

	if (voe_assets_model_read_glb(bytes, size, arena, &model, &error)) {
		fprintf(stderr, "      accepted: %s\n", name);
		VOE_TEST_CHECK(false);
	} else if (error != expected) {
		fprintf(stderr, "      wrong answer for: %s\n", name);
		VOE_TEST_CHECK_INT(error, expected);
	}

	voe_base_arena_destroy(arena);
}

static void a_file_comes_back_as_its_arrays(voe_base_arena *arena)
{
	voe_assets_model model;
	voe_base_error error = VOE_BASE_OK;
	const voe_assets_primitive *quad;
	const voe_assets_material *material;

	VOE_TEST_CHECK(voe_assets_model_read_glb(QUAD_GLB, sizeof(QUAD_GLB),
						 arena, &model, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);

	VOE_TEST_CHECK_INT(model.mesh_count, 1);
	VOE_TEST_CHECK_INT(model.primitive_count, 1);
	VOE_TEST_CHECK_INT(model.material_count, 1);
	VOE_TEST_CHECK_INT(model.image_count, 1);
	VOE_TEST_CHECK_INT(model.node_count, 2);

	// The scene names one root, and the other node is its child — so a
	// reader that treated every node as a root would draw the child twice,
	// once with the parent's transform and once without.
	VOE_TEST_CHECK_INT(model.root_count, 1);
	if (model.root_count == 1)
		VOE_TEST_CHECK_INT(model.roots[0], 0);

	VOE_TEST_CHECK_INT(model.meshes[0].first_primitive, 0);
	VOE_TEST_CHECK_INT(model.meshes[0].primitive_count, 1);

	// ---- the geometry
	quad = &model.primitives[0];
	VOE_TEST_CHECK_INT(quad->vertex_count, 4);
	VOE_TEST_CHECK_INT(quad->index_count, 6);
	VOE_TEST_CHECK_INT(quad->material, 0);
	VOE_TEST_CHECK(quad->normals != NULL);
	VOE_TEST_CHECK(quad->uvs != NULL);

	check_vector(quad->positions[0], (voe_math_float3){ 0.0f, 0.0f, 0.0f });
	check_vector(quad->positions[1], (voe_math_float3){ 1.0f, 0.0f, 0.0f });
	check_vector(quad->positions[2], (voe_math_float3){ 1.0f, 1.0f, 0.0f });
	check_vector(quad->positions[3], (voe_math_float3){ 0.0f, 1.0f, 0.0f });
	if (quad->normals != NULL)
		check_vector(quad->normals[2],
			     (voe_math_float3){ 0.0f, 0.0f, 1.0f });
	if (quad->uvs != NULL) {
		VOE_TEST_CHECK_FLOAT(quad->uvs[0].x, 0.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(quad->uvs[0].y, 1.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(quad->uvs[2].x, 1.0f, 0.0f);
		VOE_TEST_CHECK_FLOAT(quad->uvs[2].y, 0.0f, 0.0f);
	}

	// The file's indices are unsigned shorts and these are unsigned ints:
	// widened on the way in, so nothing downstream has to know which of
	// glTF's three index types the file used.
	VOE_TEST_CHECK_INT(quad->indices[0], 0);
	VOE_TEST_CHECK_INT(quad->indices[1], 1);
	VOE_TEST_CHECK_INT(quad->indices[2], 2);
	VOE_TEST_CHECK_INT(quad->indices[3], 0);
	VOE_TEST_CHECK_INT(quad->indices[4], 2);
	VOE_TEST_CHECK_INT(quad->indices[5], 3);

	// ---- the material
	material = &model.materials[0];
	VOE_TEST_CHECK_FLOAT(material->base_colour.x, 0.25f, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->base_colour.y, 0.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->base_colour.z, 0.75f, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->base_colour.w, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->metallic, 0.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->roughness, 0.25f, 0.0f);
	check_vector(material->emissive,
		     (voe_math_float3){ 0.125f, 0.25f, 0.375f });

	// Two textures, one picture, one index — see the header.
	VOE_TEST_CHECK_INT(material->base_colour_image, 0);
	VOE_TEST_CHECK_INT(material->metallic_roughness_image, 0);

	// And the channels the file said nothing about are "there isn't one"
	// rather than zero, which would name the first picture.
	VOE_TEST_CHECK_INT(material->normal_image, VOE_ASSETS_MODEL_NONE);
	VOE_TEST_CHECK_INT(material->occlusion_image, VOE_ASSETS_MODEL_NONE);
	VOE_TEST_CHECK_INT(material->emissive_image, VOE_ASSETS_MODEL_NONE);

	// ---- the picture, decoded by card 017's reader out of a chunk
	VOE_TEST_CHECK_INT(model.images[0].width, 4);
	VOE_TEST_CHECK_INT(model.images[0].height, 5);
	VOE_TEST_CHECK(model.images[0].pixels != NULL);

	// ---- the nodes
	//
	// The parent gave three parts and the child gave three parts, and both
	// arrive as one matrix — so the importer sees one shape. The last column
	// is the translation, which is what says the matrix was not transposed
	// by mistake on the way through.
	VOE_TEST_CHECK_INT(model.nodes[0].mesh, VOE_ASSETS_MODEL_NONE);
	VOE_TEST_CHECK_INT(model.nodes[0].child_count, 1);
	if (model.nodes[0].child_count == 1)
		VOE_TEST_CHECK_INT(model.nodes[0].children[0], 1);
	check_vector(voe_math_float4x4_transform_point(
			     model.nodes[0].local,
			     (voe_math_float3){ 0.0f, 0.0f, 0.0f }),
		     (voe_math_float3){ 1.0f, 2.0f, 3.0f });

	VOE_TEST_CHECK_INT(model.nodes[1].mesh, 0);
	VOE_TEST_CHECK_INT(model.nodes[1].child_count, 0);

	// A quarter turn about +Y and a scale of two: +X is doubled and then
	// sent to -Z, because a positive rotation about +Y takes +Z towards +X
	// (math/tests/quat.c). A reader that had transposed the rotation would
	// send it to +Z instead, which is the mirrored answer this checks for.
	check_vector(voe_math_float4x4_transform_point(
			     model.nodes[1].local,
			     (voe_math_float3){ 1.0f, 0.0f, 0.0f }),
		     (voe_math_float3){ 0.0f, 0.0f, -2.0f });
}

// A file that is fine and that this reader will not read. Each one prints what
// it was, which is the difference between "no" and "export it differently".
static void well_formed_files_this_reader_refuses(void)
{
	refuses("a required extension", REQUIRED_EXTENSION_GLB,
		sizeof(REQUIRED_EXTENSION_GLB), VOE_BASE_ERROR_UNSUPPORTED);
	refuses("a triangle strip", TRIANGLE_STRIP_GLB,
		sizeof(TRIANGLE_STRIP_GLB), VOE_BASE_ERROR_UNSUPPORTED);
	refuses("a buffer in another file", EXTERNAL_BUFFER_GLB,
		sizeof(EXTERNAL_BUFFER_GLB), VOE_BASE_ERROR_UNSUPPORTED);

	// The text form, which is the mistake a person actually makes. It is
	// unsupported and not malformed: the file is a perfectly good glTF.
	{
		static const uint8_t text[] =
			"{\"asset\":{\"version\":\"2.0\"}}";

		refuses("a .gltf", text, sizeof(text) - 1,
			VOE_BASE_ERROR_UNSUPPORTED);
	}
}

// One byte of a working file, changed. This is what a broken file is, and every
// one of these has to be refused rather than followed into the buffer.
static void broken_containers_are_malformed(void)
{
	static uint8_t broken[sizeof(QUAD_GLB)];

	refuses("nothing at all", QUAD_GLB, 0, VOE_BASE_ERROR_MALFORMED);
	refuses("less than a header", QUAD_GLB, 8, VOE_BASE_ERROR_MALFORMED);

	// Truncated: the file says how long it is and it is not that long. Every
	// prefix is tried, because a truncation lands wherever it lands — in the
	// header, in a chunk header, in the middle of the JSON, in the middle of
	// the binary chunk.
	for (size_t length = 12; length < sizeof(QUAD_GLB); length += 7)
		refuses("a truncated file", QUAD_GLB, length,
			VOE_BASE_ERROR_MALFORMED);

	memcpy(broken, QUAD_GLB, sizeof(broken));
	broken[0] = 'x';
	refuses("a bad magic number", broken, sizeof(broken),
		VOE_BASE_ERROR_MALFORMED);

	memcpy(broken, QUAD_GLB, sizeof(broken));
	broken[4] = 1;
	refuses("container version 1", broken, sizeof(broken),
		VOE_BASE_ERROR_UNSUPPORTED);

	// The declared length, made larger than the buffer: a file that says it
	// is longer than it is.
	memcpy(broken, QUAD_GLB, sizeof(broken));
	broken[8] = 0xff;
	refuses("a file longer than itself", broken, sizeof(broken),
		VOE_BASE_ERROR_MALFORMED);

	// The first chunk's length, made larger than what is left. This is the
	// number that a reader believing it would walk off the end of.
	memcpy(broken, QUAD_GLB, sizeof(broken));
	broken[12] = 0xff;
	broken[13] = 0xff;
	refuses("a chunk longer than the file", broken, sizeof(broken),
		VOE_BASE_ERROR_MALFORMED);

	// The first chunk's type, so there is no JSON chunk at all.
	memcpy(broken, QUAD_GLB, sizeof(broken));
	broken[16] = 'x';
	refuses("no JSON chunk", broken, sizeof(broken),
		VOE_BASE_ERROR_MALFORMED);

	// A byte inside the JSON, which makes the description unparseable.
	memcpy(broken, QUAD_GLB, sizeof(broken));
	broken[20] = '#';
	refuses("broken JSON", broken, sizeof(broken),
		VOE_BASE_ERROR_MALFORMED);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	a_file_comes_back_as_its_arrays(arena);
	well_formed_files_this_reader_refuses();
	broken_containers_are_malformed();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
