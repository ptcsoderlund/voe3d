// The editor's one model store models.h describes: made empty, cleared
// through the device and destroyed, and handed out read-only.
#include "models.h"

#include <base/assert.h>

#include <stdlib.h>

struct voe_editor_models {
	voe_3d_models *store;
};

voe_editor_models *voe_editor_models_new(void)
{
	voe_editor_models *models = calloc(1, sizeof(*models));

	VOE_BASE_ASSERT(models != NULL, "out of memory making the model store");
	models->store = voe_3d_models_new();
	VOE_BASE_ASSERT(models->store != NULL, "made a model store with none");
	return models;
}

void voe_editor_models_destroy(voe_editor_models *models,
			       voe_render_device *device)
{
	if (models == NULL)
		return;
	VOE_BASE_ASSERT(device != NULL, "clearing models with no device");
	voe_3d_models_clear(models->store, device);
	voe_3d_models_destroy(models->store);
	free(models);
}

const voe_3d_models *voe_editor_models_store(const voe_editor_models *models)
{
	VOE_BASE_ASSERT(models != NULL, "reading no model store");
	VOE_BASE_ASSERT(models->store != NULL, "a model store with none");
	return models->store;
}
