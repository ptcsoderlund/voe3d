// A tree's model from its skeleton (3d/tree_grow.h): each level's bark as one
// primitive of tubes and its leaves as one primitive of cards, four levels,
// two materials, a root node a level.
//
// A tube is rings of `sides` + 1 vertices (the last repeats the first for the
// seam's u) about the limb's centre line, framed by parallel transport so it
// does not twist. Sides at level 0 are 12 trunk, 8 branch, 4 twig. Its rings
// stand at every (steps / lengths)-th skeleton point, plus the collar a child
// has. The cards are placed once and every level draws its share of them.
//
// Constraints: each primitive's arrays are counted first and pushed once;
// nothing else is pushed but the cards, VOE_3D_TREE_LEAVES of them.
#include <3d/tree_grow.h>

#include <assets/model.h>
#include <base/assert.h>
#include <math/float2.h>
#include <math/float3.h>
#include <math/float4x4.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define ORDERS 3
#define TWIG 2
#define TAU 6.28318531f
#define MOST_STEPS 8
#define MOST_RINGS (MOST_STEPS + 2)
// Where on a twig, as a share of its length, its cards start.
#define LEAF_START 0.2f

static const uint32_t limb_sides[ORDERS] = { 12, 8, 4 };

// One leaf card at level 0: its centre and the two unit axes it spans.
typedef struct {
	voe_math_float3 centre;
	voe_math_float3 across;
	voe_math_float3 along;
} card;

// One level's tube counts for a limb; all 0 when it is no tube there.
typedef struct {
	uint32_t sides;
	uint32_t lengths;
	uint32_t rings;
} tube;

typedef struct {
	voe_math_float3 *positions;
	voe_math_float3 *normals;
	voe_math_float2 *uvs;
	uint32_t *indices;
	uint32_t vertex_count;
	uint32_t index_count;
} build;

// The next number in [0, 1) from a xorshift64 on `*seed`.
static float draw(uint64_t *seed)
{
	uint64_t x = *seed;

	VOE_BASE_ASSERT(x != 0, "a nonzero seed");
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*seed = x;
	return (float)(x >> 40) * 0x1p-24f;
}

// A nonzero seed from the variation alone, its bits mixed by splitmix64 on
// another stream than the skeleton's.
static uint64_t seed_of(uint32_t variation)
{
	uint64_t z = ((uint64_t)variation << 32) + 0x2545F4914F6CDD1Dull;

	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	z ^= z >> 31;
	return z != 0 ? z : 1;
}

// Two unit vectors perpendicular to unit `d` and to each other, `w` = d × u.
static void around(voe_math_float3 d, voe_math_float3 *u, voe_math_float3 *w)
{
	const voe_math_float3 ref = fabsf(d.y) < 0.9f ?
					    (voe_math_float3){ 0.0f, 1.0f, 0.0f } :
					    (voe_math_float3){ 1.0f, 0.0f, 0.0f };

	*u = voe_math_float3_normalize(voe_math_float3_cross(d, ref));
	*w = voe_math_float3_cross(d, *u);
}

// The steps of the limb whose first segment is `i`; 0 when `i` is not one's
// first.
static uint32_t limb_at(const voe_3d_tree_skeleton *s, uint32_t i)
{
	const voe_3d_tree_segment *seg = s->segments;
	uint32_t n = 1;

	if (seg[i].parent != VOE_3D_TREE_NO_PARENT &&
	    seg[seg[i].parent].order == seg[i].order)
		return 0;
	while (i + n < s->count && seg[i + n].parent == i + n - 1 &&
	       seg[i + n].order == seg[i].order)
		n++;
	VOE_BASE_ASSERT(n <= MOST_STEPS && seg[i].order < ORDERS,
			"a limb the grower makes");
	return n;
}

static tube tube_of(uint32_t order, uint32_t steps, uint32_t level)
{
	uint32_t sides = limb_sides[order] >> level;
	uint32_t lengths = steps >> level;

	if (level > 0 && order == TWIG)
		return (tube){ 0 };
	sides = sides < 3 ? 3 : sides;
	lengths = lengths < 1 ? 1 : lengths;
	return (tube){ sides, lengths, lengths + 1 + (order > 0 ? 1u : 0u) };
}

// The radius of `s`'s surface across from `p`.
static float radius_at(voe_3d_tree_segment s, voe_math_float3 p)
{
	const voe_math_float3 d = voe_math_float3_sub(s.end, s.start);
	const float t = voe_math_float3_dot(voe_math_float3_sub(p, s.start), d) /
			voe_math_float3_dot(d, d);
	const float c = fminf(fmaxf(t, 0.0f), 1.0f);

	return s.radius_start + (s.radius_end - s.radius_start) * c;
}

