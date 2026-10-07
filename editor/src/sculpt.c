// The brush's start, its put-down, whether a thing wears a landscape, and the
// held drag that sculpts it: hover, press, stamps and the release's undo step.
// See the header for why the brush is never saved or undone.
#include "sculpt.h"

#include "inspector.h"
#include "models.h"
#include "scene.h"
#include "session.h"
#include "strokes.h"
#include "undo.h"

#include <base/assert.h>

#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/pick.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
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

// The loaded grid `path` names in the store, NULL for none.
static const voe_assets_landscape *grid(const voe_editor_models *models,
					const char *path)
{
	const voe_3d_model_entry *entry =
		voe_3d_models_find(voe_editor_models_store(models), path);

	VOE_BASE_ASSERT(path != NULL, "finding a grid by no path");
	if (entry == NULL || !entry->loaded)
		return NULL;
	return entry->landscape;
}

// The selection's model path when it wears a loaded landscape, else NULL; a
// path that fills its room with no terminator is none.
static const char *worn(const voe_editor_scene *scene,
			const voe_editor_models *models)
{
	const char *path;

	VOE_BASE_ASSERT(scene != NULL && models != NULL,
			"asking after a worn grid with no scene or store");
	if (!voe_editor_sculpt_wears(scene->world, scene->selected))
		return NULL;
	path = voe_3d_model_get(scene->world, scene->selected)->path;
	if (memchr(path, '\0', VOE_3D_MODEL_PATH) == NULL)
		return NULL;
	return grid(models, path) != NULL ? path : NULL;
}

// The ground under the pointer, through the pick ray of the view it is over
// as pick.c makes it; `hit` false over no view or off the ground.
static void hover(voe_editor_sculpt *sculpt, const voe_editor_scene *scene,
		  const voe_editor_views *views,
		  const voe_editor_models *models, voe_math_float2 pointer)
{
	voe_3d_landscape_hit hit;
	uint32_t view;
	voe_math_float2 point;

	sculpt->hit = false;
	if (!voe_editor_views_under(views, pointer, &view, &point))
		return;
	const voe_editor_view *under = &views->views[view];
	voe_3d_ray ray = voe_3d_pick_ray(
		voe_editor_view_pass_camera(under, (voe_render_light){ 0 }).view,
		under->eye,
		(voe_platform_size){ (int)under->width, (int)under->height },
		point);
	if (!voe_3d_pick_landscape(scene->world,
				   voe_editor_models_store(models),
				   scene->selected, ray, &hit))
		return;
	sculpt->hit = true;
	sculpt->x = hit.x;
	sculpt->z = hit.z;
	sculpt->entity = scene->selected;
	VOE_BASE_ASSERT(sculpt->hit, "a hover found and lost");
}

static bool rect_empty(voe_3d_landscape_rect rect)
{
	return rect.x0 >= rect.x1 || rect.z0 >= rect.z1;
}

// The smallest rect holding both; an empty one adds nothing.
static voe_3d_landscape_rect join(voe_3d_landscape_rect a,
				  voe_3d_landscape_rect b)
{
	if (rect_empty(a))
		return b;
	if (rect_empty(b))
		return a;
	return (voe_3d_landscape_rect){ .x0 = a.x0 < b.x0 ? a.x0 : b.x0,
					.z0 = a.z0 < b.z0 ? a.z0 : b.z0,
					.x1 = a.x1 > b.x1 ? a.x1 : b.x1,
					.z1 = a.z1 > b.z1 ? a.z1 : b.z1 };
}

// The stretch from the last hit to this one cut into stamps at most radius/4
// apart, the frame's seconds shared among them, their rects joined.
static void stamp(voe_editor_sculpt *sculpt, voe_editor_models *models,
		  float seconds, voe_base_arena *scratch)
{
	const float dx = sculpt->x - sculpt->last_x;
	const float dz = sculpt->z - sculpt->last_z;
	const float spacing = sculpt->radius / 4.0f;
	const float length = sqrtf(dx * dx + dz * dz);
	const voe_3d_brush brush = { .kind = sculpt->kind,
				     .radius = sculpt->radius,
				     .strength = sculpt->strength,
				     .softness = sculpt->softness,
				     .target = sculpt->target };
	uint32_t count = 1;

	VOE_BASE_ASSERT(spacing > 0.0f, "a brush with no radius");
	if (length > spacing)
		count = length / spacing >= (float)VOE_EDITOR_SCULPT_STAMPS ?
				VOE_EDITOR_SCULPT_STAMPS :
				(uint32_t)ceilf(length / spacing);
	for (uint32_t i = 1; i <= count; i++) {
		const float t = (float)i / (float)count;

		sculpt->touched = join(
			sculpt->touched,
			voe_editor_models_brush(models, sculpt->path, &brush,
						sculpt->last_x + dx * t,
						sculpt->last_z + dz * t,
						seconds / (float)count,
						scratch));
	}
	sculpt->last_x = sculpt->x;
	sculpt->last_z = sculpt->z;
	VOE_BASE_ASSERT(count <= VOE_EDITOR_SCULPT_STAMPS, "stamps past the cap");
}

