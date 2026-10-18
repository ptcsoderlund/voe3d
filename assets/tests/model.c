// The `.glb` reader: that a file's description comes back as the arrays the
// engine wants, and that every shape of broken or unreadable file is refused
// with the right one of the two answers.
//
// THE FILES ARE BUILT RATHER THAN EXPORTED, AND model_data.inc IS WHERE THEY
// ARE. An exporter's output is large, changes when the exporter does, and holds
// none of the specific things worth checking — a node with a matrix and a node
// with three parts, two textures over one picture, an index at the very edge of
// its vertex count. These are assembled to hold exactly those. The COLOR_0
// files are the exception: one unindexed triangle each, assembled here at run
// time from readable JSON, because they differ only in the colour accessor, and
// so are the material files, around a picture encoded here, and the node-name
// files.
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
//
// BEYOND THE ARRAYS: COLOR_0 in each stored form (normalized bytes, three
// floats, absent, and signed bytes refused); material and image names, sides,
// unlit, emission strength and the colour texture's repeat, each read or
// defaulted; a 100-byte name cut at a whole character; Blender's texCoord −1
// read as no texture; node names; and a refusal's kept line, beginning with
// the material it is about and carrying the format's words after its own.
#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>
#include <base/report.h>

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

// A little-endian word into a file being assembled.
static void put_word(uint8_t *at, uint32_t word)
{
	for (int i = 0; i < 4; i++)
		at[i] = (uint8_t)(word >> (8 * i));
}

// A `.glb` of `json` and `bin` into `out`, each chunk padded to four bytes, its
// size returned; 0 when it does not fit.
static size_t assemble_glb(uint8_t *out, size_t capacity, const char *json,
			   const uint8_t *bin, uint32_t bin_size)
{
	uint32_t json_size = (uint32_t)strlen(json);
	uint32_t json_padded = (json_size + 3) & ~3u;
	uint32_t bin_padded = (bin_size + 3) & ~3u;
	uint32_t total = 12 + 8 + json_padded + 8 + bin_padded;

	VOE_TEST_CHECK(total <= capacity);
	if (total > capacity)
		return 0;
	memset(out, 0, total);
	memcpy(out, "glTF", 4);
	put_word(out + 4, 2);
	put_word(out + 8, total);
	put_word(out + 12, json_padded);
	memcpy(out + 16, "JSON", 4);
	memcpy(out + 20, json, json_size);
	memset(out + 20 + json_size, ' ', json_padded - json_size);
	put_word(out + 20 + json_padded, bin_padded);
	memcpy(out + 24 + json_padded, "BIN", 4);
	memcpy(out + 28 + json_padded, bin, bin_size);
	return total;
}

// A `.glb` of one unindexed triangle into `out`, its size returned. With
// `component` 0 the primitive has no COLOR_0; otherwise its COLOR_0 is
// `colours` read as that component type, `type` and `normalized`.
static size_t triangle_glb(uint8_t *out, size_t capacity, uint32_t component,
			   const char *type, bool normalized,
			   const void *colours, uint32_t colours_size)
{
	static const float positions[9] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
	char json[1024];
	uint8_t bin[256];
	uint32_t bin_size = (uint32_t)sizeof(positions) + colours_size;

	memcpy(bin, positions, sizeof(positions));
	memcpy(bin + sizeof(positions), colours, colours_size);
	snprintf(
		json, sizeof(json),
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"buffers\":[{\"byteLength\":%u}],"
		"\"bufferViews\":[{\"buffer\":0,\"byteLength\":36},"
		"{\"buffer\":0,\"byteOffset\":36,\"byteLength\":%u}],"
		"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,"
		"\"count\":3,\"type\":\"VEC3\"},{\"bufferView\":1,"
		"\"componentType\":%u,\"normalized\":%s,\"count\":3,"
		"\"type\":\"%s\"}],"
		"\"meshes\":[{\"primitives\":[{\"attributes\":"
		"{\"POSITION\":0%s}}]}],\"nodes\":[{\"mesh\":0}]}",
		bin_size, colours_size, component == 0 ? 5126 : component,
		normalized ? "true" : "false", type,
		component == 0 ? "" : ",\"COLOR_0\":1");
	return assemble_glb(out, capacity, json, bin, bin_size);
}

static void check_colour(voe_math_float4 actual, voe_math_float4 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.w, expected.w, TOLERANCE);
}

