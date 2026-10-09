// The min/max pyramid and the node selection of 3d/landscape.h (0396 points 3
// and 4): a leaf's lowest and highest height from the grid, a parent's from
// its up to four children, the root as the grid's box, and the walk from the
// root that splits nodes within their children's range.
//
// Used where a landscape is drawn: the pyramid is built once a load, updated
// over each stamp's rect, and selected from once a pass.
//
// Constraints: a build walks every height once per leaf it lies in (an edge
// twice); an update walks the leaves over its rect and their ancestors only.
// The selection makes one pass per level over at most `capacity` nodes, coarse
// to fine, so room runs out on the finest splits first. Nothing allocates but
// on the arena handed to the build.
#include <3d/landscape.h>

#include <base/assert.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#define QUADS VOE_3D_LANDSCAPE_NODE_QUADS
#define MORPH_SHARE 0.3f

static_assert((QUADS << (VOE_3D_LANDSCAPE_LOD_LEVELS - 1)) >=
	      VOE_ASSETS_LANDSCAPE_CELLS_MAX);

static uint32_t node_cells(uint32_t level)
{
	return (uint32_t)QUADS << level;
}

static float at(const voe_assets_landscape *landscape, uint32_t c, uint32_t r)
{
	return landscape->heights[(size_t)r * (landscape->cells + 1) + c];
}

static size_t node_index(const voe_3d_landscape_lod *lod, uint32_t level,
			 uint32_t i, uint32_t j)
{
	return lod->first[level] + (size_t)j * lod->sides[level] + i;
}

// Leaf (i, j)'s lowest and highest of the heights it holds, edges included.
static void refresh_leaf(voe_3d_landscape_lod *lod,
			 const voe_assets_landscape *landscape, uint32_t i,
			 uint32_t j)
{
	VOE_BASE_ASSERT(i < lod->sides[0] && j < lod->sides[0], "a leaf");
	const uint32_t c0 = i * QUADS, r0 = j * QUADS;
	const uint32_t c1 = c0 + QUADS < landscape->cells ? c0 + QUADS :
							     landscape->cells;
	const uint32_t r1 = r0 + QUADS < landscape->cells ? r0 + QUADS :
							     landscape->cells;
	float low = at(landscape, c0, r0), high = low;

	for (uint32_t r = r0; r <= r1; r++) {
		for (uint32_t c = c0; c <= c1; c++) {
			low = fminf(low, at(landscape, c, r));
			high = fmaxf(high, at(landscape, c, r));
		}
	}
	lod->lows[node_index(lod, 0, i, j)] = low;
	lod->highs[node_index(lod, 0, i, j)] = high;
}

// Node (i, j) at `level` from its children inside the grid at level − 1.
static void refresh_parent(voe_3d_landscape_lod *lod, uint32_t level,
			   uint32_t i, uint32_t j)
{
	VOE_BASE_ASSERT(level >= 1 && level < lod->levels, "a parent level");
	const uint32_t below = level - 1;
	float low = INFINITY, high = -INFINITY;

	for (uint32_t q = 0; q < 4; q++) {
		const uint32_t ci = 2 * i + (q & 1), cj = 2 * j + (q >> 1);

		if (ci >= lod->sides[below] || cj >= lod->sides[below])
			continue;
		low = fminf(low, lod->lows[node_index(lod, below, ci, cj)]);
		high = fmaxf(high, lod->highs[node_index(lod, below, ci, cj)]);
	}
	VOE_BASE_ASSERT(low <= high, "a parent has its first child");
	lod->lows[node_index(lod, level, i, j)] = low;
	lod->highs[node_index(lod, level, i, j)] = high;
}

voe_3d_landscape_lod voe_3d_landscape_lod_build(const voe_assets_landscape *landscape,
						voe_base_arena *arena)
{
	VOE_BASE_ASSERT(landscape && landscape->heights && arena,
			"a landscape and an arena");
	VOE_BASE_ASSERT(landscape->cells >= 1 &&
				landscape->cells <= VOE_ASSETS_LANDSCAPE_CELLS_MAX,
			"cells within the most a grid has");
	voe_3d_landscape_lod lod = { 0 };
	uint32_t total = 0;

	for (uint32_t level = 0; level < VOE_3D_LANDSCAPE_LOD_LEVELS; level++) {
		const uint32_t cells = node_cells(level);

		lod.sides[level] = (landscape->cells + cells - 1) / cells;
		lod.first[level] = total;
		total += lod.sides[level] * lod.sides[level];
		lod.levels = level + 1;
		if (cells >= landscape->cells)
			break;
	}
	lod.lows = voe_base_arena_push(arena, sizeof(float) * total);
	lod.highs = voe_base_arena_push(arena, sizeof(float) * total);
	voe_3d_landscape_lod_update(
		&lod, landscape,
		(voe_3d_landscape_rect){ .x1 = landscape->cells + 1,
					 .z1 = landscape->cells + 1 });
	VOE_BASE_ASSERT(lod.sides[lod.levels - 1] == 1, "one root");
	return lod;
}

