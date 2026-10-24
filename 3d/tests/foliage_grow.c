// The grown foliage of 3d/foliage_grow.h on a flat 256 m grid of 64 cells
// with two kinds: a layer growing 4 copies per 100 m² over the whole grid
// grows about 0.04 × 256²; paint on the west half grows nothing past a cell
// east of it; a second layer at half the tenths on the east half grows about
// half as thick; a rect remade after a repaint equals a whole grow on the new
// paint; remaking one kind leaves the other's copies; and a 512 m grid at the
// most two kinds can grow stops at the cap. Needs no graphics card.
#include <3d/foliage_grow.h>
#include <3d/foliage.h>
#include <3d/landscape.h>
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARENA (64 * 1024 * 1024)
#define CELLS 64

struct grown {
	voe_assets_landscape land;
	voe_assets_landscape_copy *copies;
	voe_scene_transform *placed;
	uint32_t count;
};

static struct grown flat(float size, uint32_t layers, voe_base_arena *arena)
{
	const uint32_t n = CELLS + 1;
	struct grown out = {
		.land = voe_assets_landscape_flat(size, CELLS, arena),
	};

	out.land.layer_count = layers;
	out.land.kind_count = 2;
	out.land.looks[0] = voe_assets_landscape_look_default();
	out.land.looks[1] = voe_assets_landscape_look_default();
	out.land.paint = voe_base_arena_push(arena, (size_t)2 * n * n * 4);
	memset(out.land.paint, 0, (size_t)2 * n * n * 4);
	for (uint32_t i = 0; i < n * n; i++)
		out.land.paint[i * 4] = 255;
	out.copies = voe_base_arena_push(arena, sizeof(voe_assets_landscape_copy) *
							VOE_3D_FOLIAGE_GROWN);
	out.placed = voe_base_arena_push(arena, sizeof(voe_scene_transform) *
							VOE_3D_FOLIAGE_GROWN);
	return out;
}

// Point (c, r) given `weight` of `layer` (1-3) and the rest to layer 0.
static void paint(voe_assets_landscape *land, uint32_t c, uint32_t r,
		  uint32_t layer, uint8_t weight)
{
	uint8_t *at = &land->paint[(r * (CELLS + 1) + c) * 4];

	at[1] = at[2] = at[3] = 0;
	at[layer] = weight;
	at[0] = (uint8_t)(255 - weight);
}

static void paint_columns(voe_assets_landscape *land, uint32_t c0, uint32_t c1,
			  uint32_t layer)
{
	for (uint32_t r = 0; r <= CELLS; r++)
		for (uint32_t c = c0; c < c1; c++)
			paint(land, c, r, layer, 255);
}

static const voe_3d_landscape_rect whole = { 0, 0, CELLS + 1, CELLS + 1 };

static void grow(struct grown *g, voe_3d_landscape_rect rect, uint32_t kinds)
{
	g->count = voe_3d_foliage_grow(&g->land, rect, kinds, g->copies,
				       g->placed, g->count);
}

static int by_kind_x_z(const void *pa, const void *pb)
{
	const voe_assets_landscape_copy *a = pa, *b = pb;

	if (a->kind != b->kind)
		return a->kind < b->kind ? -1 : 1;
	if (a->x != b->x)
		return a->x < b->x ? -1 : 1;
	return a->z < b->z ? -1 : a->z > b->z;
}

static bool same(voe_assets_landscape_copy a, voe_assets_landscape_copy b)
{
	return a.x == b.x && a.z == b.z && a.turn == b.turn &&
	       a.scale == b.scale && a.kind == b.kind && a.lean == b.lean;
}

// Kind `kind`'s copies, or every kind's for kind 2, copied to `out` sorted;
// how many.
static uint32_t sorted(const struct grown *g, uint32_t kind,
		       voe_assets_landscape_copy *out)
{
	uint32_t n = 0;

	for (uint32_t i = 0; i < g->count; i++)
		if (kind == 2 || g->copies[i].kind == kind)
			out[n++] = g->copies[i];
	qsort(out, n, sizeof(*out), by_kind_x_z);
	return n;
}

static bool equal_sorted(const struct grown *a, const struct grown *b,
			 uint32_t kind, voe_base_arena *arena)
{
	voe_assets_landscape_copy *x = voe_base_arena_push(
		arena, sizeof(*x) * VOE_3D_FOLIAGE_GROWN);
	voe_assets_landscape_copy *y = voe_base_arena_push(
		arena, sizeof(*y) * VOE_3D_FOLIAGE_GROWN);
	const uint32_t n = sorted(a, kind, x);
	bool equal = n == sorted(b, kind, y) && n > 0;

	for (uint32_t i = 0; i < n && equal; i++)
		equal = same(x[i], y[i]);
	return equal;
}

