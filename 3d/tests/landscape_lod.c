// The pyramid and the selection of 3d/landscape.h on a 2048-cell 4096 m grid
// of hills, eyes on a 9 × 9 lattice 2 m above the ground and 3 km up: the
// chosen nodes cover the square once, neighbours differ by at most a level,
// never more than the capacity and 4 still cover, each node's low and high
// bound its heights, an update after a stamp equals a fresh build, and the
// pyramid's box is the heights'. Needs no graphics card.
#include <3d/landscape.h>
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define ARENA (1024 * 1024)
#define CELLS 2048
#define SIZE 4096.0f
#define LEAVES (CELLS / VOE_3D_LANDSCAPE_NODE_QUADS)
#define CAPACITY 1024
#define LATTICE 9
#define UNSET UINT32_MAX

static voe_3d_landscape_node nodes[CAPACITY];
// Each leaf's chosen level, UNSET where no node covers it.
static uint32_t leaf_levels[LEAVES * LEAVES];

static voe_assets_landscape hills(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(SIZE, CELLS, arena);
	const float cell = SIZE / CELLS;

	for (uint32_t r = 0; r <= CELLS; r++) {
		for (uint32_t c = 0; c <= CELLS; c++) {
			const float x = -0.5f * SIZE + (float)c * cell;
			const float z = -0.5f * SIZE + (float)r * cell;

			land.heights[(size_t)r * (CELLS + 1) + c] =
				150.0f * sinf(x * 0.004f) * cosf(z * 0.003f) +
				40.0f * sinf(x * 0.02f + z * 0.013f);
		}
	}
	return land;
}

static uint32_t to_index(float metres, float step)
{
	return (uint32_t)lroundf((metres + 0.5f * SIZE) / step);
}

// Marks each node's leaves with its level; true when every leaf is marked
// once and the areas sum to the square's.
static bool covers_once(uint32_t count)
{
	const float leaf = SIZE / LEAVES;
	double area = 0.0;

	for (uint32_t k = 0; k < LEAVES * LEAVES; k++)
		leaf_levels[k] = UNSET;
	for (uint32_t k = 0; k < count; k++) {
		const uint32_t i = to_index(nodes[k].x, leaf);
		const uint32_t j = to_index(nodes[k].z, leaf);
		const uint32_t n = 1u << nodes[k].level;

		area += (double)nodes[k].side * nodes[k].side;
		for (uint32_t dj = 0; dj < n; dj++) {
			for (uint32_t di = 0; di < n; di++) {
				if (i + di >= LEAVES || j + dj >= LEAVES)
					return false;
				uint32_t *at = &leaf_levels[(j + dj) * LEAVES + i + di];

				if (*at != UNSET)
					return false;
				*at = nodes[k].level;
			}
		}
	}
	for (uint32_t k = 0; k < LEAVES * LEAVES; k++)
		if (leaf_levels[k] == UNSET)
			return false;
	return area == (double)SIZE * SIZE;
}

// Leaves side by side in different nodes are those nodes sharing an edge.
static bool neighbours_within_a_level(void)
{
	for (uint32_t j = 0; j < LEAVES; j++) {
		for (uint32_t i = 0; i < LEAVES; i++) {
			const int a = (int)leaf_levels[j * LEAVES + i];

			if (i + 1 < LEAVES &&
			    abs(a - (int)leaf_levels[j * LEAVES + i + 1]) > 1)
				return false;
			if (j + 1 < LEAVES &&
			    abs(a - (int)leaf_levels[(j + 1) * LEAVES + i]) > 1)
				return false;
		}
	}
	return true;
}

static bool bounds_hold(const voe_assets_landscape *land, uint32_t count)
{
	const float cell = SIZE / CELLS;

	for (uint32_t k = 0; k < count; k++) {
		const uint32_t c0 = to_index(nodes[k].x, cell);
		const uint32_t r0 = to_index(nodes[k].z, cell);
		const uint32_t n = VOE_3D_LANDSCAPE_NODE_QUADS << nodes[k].level;

		for (uint32_t r = r0; r <= r0 + n; r++) {
			for (uint32_t c = c0; c <= c0 + n; c++) {
				const float h =
					land->heights[(size_t)r * (CELLS + 1) + c];

				if (h < nodes[k].low || h > nodes[k].high)
					return false;
			}
		}
	}
	return true;
}