// COLOR_0 as normalized bytes and as three floats comes back as float4s, none
// comes back NULL, and signed bytes are refused as glTF never allows them.
static void a_primitive_carries_its_vertex_colours(voe_base_arena *arena)
{
	static const uint8_t bytes[12] = { 255, 0, 0, 255, 0, 51, 0, 128,
					   0,   0, 255, 0 };
	static const float floats[9] = { 0.25f, 0.5f, 0.75f, 0.25f, 0.5f,
					 0.75f, 0.25f, 0.5f, 0.75f };
	static uint8_t file[2048];
	voe_assets_model model = { 0 };
	voe_base_error error = VOE_BASE_OK;
	size_t size;

	size = triangle_glb(file, sizeof(file), 5121, "VEC4", true, bytes,
			    sizeof(bytes));
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.primitive_count == 1 && model.primitives[0].colours) {
		const voe_math_float4 *colours = model.primitives[0].colours;

		// Unindexed, so expanded: the colours come through it whole.
		VOE_TEST_CHECK_INT(model.primitives[0].index_count, 3);
		check_colour(colours[0], (voe_math_float4){ 1, 0, 0, 1 });
		check_colour(colours[1],
			     (voe_math_float4){ 0, 0.2f, 0, 128.0f / 255.0f });
		check_colour(colours[2], (voe_math_float4){ 0, 0, 1, 0 });
	} else {
		VOE_TEST_CHECK(false);
	}

	size = triangle_glb(file, sizeof(file), 5126, "VEC3", false, floats,
			    sizeof(floats));
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.primitive_count == 1 && model.primitives[0].colours)
		check_colour(model.primitives[0].colours[2],
			     (voe_math_float4){ 0.25f, 0.5f, 0.75f, 1 });
	else
		VOE_TEST_CHECK(false);

	size = triangle_glb(file, sizeof(file), 0, "VEC4", false, bytes,
			    sizeof(bytes));
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	VOE_TEST_CHECK(model.primitive_count == 1 &&
		       model.primitives[0].colours == NULL);

	size = triangle_glb(file, sizeof(file), 5120, "VEC4", true, bytes,
			    sizeof(bytes));
	refuses("a COLOR_0 of signed bytes", file, size,
		VOE_BASE_ERROR_UNSUPPORTED);
}

// A `.glb` of one picture, `png`, and the materials `materials` (a JSON
// array's inside) into `out`, its size returned. `required` is "" or an
// `"extensionsRequired":[...],` member; `image_name` is "" or a `"name":"...",`.
static size_t materials_glb(uint8_t *out, size_t capacity,
			    voe_assets_bytes png, const char *required,
			    const char *image_name, const char *materials)
{
	char json[2048];

	snprintf(json, sizeof(json),
		 "{\"asset\":{\"version\":\"2.0\"},%s"
		 "\"buffers\":[{\"byteLength\":%u}],"
		 "\"bufferViews\":[{\"buffer\":0,\"byteLength\":%u}],"
		 "\"images\":[{%s\"bufferView\":0,\"mimeType\":\"image/png\"}],"
		 "\"textures\":[{\"source\":0}],\"materials\":[%s]}",
		 required, (uint32_t)png.count, (uint32_t)png.count,
		 image_name, materials);
	return assemble_glb(out, capacity, json, png.bytes,
			    (uint32_t)png.count);
}

static void check_material(const voe_assets_material *material,
			   const char *name, bool double_sided, bool unlit,
			   float strength, float scale)
{
	VOE_TEST_CHECK(strcmp(material->name.text, name) == 0);
	VOE_TEST_CHECK(material->double_sided == double_sided);
	VOE_TEST_CHECK(material->unlit == unlit);
	VOE_TEST_CHECK_FLOAT(material->emissive_strength, strength, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->colour_scale.x, scale, 0.0f);
	VOE_TEST_CHECK_FLOAT(material->colour_scale.y, scale, 0.0f);
}