// Spacing 5 m; every lattice point kept, about 2621.
static void a_layer_grows_its_density(voe_base_arena *arena)
{
	struct grown g = flat(256.0f, 2, arena);
	const float expected = 0.04f * 256.0f * 256.0f;

	paint_columns(&g.land, 0, CELLS + 1, 1);
	g.land.grows[1][0] = 40;
	grow(&g, whole, 1u);
	VOE_TEST_CHECK(fabsf((float)g.count - expected) <= 0.15f * expected);
	VOE_TEST_CHECK_FLOAT(g.placed[0].scale.x,
			     (float)g.copies[0].scale * 0.001f, 1e-6f);
}

// Points 0-31 painted, the last at x -4 m: nothing at or past x 0.
static void nothing_grows_past_the_paint(voe_base_arena *arena)
{
	struct grown g = flat(256.0f, 2, arena);
	bool west = true;

	paint_columns(&g.land, 0, CELLS / 2, 1);
	g.land.grows[1][0] = 40;
	grow(&g, whole, 1u);
	for (uint32_t i = 0; i < g.count; i++)
		west &= g.copies[i].x <= 0;
	VOE_TEST_CHECK(g.count > 1000);
	VOE_TEST_CHECK(west);
}

// West layer 1 at 40, east layer 2 at 20 tenths of kind 0, and at 30 of kind 1.
static struct grown west_and_east(voe_base_arena *arena)
{
	struct grown g = flat(256.0f, 3, arena);

	paint_columns(&g.land, 0, CELLS / 2, 1);
	paint_columns(&g.land, CELLS / 2, CELLS + 1, 2);
	g.land.grows[1][0] = 40;
	g.land.grows[2][0] = 20;
	g.land.grows[2][1] = 30;
	return g;
}

static void half_the_tenths_grows_half(voe_base_arena *arena)
{
	struct grown g = west_and_east(arena);
	uint32_t west = 0, east = 0;

	grow(&g, whole, 1u);
	for (uint32_t i = 0; i < g.count; i++) {
		west += g.copies[i].x < 0;
		east += g.copies[i].x >= 0;
	}
	VOE_TEST_CHECK(west > 1000);
	VOE_TEST_CHECK(fabsf((float)east / (float)west - 0.5f) <= 0.15f);
}

// Points 8-39 by 8-29 given 100 of layer 1 and 155 of layer 2.
static const voe_3d_landscape_rect repainted = { 8, 8, 40, 30 };

static void repaint(voe_assets_landscape *land)
{
	for (uint32_t r = repainted.z0; r < repainted.z1; r++) {
		for (uint32_t c = repainted.x0; c < repainted.x1; c++) {
			uint8_t *at = &land->paint[(r * (CELLS + 1) + c) * 4];

			at[0] = 0;
			at[1] = 100;
			at[2] = 155;
		}
	}
}

static void a_remade_rect_equals_a_whole_grow(voe_base_arena *arena)
{
	struct grown g = west_and_east(arena);
	struct grown fresh = west_and_east(arena);
	bool placed = true;

	grow(&g, whole, 3u);
	repaint(&g.land);
	grow(&g, repainted, 3u);
	repaint(&fresh.land);
	grow(&fresh, whole, 3u);
	VOE_TEST_CHECK_INT(g.count, fresh.count);
	VOE_TEST_CHECK(equal_sorted(&g, &fresh, 2, arena));
	for (uint32_t i = 0; i < g.count; i++) {
		const voe_scene_transform t =
			voe_3d_foliage_place(&g.land, g.copies[i]);

		placed &= t.position.x == g.placed[i].position.x &&
			  t.position.z == g.placed[i].position.z &&
			  t.scale.x == g.placed[i].scale.x;
	}
	VOE_TEST_CHECK(placed);
}

static void remaking_one_kind_leaves_the_other(voe_base_arena *arena)
{
	struct grown g = west_and_east(arena);
	struct grown before = west_and_east(arena);

	grow(&g, whole, 3u);
	grow(&before, whole, 3u);
	repaint(&g.land);
	grow(&g, whole, 1u << 1);
	VOE_TEST_CHECK(equal_sorted(&g, &before, 0, arena));
	VOE_TEST_CHECK(!equal_sorted(&g, &before, 1, arena));
}

// Spacing √2 m: about 131000 points a kind, two kinds.
static void the_count_stops_at_the_cap(voe_base_arena *arena)
{
	struct grown g = flat(512.0f, 2, arena);

	for (uint32_t l = 0; l < 2; l++)
		g.land.grows[l][0] = g.land.grows[l][1] = 500;
	grow(&g, whole, 3u);
	VOE_TEST_CHECK_INT(g.count, VOE_3D_FOLIAGE_GROWN);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	a_layer_grows_its_density(arena);
	voe_base_arena_clear(arena);
	nothing_grows_past_the_paint(arena);
	voe_base_arena_clear(arena);
	half_the_tenths_grows_half(arena);
	voe_base_arena_clear(arena);
	a_remade_rect_equals_a_whole_grow(arena);
	voe_base_arena_clear(arena);
	remaking_one_kind_leaves_the_other(arena);
	voe_base_arena_clear(arena);
	the_count_stops_at_the_cap(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