// The press: the path and the whole grid copied, flatten's height taken.
static void begin(voe_editor_sculpt *sculpt, const voe_editor_models *models,
		  const char *path)
{
	const voe_assets_landscape *land = grid(models, path);
	size_t count;

	VOE_BASE_ASSERT(land != NULL && sculpt->hit,
			"a stroke begun off a loaded ground");
	VOE_BASE_ASSERT(strlen(path) < VOE_3D_MODEL_PATH,
			"a stroke's path past its room");
	memcpy(sculpt->path, path, strlen(path) + 1);
	sculpt->cells = land->cells;
	count = (size_t)(land->cells + 1) * (land->cells + 1);
	sculpt->before = malloc(count * sizeof(*sculpt->before));
	VOE_BASE_ASSERT(sculpt->before != NULL,
			"out of memory copying a grid at a press");
	memcpy(sculpt->before, land->heights, count * sizeof(*sculpt->before));
	sculpt->target = voe_3d_landscape_height(land, sculpt->x, sculpt->z);
	sculpt->last_x = sculpt->x;
	sculpt->last_z = sculpt->z;
	sculpt->touched = (voe_3d_landscape_rect){ 0 };
	sculpt->stroking = true;
}

// The release: the touched rect's heights from the copy and the store made a
// stroke and handed to the undo line, the project marked edited. An empty
// rect, or a grid gone or resized under the stroke, records nothing.
static void end(voe_editor_sculpt *sculpt, const voe_editor_models *models,
		struct voe_editor_undo *undo,
		struct voe_editor_session *session, voe_base_arena *scratch)
{
	const voe_assets_landscape *land = grid(models, sculpt->path);
	const voe_3d_landscape_rect rect = sculpt->touched;

	VOE_BASE_ASSERT(sculpt->stroking && sculpt->before != NULL,
			"ending no stroke");
	if (!rect_empty(rect) && land != NULL && land->cells == sculpt->cells) {
		const uint32_t width = rect.x1 - rect.x0;
		const uint32_t side = sculpt->cells + 1;
		const size_t count = (size_t)width * (rect.z1 - rect.z0);
		float *heights = malloc(2 * count * sizeof(*heights));

		VOE_BASE_ASSERT(heights != NULL,
				"out of memory cutting a stroke's heights");
		for (uint32_t z = rect.z0; z < rect.z1; z++) {
			const size_t from = (size_t)z * side + rect.x0;
			const size_t to = (size_t)(z - rect.z0) * width;

			memcpy(heights + to, sculpt->before + from,
			       width * sizeof(*heights));
			memcpy(heights + count + to, land->heights + from,
			       width * sizeof(*heights));
		}
		voe_editor_undo_stroke(undo, session->project, scratch,
				       voe_editor_stroke_new(sculpt->path, rect,
							     heights,
							     heights + count));
		free(heights);
		voe_editor_session_edited(session);
	}
	free(sculpt->before);
	sculpt->before = NULL;
	sculpt->stroking = false;
	VOE_BASE_ASSERT(sculpt->before == NULL, "a stroke's copy kept");
}

bool voe_editor_sculpt_read(voe_editor_sculpt *sculpt,
			    struct voe_editor_scene *scene,
			    const voe_editor_views *views,
			    struct voe_editor_models *models,
			    struct voe_editor_undo *undo,
			    struct voe_editor_session *session,
			    voe_math_float2 pointer, bool left, bool over,
			    float seconds, voe_base_arena *scratch)
{
	const bool edge = left && !sculpt->left_was;
	const char *path;

	VOE_BASE_ASSERT(sculpt != NULL && scene != NULL && views != NULL,
			"sculpting with no brush, scene or views");
	VOE_BASE_ASSERT(models != NULL && undo != NULL && session != NULL &&
				scratch != NULL,
			"sculpting with no store, undo, session or scratch");
	sculpt->left_was = left;
	path = sculpt->chosen ? worn(scene, models) : NULL;
	if (path != NULL && sculpt->stroking &&
	    (scene->selected.index != sculpt->entity.index ||
	     scene->selected.generation != sculpt->entity.generation))
		path = NULL;
	if (path == NULL) {
		sculpt->hit = false;
		if (sculpt->stroking)
			end(sculpt, models, undo, session, scratch);
		return false;
	}
	if (over)
		hover(sculpt, scene, views, models, pointer);
	else
		sculpt->hit = false;

	if (!sculpt->stroking) {
		if (!edge || !sculpt->hit)
			return false;
		begin(sculpt, models, path);
		stamp(sculpt, models, seconds, scratch);
		return true;
	}
	if (!left) {
		end(sculpt, models, undo, session, scratch);
		return false;
	}
	if (sculpt->hit)
		stamp(sculpt, models, seconds, scratch);
	VOE_BASE_ASSERT(sculpt->stroking, "a held stroke lost");
	return false;
}
