// The model store's foliage on a landscape. A forest of one kind and two
// copies: each placed on the ground, a raise under copy 0 lifting it alone and
// a put of the old heights setting it back; a scatter adding placed copies and
// marking the entry edited; an erase put back by undo's put; a kind set adding
// a second at size 0; a rename following a kind. A kind's base size at 2×
// doubling its copies' scales and back at 0 restoring them; a file with
// size0=2000 loading its copies at twice scale. Two layers, rock growing kind
// 0: grown copies only on rock; a rock stamp adding some and its undo's paint
// put taking them back; rock's growth to 0 and back; an erase leaving them; a
// shadowless look relighting.
//
// The store first holds a failed `Land/bad.landscape`, as models_landscape.c
// leaves it after its renames, for the cases that ask a failed entry.
//
// IT SKIPS WHEN THERE IS NO GRAPHICS CARD, because a load uploads.
#include "models_landscape_store.inc"

#define FOREST "Woods/Forest.landscape"
#define PINE "Trees/pine.glb"
#define MOVED_PINE "Forest/pine.glb"
#define FIR "Woods/fir.glb"
#define SIZED_FOREST "Woods/Sized.landscape"
#define GROWN "Woods/Grown.landscape"

// The forest's text: 8 cells, kind 0 PINE at base size `size0`, size ±20 %,
// turn ±172° and tilt 0 %, copy 0 at the
// centre and copy 1 at (6, 6) m, loaded at `path` through _load as a file is.
static const voe_3d_model_entry *load_forest(voe_3d_models *models,
					     voe_render_device *device,
					     voe_base_arena *arena,
					     const char *path, uint16_t size0)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 8, arena);
	voe_assets_landscape_copy copies[2] = {
		{ .x = 0, .z = 0, .scale = 1000 },
		{ .x = 6000, .z = 6000, .turn = 16384, .scale = 1500 },
	};
	voe_base_error error = VOE_BASE_OK;
	voe_assets_landscape_text text;
	const voe_3d_model_entry *entry;

	snprintf(land.kinds[0], sizeof(land.kinds[0]), "%s", PINE);
	land.kind_count = 1;
	land.looks[0] = voe_assets_landscape_look_default();
	land.looks[0].size = size0;
	land.looks[0].turn_vary = 172;
	land.copies = copies;
	land.copy_count = 2;
	text = voe_assets_landscape_write(&land, arena);
	VOE_TEST_CHECK(voe_3d_models_load(models, device, path, 1,
					  (const uint8_t *)text.text, text.size,
					  &error));
	entry = voe_3d_models_find(models, path);
	if (entry == NULL || !entry->loaded || entry->landscape == NULL) {
		VOE_TEST_CHECK(false);
		return NULL;
	}
	VOE_TEST_CHECK_INT(entry->landscape->kind_count, 1);
	VOE_TEST_CHECK_INT(entry->landscape->copy_count, 2);
	VOE_TEST_CHECK(entry->landscape->copies != copies);
	return entry;
}

// Copy `i`'s placed transform standing on the heights at its x and z.
static void on_the_ground(const voe_3d_models *models,
			  const voe_3d_model_entry *entry, uint32_t i)
{
	const voe_scene_transform *placed =
		voe_3d_models_landscape_placed(models, entry);
	const voe_assets_landscape_copy copy = entry->landscape->copies[i];
	const float x = (float)copy.x * 0.001f, z = (float)copy.z * 0.001f;

	VOE_TEST_CHECK(placed != NULL);
	if (placed == NULL)
		return;
	VOE_TEST_CHECK_FLOAT((float)placed[i].position.x, x, 1e-6f);
	VOE_TEST_CHECK_FLOAT((float)placed[i].position.z, z, 1e-6f);
	VOE_TEST_CHECK_FLOAT((float)placed[i].position.y,
			     voe_3d_landscape_height(entry->landscape, x, z),
			     1e-6f);
}