// Names, doubleSided, unlit, emissive strength and the colour texture's scale
// read back; none of them reads the defaults; a 100-byte name is cut at a whole
// character; a file requiring KHR_texture_transform loads.
static void a_material_carries_what_import_writes(voe_base_arena *arena)
{
	static uint8_t pixel[4] = { 255, 255, 255, 255 };
	static uint8_t file[4096];
	voe_assets_bytes png = { 0 };
	voe_assets_model model = { 0 };
	voe_base_error error = VOE_BASE_OK;
	char euros_first[128] = "";
	char a_first[128] = "a";
	char image_name[160];
	char material[160];
	size_t size;

	VOE_TEST_CHECK(voe_assets_png_encode(
		arena, (voe_assets_image){ 1, 1, pixel }, &png, &error));

	size = materials_glb(
		file, sizeof(file), png, "", "\"name\":\"Bark\",",
		"{\"name\":\"Leaf\",\"doubleSided\":true,"
		"\"extensions\":{\"KHR_materials_unlit\":{}}},"
		"{\"name\":\"Lamp\",\"extensions\":"
		"{\"KHR_materials_emissive_strength\":{\"emissiveStrength\":4}},"
		"\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0,"
		"\"extensions\":{\"KHR_texture_transform\":{\"scale\":[3,3]}}}}}");
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.material_count == 2 && model.image_count == 1) {
		check_material(&model.materials[0], "Leaf", true, true, 1, 1);
		check_material(&model.materials[1], "Lamp", false, false, 4, 3);
		VOE_TEST_CHECK_INT(model.materials[1].base_colour_image, 0);
		VOE_TEST_CHECK(strcmp(model.image_names[0].text, "Bark") == 0);
	} else {
		VOE_TEST_CHECK(false);
	}

	// Required, and still read: the defaults, because nothing uses it.
	size = materials_glb(file, sizeof(file), png,
			     "\"extensionsRequired\":[\"KHR_texture_transform\"],",
			     "", "{}");
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.material_count == 1 && model.image_count == 1) {
		check_material(&model.materials[0], "", false, false, 1, 1);
		VOE_TEST_CHECK(strcmp(model.image_names[0].text, "") == 0);
	} else {
		VOE_TEST_CHECK(false);
	}

	// 33 euro signs of three bytes and one more byte, 100 in all. Behind
	// "a" the 63-byte cut lands inside the 21st sign and backs off to 61;
	// in front of it, it falls between two signs and keeps 63.
	for (int i = 0; i < 33; i++) {
		strcat(euros_first, "\xe2\x82\xac");
		strcat(a_first, "\xe2\x82\xac");
	}
	strcat(euros_first, "a");
	VOE_TEST_CHECK_INT(strlen(euros_first), 100);
	snprintf(image_name, sizeof(image_name), "\"name\":\"%s\",", a_first);
	snprintf(material, sizeof(material), "{\"name\":\"%s\"}", euros_first);
	size = materials_glb(file, sizeof(file), png, "", image_name, material);
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.material_count == 1 && model.image_count == 1) {
		VOE_TEST_CHECK_INT(strlen(model.materials[0].name.text), 63);
		VOE_TEST_CHECK(memcmp(model.materials[0].name.text, euros_first,
				      63) == 0);
		VOE_TEST_CHECK_INT(strlen(model.image_names[0].text), 61);
	} else {
		VOE_TEST_CHECK(false);
	}
}

// Blender's `"texCoord": -1` on the base colour texture: the material reads, its
// base colour texture NONE with no line said, the rest as written (0446).
static void a_texcoord_of_minus_one_is_no_texture(voe_base_arena *arena)
{
	static uint8_t pixel[4] = { 255, 255, 255, 255 };
	static uint8_t file[4096];
	voe_assets_bytes png = { 0 };
	voe_assets_model model = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const voe_assets_material *needles;
	size_t size;

	VOE_TEST_CHECK(voe_assets_png_encode(
		arena, (voe_assets_image){ 1, 1, pixel }, &png, &error));
	size = materials_glb(
		file, sizeof(file), png, "", "",
		"{\"name\":\"Needles\",\"doubleSided\":true,"
		"\"pbrMetallicRoughness\":{\"baseColorFactor\":[0.25,0.5,0.125,1],"
		"\"roughnessFactor\":0.75,"
		"\"baseColorTexture\":{\"index\":0,\"texCoord\":-1},"
		"\"metallicRoughnessTexture\":{\"index\":0}}}");
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.material_count != 1) {
		VOE_TEST_CHECK(false);
		return;
	}
	needles = &model.materials[0];
	check_material(needles, "Needles", true, false, 1, 1);
	VOE_TEST_CHECK_INT(needles->base_colour_image, VOE_ASSETS_MODEL_NONE);
	VOE_TEST_CHECK_INT(needles->metallic_roughness_image, 0);
	check_colour(needles->base_colour,
		     (voe_math_float4){ 0.25f, 0.5f, 0.125f, 1 });
	VOE_TEST_CHECK_FLOAT(needles->roughness, 0.75f, 0.0f);
}