static voe_math_float3 eye_at(const voe_assets_landscape *land, uint32_t a,
			      uint32_t b, bool high)
{
	const float x = -0.5f * SIZE + (float)a * SIZE / (LATTICE - 1);
	const float z = -0.5f * SIZE + (float)b * SIZE / (LATTICE - 1);

	return (voe_math_float3){
		x, high ? 3000.0f : voe_3d_landscape_height(land, x, z) + 2.0f,
		z
	};
}

// Every eye: covered once, neighbours within a level, refining never cut
// short; on the diagonal each node's bounds checked against its heights.
static void lattice_covers_once_within_a_level(const voe_3d_landscape_lod *lod,
					       const voe_assets_landscape *land)
{
	for (uint32_t h = 0; h < 2; h++) {
		for (uint32_t b = 0; b < LATTICE; b++) {
			for (uint32_t a = 0; a < LATTICE; a++) {
				const uint32_t count = voe_3d_landscape_select(
					lod, land, eye_at(land, a, b, h == 1),
					nodes, CAPACITY);

				VOE_TEST_CHECK(count + 3 <= CAPACITY);
				VOE_TEST_CHECK(covers_once(count));
				VOE_TEST_CHECK(neighbours_within_a_level());
				if (a == b)
					VOE_TEST_CHECK(bounds_hold(land, count));
			}
		}
	}
}

// 64 nodes and 4 nodes: never more, and the square still covered once.
static void capacity_is_never_passed(const voe_3d_landscape_lod *lod,
				     const voe_assets_landscape *land)
{
	const uint32_t capacities[] = { 64, 4 };

	for (uint32_t k = 0; k < 2; k++) {
		for (uint32_t b = 0; b < LATTICE; b++) {
			for (uint32_t a = 0; a < LATTICE; a++) {
				const uint32_t count = voe_3d_landscape_select(
					lod, land, eye_at(land, a, b, false),
					nodes, capacities[k]);

				VOE_TEST_CHECK(count <= capacities[k]);
				VOE_TEST_CHECK(covers_once(count));
			}
		}
	}
}

static void update_after_a_stamp_is_a_fresh_build(voe_3d_landscape_lod *lod,
						   voe_assets_landscape *land,
						   voe_base_arena *arena)
{
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_RAISE,
				     .radius = 150.0f,
				     .strength = 1.0f,
				     .softness = 0.5f };
	const voe_3d_landscape_rect rect = voe_3d_landscape_brush(
		land, &brush, 300.0f, -200.0f, 0.1f, arena);

	VOE_TEST_CHECK(rect.x0 < rect.x1 && rect.z0 < rect.z1);
	voe_3d_landscape_lod_update(lod, land, rect);
	const voe_3d_landscape_lod fresh = voe_3d_landscape_lod_build(land, arena);
	const uint32_t top = fresh.levels - 1;
	const uint32_t total = fresh.first[top] + fresh.sides[top] * fresh.sides[top];
	bool same = fresh.levels == lod->levels;

	VOE_TEST_CHECK_INT(fresh.levels, 7);
	for (uint32_t k = 0; same && k < total; k++)
		same = fresh.lows[k] == lod->lows[k] &&
		       fresh.highs[k] == lod->highs[k];
	VOE_TEST_CHECK(same);
}

static void lod_box_is_the_heights_box(const voe_3d_landscape_lod *lod,
				       const voe_assets_landscape *land)
{
	voe_math_float3 a_min, a_max, b_min, b_max;

	voe_3d_landscape_lod_box(lod, land, &a_min, &a_max);
	voe_3d_landscape_box(land, &b_min, &b_max);
	VOE_TEST_CHECK_FLOAT(a_min.x, b_min.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(a_min.y, b_min.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(a_min.z, b_min.z, 0.0f);
	VOE_TEST_CHECK_FLOAT(a_max.x, b_max.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(a_max.y, b_max.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(a_max.z, b_max.z, 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);
	voe_assets_landscape land = hills(arena);
	voe_3d_landscape_lod lod = voe_3d_landscape_lod_build(&land, arena);

	lattice_covers_once_within_a_level(&lod, &land);
	capacity_is_never_passed(&lod, &land);
	update_after_a_stamp_is_a_fresh_build(&lod, &land, arena);
	lod_box_is_the_heights_box(&lod, &land);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