static build build_of(voe_base_arena *arena, uint32_t vertices,
		      uint32_t indices)
{
	const size_t v = vertices > 0 ? vertices : 1;
	const size_t i = indices > 0 ? indices : 1;

	return (build){
		.positions = voe_base_arena_push(arena, sizeof(voe_math_float3) * v),
		.normals = voe_base_arena_push(arena, sizeof(voe_math_float3) * v),
		.uvs = voe_base_arena_push(arena, sizeof(voe_math_float2) * v),
		.indices = voe_base_arena_push(arena, sizeof(uint32_t) * i),
	};
}

static void put_vertex(build *b, voe_math_float3 p, voe_math_float3 n,
		       voe_math_float2 uv)
{
	b->positions[b->vertex_count] = p;
	b->normals[b->vertex_count] = n;
	b->uvs[b->vertex_count] = uv;
	b->vertex_count++;
}

// The frame's normal carried from `normal` onto the plane across `tangent`.
static voe_math_float3 carried(voe_math_float3 normal, voe_math_float3 tangent)
{
	const voe_math_float3 n = voe_math_float3_sub(
		normal, voe_math_float3_scale(tangent, voe_math_float3_dot(
								normal, tangent)));
	voe_math_float3 u, w;

	if (voe_math_float3_length(n) > 1e-4f)
		return voe_math_float3_normalize(n);
	around(tangent, &u, &w);
	return u;
}

// Rings about `n` centres, each wound counter-clockwise seen from outside.
static void put_rings(build *b, const voe_math_float3 *centre,
		      const float *radius, uint32_t n, uint32_t sides,
		      float u_around)
{
	const uint32_t first = b->vertex_count;
	voe_math_float3 normal = { 0 };
	float v = 0.0f;

	for (uint32_t j = 0; j < n; j++) {
		const voe_math_float3 tangent = voe_math_float3_normalize(
			voe_math_float3_sub(centre[j + 1 < n ? j + 1 : j],
					    centre[j > 0 ? j - 1 : 0]));
		voe_math_float3 binormal, w;

		if (j == 0)
			around(tangent, &normal, &w);
		normal = carried(normal, tangent);
		binormal = voe_math_float3_cross(tangent, normal);
		if (j > 0)
			v += voe_math_float3_length(
				voe_math_float3_sub(centre[j], centre[j - 1]));
		for (uint32_t s = 0; s <= sides; s++) {
			const float a = TAU * (float)s / (float)sides;
			const voe_math_float3 o = voe_math_float3_add(
				voe_math_float3_scale(normal, cosf(a)),
				voe_math_float3_scale(binormal, sinf(a)));

			put_vertex(b,
				   voe_math_float3_add(
					   centre[j],
					   voe_math_float3_scale(o, radius[j])),
				   o,
				   (voe_math_float2){
					   u_around * (float)s / (float)sides, v });
		}
	}
	for (uint32_t j = 0; j + 1 < n; j++) {
		for (uint32_t s = 0; s < sides; s++) {
			const uint32_t a = first + j * (sides + 1) + s;
			const uint32_t c = a + sides + 1;
			const uint32_t quad[6] = { a, a + 1, c, a + 1, c + 1, c };

			for (uint32_t q = 0; q < 6; q++)
				b->indices[b->index_count++] = quad[q];
		}
	}
}

// The limb from `first`, `steps` long, as `shape`'s tube; a child's collar
// stands where its first segment leaves its parent's surface.
static void put_tube(build *b, const voe_3d_tree_skeleton *s, uint32_t first,
		     uint32_t steps, tube shape)
{
	const voe_3d_tree_segment head = s->segments[first];
	voe_math_float3 centre[MOST_RINGS];
	float radius[MOST_RINGS];
	uint32_t n = 0;

	centre[n] = head.start;
	radius[n++] = head.radius_start;
	if (head.parent != VOE_3D_TREE_NO_PARENT) {
		const float r = radius_at(s->segments[head.parent], head.start);
		const voe_math_float3 d = voe_math_float3_sub(head.end, head.start);
		const float length = voe_math_float3_length(d);

		centre[n] = voe_math_float3_add(
			head.start, voe_math_float3_scale(
					    d, fminf(r, 0.5f * length) / length));
		radius[n++] = 0.5f * (head.radius_start + r);
	}
	for (uint32_t j = 1; j <= shape.lengths; j++) {
		const voe_3d_tree_segment at =
			s->segments[first + j * steps / shape.lengths - 1];

		centre[n] = at.end;
		radius[n++] = at.radius_end;
	}
	VOE_BASE_ASSERT(n == shape.rings, "the rings counted");
	put_rings(b, centre, radius, n, shape.sides,
		  fmaxf(1.0f, roundf(TAU * head.radius_start)));
}

