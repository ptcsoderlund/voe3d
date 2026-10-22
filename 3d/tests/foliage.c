// The foliage arithmetic of 3d/foliage.h on a flat 16-cell 64 m grid with two
// kinds and room for the cap: a held scatter stamp filling to about its
// density and stopping, spaced, of the picked kind and within its radius; a
// seed giving the same copies twice; erase by kind and undo's put both ways; a
// copy leaning on a 45° plane or standing upright; _replace following a
// raised height; and a kind's base size scaling its copy. Needs no graphics
// card.
#include <3d/foliage.h>
#include <3d/landscape.h>
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ARENA (32 * 1024 * 1024)
#define STAMPS 100

struct grown {
	voe_assets_landscape land;
	voe_scene_transform *placed;
};

static struct grown flat_with_room(voe_base_arena *arena)
{
	struct grown out = {
		.land = voe_assets_landscape_flat(64.0f, 16, arena),
	};

	out.land.kind_count = 2;
	out.land.copies = voe_base_arena_push(
		arena, sizeof(voe_assets_landscape_copy) *
			       VOE_ASSETS_LANDSCAPE_COPIES);
	out.placed = voe_base_arena_push(arena, sizeof(voe_scene_transform) *
							VOE_ASSETS_LANDSCAPE_COPIES);
	return out;
}

static const voe_3d_foliage_brush kind_one = {
	.radius = 10.0f,
	.softness = 0.0f,
	.density = 4.0f,
	.size_vary = 0.2f,
	.turn_vary = 3.14159265f,
	.lean = 0.0f,
	.picked = 1u << 1,
};

// 100 stamps of 0.1 s at the centre; how many they said they added.
static uint32_t hold(struct grown *g, uint64_t seed)
{
	voe_assets_landscape_copy added[VOE_3D_FOLIAGE_TRIES];
	uint32_t total = 0;

	for (int i = 0; i < STAMPS; i++)
		total += voe_3d_foliage_scatter(&g->land, g->placed, &kind_one,
						0.0f, 0.0f, 0.1f, &seed, added);
	return total;
}

static bool same(voe_assets_landscape_copy a, voe_assets_landscape_copy b)
{
	return a.x == b.x && a.z == b.z && a.turn == b.turn &&
	       a.scale == b.scale && a.kind == b.kind && a.lean == b.lean;
}

// Spacing 0.84·√(100 / 4) = 4.2 m; about 4 a 100 m² over the 314 m² disc.
static void a_held_scatter_fills_to_the_density(voe_base_arena *arena)
{
	struct grown g = flat_with_room(arena);
	const uint32_t total = hold(&g, 0x9e3779b97f4a7c15u);
	const float spacing = 0.84f * sqrtf(100.0f / 4.0f);
	bool kind_one_within = true, spaced = true, placed = true;

	VOE_TEST_CHECK_INT(total, g.land.copy_count);
	VOE_TEST_CHECK(g.land.copy_count >= 6 && g.land.copy_count <= 16);
	for (uint32_t i = 0; i < g.land.copy_count; i++) {
		const voe_assets_landscape_copy c = g.land.copies[i];
		const float x = (float)c.x * 0.001f, z = (float)c.z * 0.001f;

		kind_one_within &= c.kind == 1 && x * x + z * z <= 100.001f;
		placed &= g.placed[i].position.x == (double)x &&
			  g.placed[i].scale.x == (float)c.scale * 0.001f;
		for (uint32_t j = 0; j < i; j++) {
			const float dx = (float)(c.x - g.land.copies[j].x) * 0.001f;
			const float dz = (float)(c.z - g.land.copies[j].z) * 0.001f;

			spaced &= dx * dx + dz * dz >= spacing * spacing;
		}
	}
	VOE_TEST_CHECK(kind_one_within);
	VOE_TEST_CHECK(spaced);
	VOE_TEST_CHECK(placed);
}

static void a_seed_gives_the_same_copies(voe_base_arena *arena)
{
	struct grown a = flat_with_room(arena);
	struct grown b = flat_with_room(arena);
	bool equal = true;

	(void)hold(&a, 12345u);
	(void)hold(&b, 12345u);
	VOE_TEST_CHECK_INT(a.land.copy_count, b.land.copy_count);
	for (uint32_t i = 0; i < a.land.copy_count; i++)
		equal &= same(a.land.copies[i], b.land.copies[i]);
	VOE_TEST_CHECK(equal);
}

