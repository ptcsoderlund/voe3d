// The model store's foliage calls (3d/models_foliage.h): each finds a loaded
// landscape's entry, hands its grid and its held placed transforms to
// 3d/foliage.h, and marks the entry edited when a copy, a group or glb, a look
// or a growth changed. A scatter or erase that changed a copy, every put of
// copies, every glb set and every changed look or growth raise the store's
// `changes`, so the bounce relights (0451 point 3). A copy's glb is picked
// live (0521 point 3), so a glb added or a new size or share places that
// group's copies again, and those, a new spread or a growth remake its grown
// copies over the grid through models_landscape.c's regrow.
//
// Used by the editor: the scatter and erase brushes while a stroke is held,
// the put for its undo step, the glb set for a dropped model, the look and
// growth sets for a group's settings; and by what draws the copies, through
// _placed and _grown.
//
// Constraints: a path is found by models_landscape.c's walk over the entries;
// the placed transforms and grown copies are models_landscape.c's, made at
// the load. A look, glb or growth set remakes the whole grid for its group.
#include <3d/models_foliage.h>

#include "models_store.h"

#include <3d/foliage.h>
#include <assets/landscape.h>
#include <base/assert.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

uint32_t voe_3d_models_landscape_scatter(voe_3d_models *models,
					 const char *path,
					 const voe_3d_foliage_brush *brush,
					 float x, float z, float seconds,
					 uint64_t *seed,
					 voe_assets_landscape_copy *added)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"scattering needs a store and a path");
	VOE_BASE_ASSERT(brush != NULL && seed != NULL && added != NULL,
			"scattering needs a brush, a seed and room");
	const uint32_t index = voe_3d_models_landscape_index(models, path);
	uint32_t count;

	if (index == VOE_3D_MODELS)
		return 0;
	count = voe_3d_foliage_scatter(models->entries[index].landscape,
				       models->held[index].placed, brush, x, z,
				       seconds, seed, added);
	if (count > 0) {
		models->entries[index].edited = true;
		models->changes++;
	}
	return count;
}

uint32_t voe_3d_models_landscape_erase(voe_3d_models *models, const char *path,
				       const voe_3d_foliage_brush *brush,
				       float x, float z,
				       voe_assets_landscape_copy *removed,
				       uint32_t room)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"erasing needs a store and a path");
	VOE_BASE_ASSERT(brush != NULL && (removed != NULL || room == 0),
			"erasing needs a brush and room");
	const uint32_t index = voe_3d_models_landscape_index(models, path);
	uint32_t count;

	if (index == VOE_3D_MODELS)
		return 0;
	count = voe_3d_foliage_erase(models->entries[index].landscape,
				     models->held[index].placed, brush, x, z,
				     removed, room);
	if (count > 0) {
		models->entries[index].edited = true;
		models->changes++;
	}
	return count;
}

// Edited when the count moved: an add adds and a removal removes, or neither.
// The store's changes rise either way.
void voe_3d_models_landscape_put_copies(voe_3d_models *models,
					const char *path,
					const voe_assets_landscape_copy *copies,
					uint32_t count, bool add)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"putting copies needs a store and a path");
	VOE_BASE_ASSERT(copies != NULL || count == 0, "the copies put");
	const uint32_t index = voe_3d_models_landscape_index(models, path);

	if (index == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[index].landscape;
	const uint32_t before = land->copy_count;

	voe_3d_foliage_put(land, models->held[index].placed, copies, count,
			   add);
	models->changes++;
	if (land->copy_count != before)
		models->entries[index].edited = true;
}

// Group `group` of entry `at` remade over the whole grid, the entry edited and
// the store's changes raised.
static void remade(voe_3d_models *models, uint32_t at, uint32_t group)
{
	const voe_3d_model_entry *entry = &models->entries[at];
	const uint32_t n = entry->landscape->cells + 1;

	VOE_BASE_ASSERT(group < entry->landscape->group_count, "a held group");
	(void)voe_3d_models_regrow(entry->landscape, &models->held[at],
				   entry->path,
				   (voe_3d_landscape_rect){ 0, 0, n, n },
				   1u << group);
	models->entries[at].edited = true;
	models->changes++;
}

// Each copy of group `group` of entry `at` placed again, its glb picked anew.
static void placed_again(voe_3d_models *models, uint32_t at, uint32_t group)
{
	const voe_assets_landscape *land = models->entries[at].landscape;
	voe_scene_transform *placed = models->held[at].placed;

	for (uint32_t i = 0; i < land->copy_count; i++)
		if (land->copies[i].group == group)
			placed[i] = voe_3d_foliage_place(land, land->copies[i]);
}