static voe_assets_primitive primitive_of(build b, uint32_t material)
{
	return (voe_assets_primitive){
		.positions = b.positions,
		.normals = b.normals,
		.uvs = b.uvs,
		.vertex_count = b.vertex_count,
		.indices = b.indices,
		.index_count = b.index_count,
		.material = material,
	};
}

static voe_assets_primitive bark(voe_base_arena *arena,
				 const voe_3d_tree_skeleton *s, uint32_t level)
{
	uint32_t vertices = 0, indices = 0;
	build b;

	for (uint32_t i = 0; i < s->count; i++) {
		const uint32_t steps = limb_at(s, i);
		const tube t = steps > 0 ? tube_of(s->segments[i].order, steps,
						   level) :
					   (tube){ 0 };

		vertices += t.rings * (t.sides + 1);
		indices += t.rings > 0 ? (t.rings - 1) * t.sides * 6 : 0;
	}
	b = build_of(arena, vertices, indices);
	for (uint32_t i = 0; i < s->count; i++) {
		const uint32_t steps = limb_at(s, i);
		const tube t = steps > 0 ? tube_of(s->segments[i].order, steps,
						   level) :
					   (tube){ 0 };

		if (t.rings > 0)
			put_tube(&b, s, i, steps, t);
	}
	VOE_BASE_ASSERT(b.vertex_count == vertices && b.index_count == indices,
			"the bark counted");
	return primitive_of(b, 0);
}

// The cards of every twig into `out`, up to the cap; how many.
static uint32_t place_cards(const voe_3d_tree_skeleton *s,
			    const voe_assets_tree *values, uint32_t variation,
			    card *out)
{
	uint64_t seed = seed_of(variation);
	uint32_t n = 0;

	for (uint32_t i = 0; i < s->count && n < VOE_3D_TREE_LEAVES; i++) {
		const uint32_t steps = limb_at(s, i);
		uint32_t leaves;

		if (steps == 0 || s->segments[i].order != TWIG)
			continue;
		leaves = (uint32_t)(values->leaf_density + draw(&seed));
		for (uint32_t l = 0; l < leaves && n < VOE_3D_TREE_LEAVES; l++) {
			const float t = (LEAF_START + (1.0f - LEAF_START) *
							      draw(&seed)) *
					(float)steps;
			const uint32_t j =
				(uint32_t)fminf(t, (float)(steps - 1));
			const voe_3d_tree_segment on = s->segments[i + j];
			const voe_math_float3 along = voe_math_float3_normalize(
				voe_math_float3_sub(on.end, on.start));
			const float roll = TAU * draw(&seed);
			voe_math_float3 u, w, across;

			around(along, &u, &w);
			across = voe_math_float3_add(
				voe_math_float3_scale(u, cosf(roll)),
				voe_math_float3_scale(w, sinf(roll)));
			out[n++] = (card){
				.centre = voe_math_float3_add(
					voe_math_float3_lerp(on.start, on.end,
							     t - (float)j),
					voe_math_float3_scale(
						across,
						0.5f * values->leaf_size)),
				.across = across,
				.along = along,
			};
		}
	}
	return n;
}

// `c` as a quad `half` × 2 across, facing away from `crown`, each corner's
// normal bent away from it.
static void put_card(build *b, card c, float half, voe_math_float3 crown)
{
	static const float signs[2][4][2] = {
		{ { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } },
		{ { -1, -1 }, { -1, 1 }, { 1, 1 }, { 1, -1 } },
	};
	const uint32_t first = b->vertex_count;
	voe_math_float3 face = voe_math_float3_cross(c.across, c.along);
	const bool flip = voe_math_float3_dot(
				  face, voe_math_float3_sub(c.centre, crown)) < 0;

	face = flip ? voe_math_float3_neg(face) : face;
	for (uint32_t k = 0; k < 4; k++) {
		const float sa = signs[flip][k][0], sl = signs[flip][k][1];
		const voe_math_float3 p = voe_math_float3_add(
			c.centre,
			voe_math_float3_add(
				voe_math_float3_scale(c.across, sa * half),
				voe_math_float3_scale(c.along, sl * half)));
		const voe_math_float3 out = voe_math_float3_sub(p, crown);
		const float far = voe_math_float3_length(out);
		const voe_math_float3 bent =
			far > 1e-6f ? voe_math_float3_add(
					      face, voe_math_float3_scale(
							    out, 1.0f / far)) :
				      face;
		const float bent_length = voe_math_float3_length(bent);

		put_vertex(b, p,
			   bent_length > 1e-6f ?
				   voe_math_float3_scale(bent,
							 1.0f / bent_length) :
				   face,
			   (voe_math_float2){ 0.5f * (sa + 1.0f),
					      0.5f * (sl + 1.0f) });
	}
	for (uint32_t q = 0; q < 6; q++) {
		static const uint32_t quad[6] = { 0, 1, 2, 0, 2, 3 };

		b->indices[b->index_count++] = first + quad[q];
	}
}