static void a_sculpt_keeps_copies_standing(voe_3d_models *models,
					   voe_render_device *device,
					   voe_base_arena *arena)
{
	const voe_3d_brush raise = { .kind = VOE_3D_BRUSH_RAISE,
				     .radius = 3.0f,
				     .strength = 1.0f,
				     .softness = 0.5f };
	const voe_3d_model_entry *entry =
		load_forest(models, device, arena, FOREST, 0);
	const voe_scene_transform *placed;
	float old[81], values[81];
	voe_3d_landscape_rect rect;
	uint32_t k = 0;

	if (entry == NULL)
		return;
	placed = voe_3d_models_landscape_placed(models, entry);
	on_the_ground(models, entry, 0);
	on_the_ground(models, entry, 1);
	VOE_TEST_CHECK_FLOAT((float)placed[0].position.y, 0.0f, 0.0f);
	memcpy(old, entry->landscape->heights, sizeof(old));
	rect = voe_3d_models_landscape_brush(models, FOREST, &raise, 0.0f, 0.0f,
					     0.1f, arena);
	VOE_TEST_CHECK(!is_empty(rect));
	VOE_TEST_CHECK(placed[0].position.y > 0.0);
	on_the_ground(models, entry, 0);
	VOE_TEST_CHECK_FLOAT((float)placed[1].position.y, 0.0f, 0.0f);
	for (uint32_t r = rect.z0; r < rect.z1; r++)
		for (uint32_t c = rect.x0; c < rect.x1; c++)
			values[k++] = old[r * 9 + c];
	voe_3d_models_landscape_put(models, FOREST, rect, values);
	VOE_TEST_CHECK_FLOAT((float)placed[0].position.y, 0.0f, 0.0f);
	VOE_TEST_CHECK(one_frame(models, device));
}

// `b` where `a` stands, turned as `a` is, at `factor` times its scale.
static void scaled_alike(voe_scene_transform a, voe_scene_transform b,
			 float factor)
{
	VOE_TEST_CHECK_FLOAT(b.scale.x, factor * a.scale.x, 0.0f);
	VOE_TEST_CHECK_FLOAT(b.scale.y, factor * a.scale.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(b.scale.z, factor * a.scale.z, 0.0f);
	VOE_TEST_CHECK(b.position.x == a.position.x &&
		       b.position.y == a.position.y &&
		       b.position.z == a.position.z);
	VOE_TEST_CHECK(b.rotation.x == a.rotation.x &&
		       b.rotation.y == a.rotation.y &&
		       b.rotation.z == a.rotation.z &&
		       b.rotation.w == a.rotation.w);
}

// Kind 0's look of `entry` at base size `size`, the rest kept.
static voe_assets_landscape_look sized(const voe_3d_model_entry *entry,
				       uint16_t size)
{
	voe_assets_landscape_look look = entry->landscape->looks[0];

	look.size = size;
	return look;
}

// Kind 0 at 2× doubles both copies' scales and marks the forest edited; the
// same look again changes nothing; back to 0 gives the first transforms.
static void a_kind_size_places_its_copies_again(voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, FOREST);
	const voe_scene_transform *placed;
	voe_scene_transform first[2];
	uint32_t changes;

	if (entry == NULL || entry->landscape == NULL)
		return;
	placed = voe_3d_models_landscape_placed(models, entry);
	VOE_TEST_CHECK(placed != NULL && entry->landscape->copy_count == 2);
	if (placed == NULL || entry->landscape->copy_count != 2)
		return;
	memcpy(first, placed, sizeof(first));
	voe_3d_models_landscape_look_set(models, "Woods/none.landscape", 0,
					 sized(entry, 2000));
	voe_3d_models_landscape_saved(models, FOREST);
	changes = models->changes;
	voe_3d_models_landscape_look_set(models, FOREST, 0, sized(entry, 2000));
	VOE_TEST_CHECK_INT(entry->landscape->looks[0].size, 2000);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK(models->changes > changes);
	scaled_alike(first[0], placed[0], 2.0f);
	scaled_alike(first[1], placed[1], 2.0f);

	voe_3d_models_landscape_saved(models, FOREST);
	changes = models->changes;
	voe_3d_models_landscape_look_set(models, FOREST, 0, sized(entry, 2000));
	VOE_TEST_CHECK(!entry->edited);
	VOE_TEST_CHECK_INT(models->changes, changes);

	voe_3d_models_landscape_look_set(models, FOREST, 0, sized(entry, 0));
	VOE_TEST_CHECK(entry->edited);
	scaled_alike(first[0], placed[0], 1.0f);
	scaled_alike(first[1], placed[1], 1.0f);
}

