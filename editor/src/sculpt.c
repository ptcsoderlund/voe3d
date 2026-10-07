// The brush's start, its put-down, and whether a thing wears a landscape. See
// the header for why the brush is never saved or undone.
#include "sculpt.h"

#include "inspector.h"

#include <base/assert.h>

#include <3d/model_component.h>

#include <ctype.h>
#include <string.h>

// The ending a landscape's path has, lower case, its dot included.
#define LANDSCAPE_SUFFIX ".landscape"

void voe_editor_sculpt_start(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "starting no brush");

	*sculpt = (voe_editor_sculpt){ .radius = VOE_EDITOR_SCULPT_RADIUS,
				       .strength = VOE_EDITOR_SCULPT_STRENGTH,
				       .softness = VOE_EDITOR_SCULPT_SOFTNESS };
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KINDS; i++)
		sculpt->buttons[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_SLIDERS; i++)
		sculpt->sliders[i] = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(!sculpt->chosen, "a started brush already chosen");
}

void voe_editor_sculpt_choose_none(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "putting down no brush");

	sculpt->chosen = false;
}

bool voe_editor_sculpt_wears(const voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_3d_model *model;
	const char *end;
	size_t length;
	size_t suffix = strlen(LANDSCAPE_SUFFIX);

	VOE_BASE_ASSERT(world != NULL, "asking after a landscape in no world");

	if (!voe_ecs_entity_alive(world, entity) ||
	    voe_editor_inspector_is_part(world, entity, NULL))
		return false;
	model = voe_3d_model_get(world, entity);
	if (model == NULL)
		return false;
	end = memchr(model->path, '\0', VOE_3D_MODEL_PATH);
	length = end != NULL ? (size_t)(end - model->path) : VOE_3D_MODEL_PATH;
	if (length < suffix)
		return false;
	for (size_t i = 0; i < suffix; i++)
		if (tolower((unsigned char)model->path[length - suffix + i]) !=
		    LANDSCAPE_SUFFIX[i])
			return false;

	VOE_BASE_ASSERT(length <= VOE_3D_MODEL_PATH, "a path past its room");
	return true;
}