// Every 2^level-th card, scaled by √(2^level) about its centre.
static voe_assets_primitive leaves(voe_base_arena *arena, const card *cards,
				   uint32_t count, float size,
				   voe_math_float3 crown, uint32_t level)
{
	const uint32_t every = 1u << level;
	const uint32_t kept = (count + every - 1) / every;
	const float half = 0.5f * size * sqrtf((float)every);
	build b = build_of(arena, kept * 4, kept * 6);

	for (uint32_t i = 0; i < count; i += every)
		put_card(&b, cards[i], half, crown);
	VOE_BASE_ASSERT(b.vertex_count == kept * 4, "the cards counted");
	return primitive_of(b, 1);
}

static voe_assets_material material_of(const char *name)
{
	voe_assets_material m = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.alpha_cutoff = 0.5f,
		.emissive_strength = 1.0f,
		.colour_scale = { 1.0f, 1.0f },
		.base_colour_image = VOE_ASSETS_MODEL_NONE,
		.metallic_roughness_image = VOE_ASSETS_MODEL_NONE,
		.normal_image = VOE_ASSETS_MODEL_NONE,
		.occlusion_image = VOE_ASSETS_MODEL_NONE,
		.emissive_image = VOE_ASSETS_MODEL_NONE,
	};

	snprintf(m.name.text, sizeof(m.name.text), "%s", name);
	return m;
}

static voe_math_float3 crown_of(const card *cards, uint32_t count)
{
	voe_math_float3 sum = { 0 };

	for (uint32_t i = 0; i < count; i++)
		sum = voe_math_float3_add(sum, cards[i].centre);
	return count > 0 ? voe_math_float3_scale(sum, 1.0f / (float)count) :
			   sum;
}

void voe_3d_tree_mesh(voe_base_arena *arena,
		      const voe_3d_tree_skeleton *skeleton,
		      const voe_assets_tree *values, uint32_t variation,
		      voe_assets_model *out)
{
	card *cards = voe_base_arena_push(arena,
					  sizeof(card) * VOE_3D_TREE_LEAVES);
	voe_assets_primitive *primitives = voe_base_arena_push(
		arena, sizeof(*primitives) * 2 * VOE_3D_TREE_LEVELS);
	voe_assets_mesh *meshes =
		voe_base_arena_push(arena, sizeof(*meshes) * VOE_3D_TREE_LEVELS);
	voe_assets_node *nodes =
		voe_base_arena_push(arena, sizeof(*nodes) * VOE_3D_TREE_LEVELS);
	uint32_t *roots =
		voe_base_arena_push(arena, sizeof(*roots) * VOE_3D_TREE_LEVELS);
	voe_assets_material *materials =
		voe_base_arena_push(arena, sizeof(*materials) * 2);
	uint32_t card_count;
	voe_math_float3 crown;

	VOE_BASE_ASSERT(skeleton != NULL && skeleton->count > 0 &&
				values != NULL && out != NULL,
			"a grown skeleton, its values and an out");
	card_count = place_cards(skeleton, values, variation, cards);
	crown = crown_of(cards, card_count);
	for (uint32_t k = 0; k < VOE_3D_TREE_LEVELS; k++) {
		primitives[2 * k] = bark(arena, skeleton, k);
		primitives[2 * k + 1] = leaves(arena, cards, card_count,
					       values->leaf_size, crown, k);
		meshes[k] = (voe_assets_mesh){ 2 * k, 2 };
		nodes[k] = (voe_assets_node){
			.local = voe_math_float4x4_identity(),
			.mesh = k,
		};
		snprintf(nodes[k].name.text, sizeof(nodes[k].name.text),
			 "tree_LOD%u", k);
		roots[k] = k;
	}
	materials[0] = material_of("bark");
	materials[1] = material_of("leaf");
	*out = (voe_assets_model){
		.primitives = primitives,
		.primitive_count = 2 * VOE_3D_TREE_LEVELS,
		.meshes = meshes,
		.mesh_count = VOE_3D_TREE_LEVELS,
		.materials = materials,
		.material_count = 2,
		.nodes = nodes,
		.node_count = VOE_3D_TREE_LEVELS,
		.roots = roots,
		.root_count = VOE_3D_TREE_LEVELS,
	};
}