// A file refused with a kept line that begins with `subject` and contains each
// of `words` and `also`.
static void refused_saying(const char *what, const uint8_t *bytes, size_t size,
			   const char *subject, const char *words,
			   const char *also)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_assets_model model;
	const char *kept;

	voe_base_report_error_clear();
	VOE_TEST_CHECK(!voe_assets_model_read_glb(bytes, size, arena, &model,
						  NULL));
	kept = voe_base_report_error_first();
	if (kept == NULL || strncmp(kept, subject, strlen(subject)) != 0 ||
	    strstr(kept, words) == NULL || strstr(kept, also) == NULL) {
		fprintf(stderr, "      %s said: %s\n", what,
			kept == NULL ? "nothing" : kept);
		VOE_TEST_CHECK(false);
	}
	voe_base_arena_destroy(arena);
}

// Draco named with the format's words after it; a material `leaf` on a second
// UV map and one naming a texture past the textures, each under its material.
static void a_refusal_names_what_it_is_about(voe_base_arena *arena)
{
	static uint8_t pixel[4] = { 255, 255, 255, 255 };
	static uint8_t file[4096];
	voe_assets_bytes png = { 0 };
	voe_base_error error = VOE_BASE_OK;
	size_t size;

	refused_saying("Draco", REQUIRED_EXTENSION_GLB,
		       sizeof(REQUIRED_EXTENSION_GLB), "the file needs",
		       "KHR_draco_mesh_compression", "(glTF: ");

	VOE_TEST_CHECK(voe_assets_png_encode(
		arena, (voe_assets_image){ 1, 1, pixel }, &png, &error));
	size = materials_glb(file, sizeof(file), png, "", "",
			     "{\"name\":\"leaf\",\"pbrMetallicRoughness\":"
			     "{\"baseColorTexture\":{\"index\":0,"
			     "\"texCoord\":1}}}");
	refused_saying("texCoord 1", file, size, "the material `leaf`: ",
		       "UV map", "(glTF: ");

	size = materials_glb(file, sizeof(file), png, "", "",
			     "{\"name\":\"leaf\",\"normalTexture\":"
			     "{\"index\":5}}");
	refused_saying("a texture past the textures", file, size,
		       "the material `leaf`: ", "export it again", "(glTF: ");
}

// A `.glb` of no meshes and the nodes `nodes` (a JSON array's inside) into
// `out`, its size returned.
static size_t nodes_glb(uint8_t *out, size_t capacity, const char *nodes)
{
	static const uint8_t bin[4] = { 0 };
	char json[512];

	snprintf(json, sizeof(json),
		 "{\"asset\":{\"version\":\"2.0\"},"
		 "\"buffers\":[{\"byteLength\":4}],\"nodes\":[%s]}",
		 nodes);
	return assemble_glb(out, capacity, json, bin, sizeof(bin));
}

// A child named "Trunk_LOD1" under a root with no name reads both back, as
// Blender's levels are read from them (0425); a name that is a number is refused.
static void a_node_keeps_its_name(voe_base_arena *arena)
{
	static uint8_t file[1024];
	voe_assets_model model = { 0 };
	voe_base_error error = VOE_BASE_OK;
	size_t size;

	size = nodes_glb(file, sizeof(file),
			 "{\"children\":[1]},{\"name\":\"Trunk_LOD1\"}");
	VOE_TEST_CHECK(voe_assets_model_read_glb(file, size, arena, &model,
						 &error));
	if (model.node_count == 2) {
		VOE_TEST_CHECK(strcmp(model.nodes[0].name.text, "") == 0);
		VOE_TEST_CHECK(strcmp(model.nodes[1].name.text, "Trunk_LOD1") ==
			       0);
	} else {
		VOE_TEST_CHECK(false);
	}

	size = nodes_glb(file, sizeof(file), "{\"name\":7}");
	refuses("a node whose name is a number", file, size,
		VOE_BASE_ERROR_MALFORMED);
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
	a_primitive_carries_its_vertex_colours(arena);
	a_material_carries_what_import_writes(arena);
	a_texcoord_of_minus_one_is_no_texture(arena);
	a_node_keeps_its_name(arena);
	a_refusal_names_what_it_is_about(arena);
	well_formed_files_this_reader_refuses();
	broken_containers_are_malformed();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
