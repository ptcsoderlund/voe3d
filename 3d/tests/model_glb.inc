// A `.glb` of a unit UV sphere built on an arena, for tests that need a model
// file of some size without keeping one in the tree. Included by a test after
// <base/arena.h>, <math.h>, <stdint.h>, <stdio.h> and <string.h>.
//
//     size_t size;
//     const uint8_t *glb = sphere_glb(arena, 32, 64, false, &size);
//
// THE SPHERE is a top pole, `rings` − 1 rings of `segments` vertices and a
// bottom pole, each vertex's normal its position: one vertex per position, so
// no seam. Its triangles are wound counter-clockwise seen from outside,
// segments × (rings − 1) × 2 of them, as 32-bit indices.
//
// THE FILE is what `assets` reads (assets/include/assets/model.h): a JSON chunk
// padded with spaces, then a BIN chunk of positions, normals and indices; one
// mesh of one primitive wearing one empty material, one root node "Ball". With
// `own_level`, a second root "Ball_LOD1" wears a second mesh, the same vertices
// with the first half of the indices (half the segments, a half shell), which
// makes a file with its own levels, fewer triangles and an error against level
// 0. Little-endian, as glTF is; written on a little-endian
// machine, as every machine the engine builds for is.

#define SPHERE_GLB_PI 3.14159265358979323846f

static uint32_t sphere_glb_vertex(uint32_t segments, uint32_t r, uint32_t s)
{
	return 1 + (r - 1) * segments + s % segments;
}

static void sphere_glb_u32(uint8_t *at, uint32_t value)
{
	for (int b = 0; b < 4; b++)
		at[b] = (uint8_t)(value >> (8 * b));
}

// The index triangles, as simplify.c's test winds them.
static void sphere_glb_indices(uint32_t rings, uint32_t segments,
			       uint32_t bottom, uint32_t *indices)
{
	uint32_t at = 0;

	for (uint32_t s = 0; s < segments; s++) {
		const uint32_t top[3] = { 0, sphere_glb_vertex(segments, 1, s + 1),
					  sphere_glb_vertex(segments, 1, s) };
		const uint32_t low[3] = {
			sphere_glb_vertex(segments, rings - 1, s),
			sphere_glb_vertex(segments, rings - 1, s + 1), bottom
		};

		memcpy(&indices[at], top, sizeof(top));
		memcpy(&indices[at + 3], low, sizeof(low));
		at += 6;
		for (uint32_t r = 1; r + 1 < rings; r++) {
			const uint32_t band[6] = {
				sphere_glb_vertex(segments, r, s),
				sphere_glb_vertex(segments, r, s + 1),
				sphere_glb_vertex(segments, r + 1, s),
				sphere_glb_vertex(segments, r, s + 1),
				sphere_glb_vertex(segments, r + 1, s + 1),
				sphere_glb_vertex(segments, r + 1, s)
			};

			memcpy(&indices[at], band, sizeof(band));
			at += 6;
		}
	}
}

static const uint8_t *sphere_glb(voe_base_arena *arena, uint32_t rings,
				 uint32_t segments, bool own_level, size_t *size)
{
	const uint32_t vertex_count = 2 + (rings - 1) * segments;
	const uint32_t index_count = segments * (rings - 1) * 6;
	const size_t floats = (size_t)vertex_count * 3 * sizeof(float);
	const size_t bin = 2 * floats + (size_t)index_count * sizeof(uint32_t);
	float *positions = voe_base_arena_push(arena, floats);
	char json[1536];
	char half[96] = "";
	size_t json_size;
	uint8_t *file;

	if (own_level)
		(void)snprintf(half, sizeof(half),
			       ",{\"bufferView\":2,\"componentType\":5125,"
			       "\"count\":%u,\"type\":\"SCALAR\"}",
			       index_count / 2);
	positions[1] = 1.0f;
	positions[(vertex_count - 1) * 3 + 1] = -1.0f;
	for (uint32_t r = 1; r < rings; r++) {
		for (uint32_t s = 0; s < segments; s++) {
			const float theta = SPHERE_GLB_PI * (float)r / (float)rings;
			const float phi =
				2.0f * SPHERE_GLB_PI * (float)s / (float)segments;
			float *p = &positions[sphere_glb_vertex(segments, r, s) * 3];

			p[0] = sinf(theta) * cosf(phi);
			p[1] = cosf(theta);
			p[2] = sinf(theta) * sinf(phi);
		}
	}

	json_size = (size_t)snprintf(
		json, sizeof(json),
		"{\"asset\":{\"version\":\"2.0\"},\"scene\":0,"
		"\"scenes\":[{\"nodes\":%s}],"
		"\"nodes\":[{\"name\":\"Ball\",\"mesh\":0}%s],"
		"\"meshes\":[{\"primitives\":[{\"attributes\":"
		"{\"POSITION\":0,\"NORMAL\":1},\"indices\":2,\"material\":0}]}%s],"
		"\"materials\":[{}],"
		"\"accessors\":["
		"{\"bufferView\":0,\"componentType\":5126,\"count\":%u,\"type\":\"VEC3\"},"
		"{\"bufferView\":1,\"componentType\":5126,\"count\":%u,\"type\":\"VEC3\"},"
		"{\"bufferView\":2,\"componentType\":5125,\"count\":%u,\"type\":\"SCALAR\"}%s],"
		"\"bufferViews\":["
		"{\"buffer\":0,\"byteOffset\":0,\"byteLength\":%zu},"
		"{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu},"
		"{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu}],"
		"\"buffers\":[{\"byteLength\":%zu}]}",
		own_level ? "[0,1]" : "[0]",
		own_level ? ",{\"name\":\"Ball_LOD1\",\"mesh\":1}" : "",
		own_level ? ",{\"primitives\":[{\"attributes\":"
			    "{\"POSITION\":0,\"NORMAL\":1},\"indices\":3,"
			    "\"material\":0}]}" :
			    "",
		vertex_count, vertex_count, index_count, half, floats, floats,
		floats, 2 * floats,
		(size_t)index_count * sizeof(uint32_t), bin);
	while (json_size % 4 != 0)
		json[json_size++] = ' ';

	*size = 12 + 8 + json_size + 8 + bin;
	file = voe_base_arena_push(arena, *size);
	sphere_glb_u32(file, 0x46546C67u);
	sphere_glb_u32(file + 4, 2);
	sphere_glb_u32(file + 8, (uint32_t)*size);
	sphere_glb_u32(file + 12, (uint32_t)json_size);
	sphere_glb_u32(file + 16, 0x4E4F534Au);
	memcpy(file + 20, json, json_size);
	sphere_glb_u32(file + 20 + json_size, (uint32_t)bin);
	sphere_glb_u32(file + 24 + json_size, 0x004E4942u);
	// Positions, then the same floats as normals: a unit sphere's.
	memcpy(file + 28 + json_size, positions, floats);
	memcpy(file + 28 + json_size + floats, positions, floats);
	sphere_glb_indices(rings, segments, vertex_count - 1,
			   (uint32_t *)(file + 28 + json_size + 2 * floats));
	return file;
}
