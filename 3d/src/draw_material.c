// A shape's material path looked up in the model store. See draw_material.h.
//
// The empty path is refused before the find, because the store answers "" with
// the soft dot (3d/models.h), which is no material.
#include "draw_material.h"

#include <3d/model_component.h>
#include <base/assert.h>

#include <string.h>

const voe_3d_material *voe_3d_draw_material_named(const voe_3d_models *models,
						  const char *path)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(path != NULL, "naming a material with no path");
	if (models == NULL || path[0] == '\0' ||
	    memchr(path, '\0', VOE_3D_MODEL_PATH) == NULL)
		return NULL;
	entry = voe_3d_models_find(models, path);
	if (entry == NULL || !entry->loaded || !entry->material)
		return NULL;
	return &entry->parts[0].material;
}