// The same forest written with size0=2000 loads its copies at twice their
// own scales.
static void a_sized_file_loads_its_copies_scaled(voe_3d_models *models,
						 voe_render_device *device,
						 voe_base_arena *arena)
{
	const voe_3d_model_entry *entry =
		load_forest(models, device, arena, SIZED_FOREST, 2000);
	const voe_scene_transform *placed;

	if (entry == NULL)
		return;
	placed = voe_3d_models_landscape_placed(models, entry);
	VOE_TEST_CHECK(placed != NULL);
	if (placed == NULL)
		return;
	VOE_TEST_CHECK_INT(entry->landscape->looks[0].size, 2000);
	VOE_TEST_CHECK_FLOAT(placed[0].scale.x, 2.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(placed[1].scale.x, 3.0f, 1e-6f);
	on_the_ground(models, entry, 1);
}

static void scatter_erase_and_put_back(voe_3d_models *models)
{
	voe_3d_foliage_brush brush = { .radius = 3.0f,
				       .density = 50.0f,
				       .picked = 1u };
	const voe_3d_model_entry *entry = voe_3d_models_find(models, FOREST);
	voe_assets_landscape_copy added[VOE_3D_FOLIAGE_TRIES];
	voe_assets_landscape_copy removed[64];
	uint64_t seed = 0x9e3779b97f4a7c15u;
	uint32_t count, erased;

	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK_INT(voe_3d_models_landscape_scatter(
				   models, "Woods/none.landscape", &brush, 0.0f,
				   0.0f, 0.1f, &seed, added),
			   0);
	voe_3d_models_landscape_saved(models, FOREST);
	count = voe_3d_models_landscape_scatter(models, FOREST, &brush, 0.0f,
						0.0f, 0.1f, &seed, added);
	VOE_TEST_CHECK(count > 0);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK_INT(entry->landscape->copy_count, 2 + count);
	on_the_ground(models, entry, entry->landscape->copy_count - 1);

	voe_3d_models_landscape_saved(models, FOREST);
	erased = voe_3d_models_landscape_erase(models, FOREST, &brush, 0.0f,
					       0.0f, removed, 64);
	VOE_TEST_CHECK(erased > count);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK_INT(entry->landscape->copy_count, 2 + count - erased);
	voe_3d_models_landscape_put_copies(models, FOREST, removed, erased,
					   true);
	VOE_TEST_CHECK_INT(entry->landscape->copy_count, 2 + count);
	on_the_ground(models, entry, entry->landscape->copy_count - 1);
}

static void kinds_are_set_and_renamed(voe_3d_models *models)
{
	const voe_3d_model_entry *entry = voe_3d_models_find(models, FOREST);

	if (entry == NULL || entry->landscape == NULL)
		return;
	VOE_TEST_CHECK(voe_3d_models_landscape_placed(
			       models, voe_3d_models_find(
					       models, "Land/bad.landscape")) ==
		       NULL);
	voe_3d_models_landscape_saved(models, FOREST);
	voe_3d_models_landscape_kind_set(models, FOREST, 1, FIR);
	VOE_TEST_CHECK_INT(entry->landscape->kind_count, 2);
	VOE_TEST_CHECK(strcmp(entry->landscape->kinds[1], FIR) == 0);
	VOE_TEST_CHECK_INT(entry->landscape->looks[1].size, 0);
	VOE_TEST_CHECK_INT(entry->landscape->looks[1].turn_vary,
			   VOE_ASSETS_LANDSCAPE_TURN_VARY_DEFAULT);
	VOE_TEST_CHECK_INT(entry->landscape->grows[0][1], 0);
	VOE_TEST_CHECK(entry->edited);
	voe_3d_models_rename(models, "Trees", "Forest");
	VOE_TEST_CHECK(strcmp(entry->landscape->kinds[0], MOVED_PINE) == 0);
	VOE_TEST_CHECK(strcmp(entry->landscape->kinds[1], FIR) == 0);
}

// 64 m of 8 cells, a cell 8 m: layers GRASS and ROCK, layer 1 growing kind 0
// PINE at 40 tenths, the points of columns 0 to 3 all layer 1, so rock fades
// out between x = −8 m and 0. Loaded with no file copies.
static const voe_3d_model_entry *load_grown(voe_3d_models *models,
					    voe_render_device *device,
					    voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 8, arena);
	uint8_t *paint = voe_base_arena_push(arena, 81 * 8);
	voe_base_error error = VOE_BASE_OK;
	const voe_3d_model_entry *entry;

	memset(paint, 0, 81 * 8);
	for (uint32_t i = 0; i < 81; i++)
		paint[i * 4 + (i % 9 < 4 ? 1 : 0)] = 255;
	land.paint = paint;
	snprintf(land.layers[0], sizeof(land.layers[0]), "%s", GRASS);
	snprintf(land.layers[1], sizeof(land.layers[1]), "%s", ROCK);
	land.layer_count = 2;
	snprintf(land.kinds[0], sizeof(land.kinds[0]), "%s", PINE);
	land.kind_count = 1;
	land.looks[0] = voe_assets_landscape_look_default();
	land.grows[1][0] = 40;
	VOE_TEST_CHECK(voe_3d_models_load_landscape(models, device, GROWN, 1,
						    &land, &error));
	entry = voe_3d_models_find(models, GROWN);
	if (entry == NULL || !entry->loaded || entry->landscape == NULL) {
		VOE_TEST_CHECK(false);
		return NULL;
	}
	return entry;
}