void voe_3d_landscape_lod_update(voe_3d_landscape_lod *lod,
				 const voe_assets_landscape *landscape,
				 voe_3d_landscape_rect heights)
{
	VOE_BASE_ASSERT(lod && lod->lows && landscape && landscape->heights,
			"a pyramid and its landscape");
	VOE_BASE_ASSERT(heights.x1 <= landscape->cells + 1 &&
				heights.z1 <= landscape->cells + 1,
			"a rect on the grid");
	if (heights.x0 >= heights.x1 || heights.z0 >= heights.z1)
		return;
	// Leaf i holds heights i·32 to i·32 + 32, both ends, as chunks do.
	const uint32_t last = lod->sides[0] - 1;
	uint32_t x0 = heights.x0 == 0 ? 0 : (heights.x0 - 1) / QUADS;
	uint32_t z0 = heights.z0 == 0 ? 0 : (heights.z0 - 1) / QUADS;
	uint32_t x1 = (heights.x1 - 1) / QUADS < last ? (heights.x1 - 1) / QUADS :
							  last;
	uint32_t z1 = (heights.z1 - 1) / QUADS < last ? (heights.z1 - 1) / QUADS :
							  last;

	for (uint32_t j = z0; j <= z1; j++)
		for (uint32_t i = x0; i <= x1; i++)
			refresh_leaf(lod, landscape, i, j);
	for (uint32_t level = 1; level < lod->levels; level++) {
		x0 /= 2, z0 /= 2, x1 /= 2, z1 /= 2;
		for (uint32_t j = z0; j <= z1; j++)
			for (uint32_t i = x0; i <= x1; i++)
				refresh_parent(lod, level, i, j);
	}
}

void voe_3d_landscape_lod_box(const voe_3d_landscape_lod *lod,
			      const voe_assets_landscape *landscape,
			      voe_math_float3 *min, voe_math_float3 *max)
{
	VOE_BASE_ASSERT(lod && lod->levels >= 1 && landscape,
			"a pyramid and its landscape");
	VOE_BASE_ASSERT(min && max, "somewhere to put the box");
	const size_t root = lod->first[lod->levels - 1];

	*min = (voe_math_float3){ -0.5f * landscape->size, lod->lows[root],
				  -0.5f * landscape->size };
	*max = (voe_math_float3){ 0.5f * landscape->size, lod->highs[root],
				  0.5f * landscape->size };
}

// Level 0's range: twice a leaf's diagonal.
static float leaf_range(const voe_assets_landscape *landscape)
{
	const float side = (float)QUADS * landscape->size / (float)landscape->cells;

	return 2.0f * sqrtf(2.0f) * side;
}

static voe_3d_landscape_node node_at(const voe_3d_landscape_lod *lod,
				     const voe_assets_landscape *landscape,
				     uint32_t level, uint32_t i, uint32_t j)
{
	VOE_BASE_ASSERT(level < lod->levels && i < lod->sides[level] &&
				j < lod->sides[level],
			"a node inside the grid");
	const float side = (float)node_cells(level) * landscape->size /
			   (float)landscape->cells;
	const float range = leaf_range(landscape) * (float)(1u << level);
	const size_t k = node_index(lod, level, i, j);

	return (voe_3d_landscape_node){
		.x = -0.5f * landscape->size + (float)i * side,
		.z = -0.5f * landscape->size + (float)j * side,
		.side = side,
		.low = lod->lows[k],
		.high = lod->highs[k],
		.level = level,
		.morph_start = (1.0f - MORPH_SHARE) * range,
		.morph_end = range,
	};
}

// Metres from the eye to a node's square clipped to the grid, `vertical`
// standing in for the height: the header says why it is the whole grid's.
static float node_distance(const voe_assets_landscape *landscape,
			   voe_3d_landscape_node node, voe_math_float3 eye,
			   float vertical)
{
	const float half = 0.5f * landscape->size;
	const float far_x = fminf(node.x + node.side, half);
	const float far_z = fminf(node.z + node.side, half);
	const float dx = fmaxf(fmaxf(node.x - eye.x, eye.x - far_x), 0.0f);
	const float dz = fmaxf(fmaxf(node.z - eye.z, eye.z - far_z), 0.0f);

	return sqrtf(dx * dx + dz * dz + vertical * vertical);
}

uint32_t voe_3d_landscape_select(const voe_3d_landscape_lod *lod,
				 const voe_assets_landscape *landscape,
				 voe_math_float3 eye, voe_3d_landscape_node *nodes,
				 uint32_t capacity)
{
	VOE_BASE_ASSERT(lod && lod->levels >= 1 && landscape && nodes,
			"a pyramid, its landscape and room for nodes");
	VOE_BASE_ASSERT(capacity >= 1, "room for the root");
	const uint32_t top = lod->levels - 1;
	const float half = 0.5f * landscape->size;
	uint32_t count = 1;

	nodes[0] = node_at(lod, landscape, top, 0, 0);
	const float vertical = fmaxf(fmaxf(nodes[0].low - eye.y,
					   eye.y - nodes[0].high),
				     0.0f);

	for (uint32_t level = top; level > 0; level--) {
		const float range =
			leaf_range(landscape) * (float)(1u << (level - 1));
		const uint32_t walked = count;

		for (uint32_t k = 0; k < walked && count + 3 <= capacity; k++) {
			const voe_3d_landscape_node node = nodes[k];

			if (node.level != level ||
			    node_distance(landscape, node, eye, vertical) > range)
				continue;
			const uint32_t i = 2 * (uint32_t)lroundf(
						       (node.x + half) / node.side);
			const uint32_t j = 2 * (uint32_t)lroundf(
						       (node.z + half) / node.side);

			nodes[k] = node_at(lod, landscape, level - 1, i, j);
			for (uint32_t q = 1; q < 4; q++) {
				const uint32_t ci = i + (q & 1), cj = j + (q >> 1);

				if (ci < lod->sides[level - 1] &&
				    cj < lod->sides[level - 1])
					nodes[count++] = node_at(
						lod, landscape, level - 1, ci, cj);
			}
		}
	}
	VOE_BASE_ASSERT(count >= 1 && count <= capacity, "within capacity");
	return count;
}