// Erasing kind 0 takes nothing; kind 1 takes all, and undo puts them back and
// takes them again.
static void erase_and_put_by_value(voe_base_arena *arena)
{
	struct grown g = flat_with_room(arena);
	voe_3d_foliage_brush brush = kind_one;
	voe_assets_landscape_copy removed[64];
	const uint32_t count = (hold(&g, 777u), g.land.copy_count);

	brush.picked = 1u << 0;
	VOE_TEST_CHECK_INT(voe_3d_foliage_erase(&g.land, g.placed, &brush, 0.0f,
						0.0f, removed, 64),
			   0);
	VOE_TEST_CHECK_INT(g.land.copy_count, count);
	brush.picked = 1u << 1;
	VOE_TEST_CHECK_INT(voe_3d_foliage_erase(&g.land, g.placed, &brush, 0.0f,
						0.0f, removed, 64),
			   count);
	VOE_TEST_CHECK_INT(g.land.copy_count, 0);
	VOE_TEST_CHECK(count == 0 || removed[0].kind == 1);
	voe_3d_foliage_put(&g.land, g.placed, removed, count, true);
	VOE_TEST_CHECK_INT(g.land.copy_count, count);
	VOE_TEST_CHECK(count == 0 || same(g.land.copies[0], removed[0]));
	voe_3d_foliage_put(&g.land, g.placed, removed, count, false);
	VOE_TEST_CHECK_INT(g.land.copy_count, 0);
}

// +Y turned by q: v + 2w(q × v) + 2 q × (q × v).
static voe_math_float3 up_of(voe_math_quat q)
{
	const voe_math_float3 v = { 0.0f, 1.0f, 0.0f };
	const voe_math_float3 u = { q.x, q.y, q.z };
	const voe_math_float3 t = voe_math_float3_cross(u, v);
	const voe_math_float3 tt = voe_math_float3_cross(u, t);

	return (voe_math_float3){ v.x + 2.0f * (q.w * t.x + tt.x),
				  v.y + 2.0f * (q.w * t.y + tt.y),
				  v.z + 2.0f * (q.w * t.z + tt.z) };
}

// Heights rising 1 m a metre along x: a copy 1 m out stands at y 1; lean 255
// turns its up 45° toward (-1, 1, 0), lean 0 leaves it upright.
static void a_copy_leans_on_a_slope(voe_base_arena *arena)
{
	struct grown g = flat_with_room(arena);
	const uint32_t n = g.land.cells + 1;
	voe_assets_landscape_copy copy = { .x = 1000, .scale = 1000, .lean = 255 };

	for (uint32_t r = 0; r < n; r++)
		for (uint32_t c = 0; c < n; c++)
			g.land.heights[r * n + c] = -32.0f + 4.0f * (float)c;
	const voe_scene_transform leant = voe_3d_foliage_place(&g.land, copy);
	const voe_math_float3 up = up_of(leant.rotation);

	VOE_TEST_CHECK_FLOAT((float)leant.position.y, 1.0f, 1e-4f);
	VOE_TEST_CHECK_FLOAT(acosf(up.y), 0.25f * 3.14159265f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(up.x, -sqrtf(0.5f), 1e-3f);
	copy.lean = 0;
	const voe_scene_transform upright = voe_3d_foliage_place(&g.land, copy);

	VOE_TEST_CHECK_FLOAT(up_of(upright.rotation).y, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)upright.position.y, 1.0f, 1e-4f);
}

// A copy at the centre on the flat; the height under it raised 2 m, then the
// rect of that one height placed again.
static void replace_follows_a_raised_height(voe_base_arena *arena)
{
	struct grown g = flat_with_room(arena);
	const voe_assets_landscape_copy copy = { .scale = 1000 };

	voe_3d_foliage_put(&g.land, g.placed, &copy, 1, true);
	VOE_TEST_CHECK_FLOAT((float)g.placed[0].position.y, 0.0f, 0.0f);
	g.land.heights[8 * 17 + 8] = 2.0f;
	voe_3d_foliage_replace(&g.land, g.placed,
			       (voe_3d_landscape_rect){ 8, 8, 9, 9 });
	VOE_TEST_CHECK_FLOAT((float)g.placed[0].position.y, 2.0f, 1e-5f);
}

// Two copies alike but for their kind, kind 1 at base size 2×: kind 1 stands
// twice kind 0's scale, where kind 0 does, turned alike.
static void a_kinds_base_size_scales_its_copies(voe_base_arena *arena)
{
	struct grown g = flat_with_room(arena);
	const voe_assets_landscape_copy small = { .x = 2000, .z = -3000,
						  .turn = 9000, .scale = 1200 };
	voe_assets_landscape_copy big = small;

	big.kind = 1;
	g.land.kind_sizes[1] = 2000;
	const voe_scene_transform a = voe_3d_foliage_place(&g.land, small);
	const voe_scene_transform b = voe_3d_foliage_place(&g.land, big);

	VOE_TEST_CHECK_FLOAT(a.scale.x, 1.2f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(b.scale.x, 2.0f * a.scale.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(b.scale.y, 2.0f * a.scale.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(b.scale.z, 2.0f * a.scale.z, 0.0f);
	VOE_TEST_CHECK(b.position.x == a.position.x &&
		       b.position.y == a.position.y &&
		       b.position.z == a.position.z);
	VOE_TEST_CHECK(b.rotation.x == a.rotation.x &&
		       b.rotation.y == a.rotation.y &&
		       b.rotation.z == a.rotation.z &&
		       b.rotation.w == a.rotation.w);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	a_held_scatter_fills_to_the_density(arena);
	a_seed_gives_the_same_copies(arena);
	erase_and_put_by_value(arena);
	a_copy_leans_on_a_slope(arena);
	replace_follows_a_raised_height(arena);
	a_kinds_base_size_scales_its_copies(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