static uint32_t grown_count(const voe_3d_models *models,
			    const voe_3d_model_entry *entry)
{
	return voe_3d_models_landscape_grown(models, entry).count;
}

// The grown copies stand only where rock is painted, each placed at its x.
static uint32_t grown_only_on_rock(voe_3d_models *models,
				   const voe_3d_model_entry *entry)
{
	const voe_3d_models_grown grown =
		voe_3d_models_landscape_grown(models, entry);

	VOE_TEST_CHECK(grown.count > 0 && grown.placed != NULL);
	VOE_TEST_CHECK(entry->landscape->copy_count == 0);
	for (uint32_t i = 0; i < grown.count; i++) {
		VOE_TEST_CHECK(grown.copies[i].x < 0);
		VOE_TEST_CHECK_INT(grown.copies[i].kind, 0);
		VOE_TEST_CHECK_FLOAT((float)grown.placed[i].position.x,
				     (float)grown.copies[i].x * 0.001f, 1e-4f);
	}
	return grown.count;
}

// A stamp of rock east of the fade adds grown copies; the old weights put
// back over its rect give back the old count.
static void a_paint_stamp_grows_and_its_undo_takes_back(
	voe_3d_models *models, const voe_3d_model_entry *entry, uint32_t first,
	voe_base_arena *arena)
{
	const voe_3d_brush paint = { .kind = VOE_3D_BRUSH_PAINT,
				     .radius = 12.0f,
				     .strength = 1.0f,
				     .softness = 0.5f,
				     .layer = 1 };
	uint8_t old[81 * 8], values[81 * 8];
	const uint32_t changes = models->changes;
	voe_3d_landscape_rect rect;
	size_t k = 0;

	memcpy(old, entry->landscape->paint, sizeof(old));
	rect = voe_3d_models_landscape_brush(models, GROWN, &paint, 16.0f, 0.0f,
					     0.2f, arena);
	VOE_TEST_CHECK(!is_empty(rect));
	VOE_TEST_CHECK(grown_count(models, entry) > first);
	VOE_TEST_CHECK(models->changes > changes);
	for (uint32_t p = 0; p < 2; p++)
		for (uint32_t r = rect.z0; r < rect.z1; r++)
			for (uint32_t c = rect.x0; c < rect.x1; c++)
				for (uint32_t b = 0; b < 4; b++)
					values[k++] =
						old[p * 81 * 4 + (r * 9 + c) * 4 + b];
	voe_3d_models_landscape_put_paint(models, GROWN, rect, values);
	VOE_TEST_CHECK_INT(grown_count(models, entry), first);
}