// A group added starts with no copies and no growth, and a path replaced keeps
// its look, so only a glb added moves which glb a copy picks.
bool voe_3d_models_landscape_glb_set(voe_3d_models *models, const char *path,
				     uint32_t group, uint32_t glb,
				     const char *model)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a glb needs a store and a path");
	VOE_BASE_ASSERT(model != NULL && model[0] != '\0' &&
				strlen(model) < VOE_ASSETS_LANDSCAPE_GLB_PATH,
			"a glb's model path that fits");
	const uint32_t at = voe_3d_models_landscape_index(models, path);

	if (at == VOE_3D_MODELS)
		return false;
	voe_assets_landscape *land = models->entries[at].landscape;

	VOE_BASE_ASSERT(group <= land->group_count &&
				(group < land->group_count ?
					 glb <= land->groups[group].glb_count :
					 glb == 0),
			"a held glb or the next of its group, or a new group");
	if (group == land->group_count) {
		if (!voe_assets_landscape_group_add(land, model))
			return false;
		for (uint32_t l = 0; l < VOE_ASSETS_LANDSCAPE_LAYERS; l++)
			land->grows[l][group] = 0;
	} else if (glb == land->groups[group].glb_count) {
		if (!voe_assets_landscape_glb_add(land, group, model))
			return false;
		placed_again(models, at, group);
		remade(models, at, group);
		return true;
	} else if (!voe_assets_landscape_glb_set(land, group, glb, model)) {
		return false;
	}
	models->entries[at].edited = true;
	models->changes++;
	return true;
}

// One walk of the copies, each of group `group` placed again, when the size
// or share moved; the grown remade when the size, a spread or the share moved.
void voe_3d_models_landscape_look_set(voe_3d_models *models, const char *path,
				      uint32_t group, uint32_t glb,
				      voe_assets_landscape_look look)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a glb's look needs a store and a path");
	VOE_BASE_ASSERT((look.size == 0 ||
			 (look.size >= VOE_ASSETS_LANDSCAPE_GLB_SIZE_MIN &&
			  look.size <= VOE_ASSETS_LANDSCAPE_GLB_SIZE_MAX)) &&
				look.size_vary <=
					VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX &&
				look.turn_vary <=
					VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX &&
				look.tilt <= VOE_ASSETS_LANDSCAPE_TILT_MAX &&
				look.share <= VOE_ASSETS_LANDSCAPE_SHARE_MAX &&
				(look.blend_height == 0 ||
				 (look.blend_height >=
					  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN &&
				  look.blend_height <=
					  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX)),
			"a look the file can hold");
	const uint32_t at = voe_3d_models_landscape_index(models, path);

	if (at == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[at].landscape;

	VOE_BASE_ASSERT(group < land->group_count &&
				glb < land->groups[group].glb_count,
			"a held glb");
	const voe_assets_landscape_look was = land->groups[group].looks[glb];
	const bool sized = was.size != look.size || was.share != look.share;
	const bool spread = was.size_vary != look.size_vary ||
			    was.turn_vary != look.turn_vary ||
			    was.tilt != look.tilt;

	if (!sized && !spread && was.shadowless == look.shadowless &&
	    was.blends == look.blends && was.blend_height == look.blend_height)
		return;
	land->groups[group].looks[glb] = look;
	if (sized)
		placed_again(models, at, group);
	if (sized || spread) {
		remade(models, at, group);
		return;
	}
	models->entries[at].edited = true;
	models->changes++;
}

void voe_3d_models_landscape_grow_set(voe_3d_models *models, const char *path,
				      uint32_t layer, uint32_t group,
				      uint16_t density)
{
	VOE_BASE_ASSERT(models != NULL && path != NULL,
			"setting a growth needs a store and a path");
	VOE_BASE_ASSERT(density <= VOE_ASSETS_LANDSCAPE_GROW_MAX,
			"a density the file can hold");
	const uint32_t at = voe_3d_models_landscape_index(models, path);

	if (at == VOE_3D_MODELS)
		return;
	voe_assets_landscape *land = models->entries[at].landscape;

	VOE_BASE_ASSERT(layer < land->layer_count && group < land->group_count,
			"a held layer and group");
	if (land->grows[layer][group] == density)
		return;
	land->grows[layer][group] = density;
	remade(models, at, group);
}

voe_3d_models_grown voe_3d_models_landscape_grown(const voe_3d_models *models,
						  const voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(models != NULL, "grown copies need a store");
	VOE_BASE_ASSERT(entry != NULL, "grown copies need an entry");

	for (uint32_t i = 0; i < models->count; i++) {
		if (&models->entries[i] != entry)
			continue;
		if (!entry->loaded || entry->landscape == NULL)
			break;
		return (voe_3d_models_grown){
			.copies = models->held[i].grown,
			.placed = models->held[i].grown_placed,
			.count = models->held[i].grown_count,
		};
	}
	return (voe_3d_models_grown){ 0 };
}

const voe_scene_transform *
voe_3d_models_landscape_placed(const voe_3d_models *models,
			       const voe_3d_model_entry *entry)
{
	VOE_BASE_ASSERT(models != NULL, "placed copies need a store");
	VOE_BASE_ASSERT(entry != NULL, "placed copies need an entry");

	for (uint32_t i = 0; i < models->count; i++) {
		if (&models->entries[i] != entry)
			continue;
		if (!entry->loaded || entry->landscape == NULL)
			return NULL;
		return models->held[i].placed;
	}
	return NULL;
}