// Rock growing none leaves none, back at 40 the first count; an erase of
// kind 0 leaves them; a look changing only `shadowless` relights and keeps
// them.
static void the_material_owns_its_growth(voe_3d_models *models,
					 const voe_3d_model_entry *entry,
					 uint32_t first)
{
	const voe_3d_foliage_brush erase = { .radius = 40.0f,
					     .density = 4.0f,
					     .picked = 1u };
	voe_assets_landscape_copy removed[4];
	voe_assets_landscape_look look = entry->landscape->looks[0];
	uint32_t changes;

	voe_3d_models_landscape_saved(models, GROWN);
	voe_3d_models_landscape_grow_set(models, GROWN, 1, 0, 0);
	VOE_TEST_CHECK_INT(grown_count(models, entry), 0);
	VOE_TEST_CHECK_INT(entry->landscape->grows[1][0], 0);
	VOE_TEST_CHECK(entry->edited);
	voe_3d_models_landscape_grow_set(models, GROWN, 1, 0, 40);
	VOE_TEST_CHECK_INT(grown_count(models, entry), first);

	VOE_TEST_CHECK_INT(voe_3d_models_landscape_erase(models, GROWN, &erase,
							 0.0f, 0.0f, removed, 4),
			   0);
	VOE_TEST_CHECK_INT(grown_count(models, entry), first);

	voe_3d_models_landscape_saved(models, GROWN);
	changes = models->changes;
	look.shadowless = true;
	voe_3d_models_landscape_look_set(models, GROWN, 0, look);
	VOE_TEST_CHECK(entry->landscape->looks[0].shadowless);
	VOE_TEST_CHECK(models->changes > changes);
	VOE_TEST_CHECK(entry->edited);
	VOE_TEST_CHECK_INT(grown_count(models, entry), first);
	changes = models->changes;
	voe_3d_models_landscape_look_set(models, GROWN, 0, look);
	VOE_TEST_CHECK_INT(models->changes, changes);
}

static void layers_grow_their_kinds(voe_3d_models *models,
				    voe_render_device *device,
				    voe_base_arena *arena)
{
	const voe_3d_model_entry *entry = load_grown(models, device, arena);
	const voe_3d_models_grown none = voe_3d_models_landscape_grown(
		models, voe_3d_models_find(models, "Land/bad.landscape"));
	uint32_t first;

	VOE_TEST_CHECK(none.copies == NULL && none.placed == NULL &&
		       none.count == 0);
	if (entry == NULL)
		return;
	first = grown_only_on_rock(models, entry);
	printf("rock grows %u copies\n", first);
	a_paint_stamp_grows_and_its_undo_takes_back(models, entry, first,
						    arena);
	the_material_owns_its_growth(models, entry, first);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = a_device(arena);
	voe_3d_models *models;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	models = voe_3d_models_new();
	// Text that is no landscape: the load fails, its entry kept failed.
	(void)voe_3d_models_load(models, device, "Land/bad.landscape", 1,
				 (const uint8_t *)"nope", 4, &error);

	a_sculpt_keeps_copies_standing(models, device, arena);
	a_kind_size_places_its_copies_again(models);
	scatter_erase_and_put_back(models);
	kinds_are_set_and_renamed(models);
	a_sized_file_loads_its_copies_scaled(models, device, arena);
	layers_grow_their_kinds(models, device, arena);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
