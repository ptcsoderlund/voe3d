// The brush's start, its put-down, whether a thing wears a landscape, and the
// held drag that sculpts, paints, scatters or erases it: hover, press with its
// heights or paint copy or changed list, stamps and the release's undo step;
// and a kind's look edit and a layer's grow edit settled each frame before
// the brush.
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

// The scatters' fixed first xorshift state, any nonzero one.
#define FOLIAGE_SEED 0x9e3779b97f4a7c15u

void voe_editor_sculpt_start(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "starting no brush");

	*sculpt = (voe_editor_sculpt){ .radius = VOE_EDITOR_SCULPT_RADIUS,
				       .strength = VOE_EDITOR_SCULPT_STRENGTH,
				       .softness = VOE_EDITOR_SCULPT_SOFTNESS,
				       .density = VOE_EDITOR_SCULPT_DENSITY,
				       .seed = FOLIAGE_SEED };
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KINDS; i++)
		sculpt->buttons[i] = VOE_UI_NODE_NONE;
	sculpt->strength_slider = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_BRUSH_SLIDERS; i++)
		sculpt->brush_sliders[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_LAYER_ROWS; i++)
		sculpt->layer_rows[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_ASSETS_LANDSCAPE_KINDS; i++)
		sculpt->kind_sliders[i] = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(!sculpt->chosen && !sculpt->looking &&
				!sculpt->look_asked && !sculpt->growing &&
				!sculpt->grow_asked,
			"a started brush already chosen or editing a look or growth");
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

// Whether `kind` places foliage copies rather than shaping the grid.
static bool foliage_kind(voe_3d_brush_kind kind)
{
	return kind == VOE_3D_BRUSH_SCATTER || kind == VOE_3D_BRUSH_ERASE;
}

// The changed list doubled until `more` copies fit past its count.
static void changed_grow(voe_editor_sculpt *sculpt, uint32_t more)
{
	const uint64_t want = (uint64_t)sculpt->changed_count + more;
	uint64_t room = sculpt->changed_room;

	VOE_BASE_ASSERT(sculpt->changed != NULL && room > 0,
			"growing no changed list");
	if (want <= room)
		return;
	for (uint32_t i = 0; i < 32 && room < want; i++)
		room *= 2;
	VOE_BASE_ASSERT(room >= want && room <= UINT32_MAX,
			"a changed list past its room");
	sculpt->changed = realloc(sculpt->changed,
				  (size_t)room * sizeof(*sculpt->changed));
	VOE_BASE_ASSERT(sculpt->changed != NULL,
			"out of memory growing a foliage stroke");
	sculpt->changed_room = (uint32_t)room;
}

// One SCATTER or ERASE stamp at (x, z): the copies it added or removed
// appended to the changed list, grown first to hold the most it can take.
static void foliage_stamp(voe_editor_sculpt *sculpt, voe_editor_models *models,
			  float x, float z, float seconds)
{
	const voe_assets_landscape *land = grid(models, sculpt->path);
	const voe_3d_foliage_brush brush = { .radius = sculpt->radius,
					     .softness = sculpt->softness,
					     .density = sculpt->density,
					     .picked = sculpt->picked };

	VOE_BASE_ASSERT(foliage_kind(sculpt->kind), "a foliage stamp of a sculpt");
	if (land == NULL)
		return;
	if (sculpt->kind == VOE_3D_BRUSH_SCATTER) {
		changed_grow(sculpt, VOE_3D_FOLIAGE_TRIES);
		sculpt->changed_count += voe_editor_models_scatter(
			models, sculpt->path, &brush, x, z, seconds,
			&sculpt->seed, sculpt->changed + sculpt->changed_count);
	} else {
		changed_grow(sculpt, land->copy_count);
		sculpt->changed_count += voe_editor_models_erase(
			models, sculpt->path, &brush, x, z,
			sculpt->changed + sculpt->changed_count,
			sculpt->changed_room - sculpt->changed_count);
	}
	VOE_BASE_ASSERT(sculpt->changed_count <= sculpt->changed_room,
			"a foliage stamp past the changed list");
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
				     .target = sculpt->target,
				     .layer = sculpt->layer };
	uint32_t count = 1;

	VOE_BASE_ASSERT(spacing > 0.0f, "a brush with no radius");
	if (length > spacing)
		count = length / spacing >= (float)VOE_EDITOR_SCULPT_STAMPS ?
				VOE_EDITOR_SCULPT_STAMPS :
				(uint32_t)ceilf(length / spacing);
	for (uint32_t i = 1; i <= count; i++) {
		const float t = (float)i / (float)count;
		const float x = sculpt->last_x + dx * t;
		const float z = sculpt->last_z + dz * t;

		if (foliage_kind(sculpt->kind)) {
			foliage_stamp(sculpt, models, x, z,
				      seconds / (float)count);
			continue;
		}
		sculpt->touched = join(
			sculpt->touched,
			voe_editor_models_brush(models, sculpt->path, &brush, x,
						z, seconds / (float)count,
						scratch));
	}
	sculpt->last_x = sculpt->x;
	sculpt->last_z = sculpt->z;
	VOE_BASE_ASSERT(count <= VOE_EDITOR_SCULPT_STAMPS, "stamps past the cap");
}

// A malloced copy of `land`'s two paint planes, 8 bytes a point; NULL paint
// copied as every point all layer 0.
static uint8_t *paint_copy(const voe_assets_landscape *land)
{
	const size_t points = (size_t)(land->cells + 1) * (land->cells + 1);
	uint8_t *copy = malloc(points * 8);

	VOE_BASE_ASSERT(copy != NULL, "out of memory copying paint at a press");
	if (land->paint != NULL) {
		memcpy(copy, land->paint, points * 8);
		return copy;
	}
	memset(copy, 0, points * 8);
	for (size_t i = 0; i < points; i++)
		copy[i * 4] = 255;
	return copy;
}

// The press: the path and the whole grid's heights copied and flatten's height
// taken, or for PAINT its paint copied, or for SCATTER and ERASE an empty
// changed list made.
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
	if (foliage_kind(sculpt->kind)) {
		sculpt->changed = malloc(VOE_3D_FOLIAGE_TRIES *
					 sizeof(*sculpt->changed));
		VOE_BASE_ASSERT(sculpt->changed != NULL,
				"out of memory starting a foliage stroke");
		sculpt->changed_count = 0;
		sculpt->changed_room = VOE_3D_FOLIAGE_TRIES;
	} else if (sculpt->kind == VOE_3D_BRUSH_PAINT) {
		sculpt->before_paint = paint_copy(land);
	} else {
		count = (size_t)(land->cells + 1) * (land->cells + 1);
		sculpt->before = malloc(count * sizeof(*sculpt->before));
		VOE_BASE_ASSERT(sculpt->before != NULL,
				"out of memory copying a grid at a press");
		memcpy(sculpt->before, land->heights,
		       count * sizeof(*sculpt->before));
		sculpt->target =
			voe_3d_landscape_height(land, sculpt->x, sculpt->z);
	}
	sculpt->last_x = sculpt->x;
	sculpt->last_z = sculpt->z;
	sculpt->touched = (voe_3d_landscape_rect){ 0 };
	sculpt->stroking = true;
}

// The touched rect's heights from the press copy and the store, as a stroke.
static voe_editor_stroke *heights_stroke(const voe_editor_sculpt *sculpt,
					 const voe_assets_landscape *land,
					 voe_3d_landscape_rect rect)
{
	const uint32_t width = rect.x1 - rect.x0;
	const uint32_t side = sculpt->cells + 1;
	const size_t count = (size_t)width * (rect.z1 - rect.z0);
	float *heights = malloc(2 * count * sizeof(*heights));
	voe_editor_stroke *stroke;

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
	stroke = voe_editor_stroke_new(sculpt->path, rect, heights,
				       heights + count);
	free(heights);
	return stroke;
}

// The touched rect's weights from the press copy and the store, as a paint
// stroke: each side plane 0's rect then plane 1's. NULL when the store holds
// no paint, which no stamp changed.
static voe_editor_stroke *paint_stroke(const voe_editor_sculpt *sculpt,
				       const voe_assets_landscape *land,
				       voe_3d_landscape_rect rect)
{
	const uint32_t width = rect.x1 - rect.x0;
	const size_t side = sculpt->cells + 1;
	const size_t plane = side * side * 4;
	const size_t count = (size_t)width * (rect.z1 - rect.z0);
	uint8_t *bytes;
	voe_editor_stroke *stroke;

	if (land->paint == NULL)
		return NULL;
	bytes = malloc(2 * count * 8);
	VOE_BASE_ASSERT(bytes != NULL, "out of memory cutting a stroke's paint");
	for (uint32_t p = 0; p < 2; p++)
		for (uint32_t z = rect.z0; z < rect.z1; z++) {
			const size_t from =
				p * plane + ((size_t)z * side + rect.x0) * 4;
			const size_t to =
				p * count * 4 + (size_t)(z - rect.z0) * width * 4;

			memcpy(bytes + to, sculpt->before_paint + from,
			       (size_t)width * 4);
			memcpy(bytes + count * 8 + to, land->paint + from,
			       (size_t)width * 4);
		}
	stroke = voe_editor_stroke_new_paint(sculpt->path, rect, bytes,
					     bytes + count * 8);
	free(bytes);
	return stroke;
}

// The release: the touched rect's heights or weights from the copy and the
// store, or the changed copies, made a stroke and handed to the undo line, the
// project marked edited. An empty rect, a grid gone or resized under the
// stroke, or no copy changed records nothing.
static void end(voe_editor_sculpt *sculpt, const voe_editor_models *models,
		struct voe_editor_undo *undo,
		struct voe_editor_session *session, voe_base_arena *scratch)
{
	const voe_assets_landscape *land = grid(models, sculpt->path);
	const voe_3d_landscape_rect rect = sculpt->touched;
	voe_editor_stroke *stroke = NULL;

	VOE_BASE_ASSERT(sculpt->stroking && (sculpt->before != NULL ||
					     sculpt->before_paint != NULL ||
					     sculpt->changed != NULL),
			"ending no stroke");
	if (sculpt->changed != NULL) {
		if (sculpt->changed_count > 0)
			stroke = voe_editor_stroke_new_foliage(
				sculpt->path,
				sculpt->kind == VOE_3D_BRUSH_ERASE,
				sculpt->changed, sculpt->changed_count);
	} else if (!rect_empty(rect) && land != NULL &&
		   land->cells == sculpt->cells) {
		stroke = sculpt->before_paint != NULL ?
				 paint_stroke(sculpt, land, rect) :
				 heights_stroke(sculpt, land, rect);
	}
	if (stroke != NULL) {
		voe_editor_undo_stroke(undo, session->project, scratch, stroke);
		voe_editor_session_edited(session);
	}
	free(sculpt->before);
	free(sculpt->before_paint);
	free(sculpt->changed);
	sculpt->before = NULL;
	sculpt->before_paint = NULL;
	sculpt->changed = NULL;
	sculpt->changed_count = 0;
	sculpt->changed_room = 0;
	sculpt->stroking = false;
	VOE_BASE_ASSERT(sculpt->before == NULL, "a stroke's copy kept");
}

// A look's six bytes are its fields with no padding, so its bytes compare.
static_assert(sizeof(voe_assets_landscape_look) == 6,
	      "a look with padding or a field past its six bytes");

// Whether two looks are the same.
static bool look_same(voe_assets_landscape_look a, voe_assets_landscape_look b)
{
	return memcmp(&a, &b, sizeof a) == 0;
}

// The open look edit closed: when its kind's look changed, a look stroke
// handed to the undo line and the project marked edited.
static void look_close(voe_editor_sculpt *sculpt,
		       const voe_editor_models *models,
		       struct voe_editor_undo *undo,
		       struct voe_editor_session *session, voe_base_arena *scratch)
{
	const voe_assets_landscape *land = grid(models, sculpt->looking_path);
	const uint32_t kind = sculpt->looking_kind;

	VOE_BASE_ASSERT(sculpt->looking, "closing no look edit");
	if (land != NULL && kind < land->kind_count &&
	    !look_same(land->looks[kind], sculpt->looking_before)) {
		voe_editor_undo_stroke(undo, session->project, scratch,
				       voe_editor_stroke_new_look(
					       sculpt->looking_path, kind,
					       sculpt->looking_before,
					       land->looks[kind]));
		voe_editor_session_edited(session);
	}
	sculpt->looking = false;
	VOE_BASE_ASSERT(!sculpt->looking, "a look edit left open");
}

// The Foliage read's ask settled: it opens an edit on the selection's loaded
// landscape when none is open, another kind's edit closed first, and sets the
// look live; with no kind's control held the open edit closes. A selection
// with no loaded landscape, or a kind it no longer has, drops the ask.
static void looks_settle(voe_editor_sculpt *sculpt,
			 const voe_editor_scene *scene,
			 voe_editor_models *models,
			 struct voe_editor_undo *undo,
			 struct voe_editor_session *session,
			 voe_base_arena *scratch)
{
	const char *path;
	const voe_assets_landscape *land;

	if (sculpt->look_asked && sculpt->looking &&
	    sculpt->look_kind != sculpt->looking_kind)
		look_close(sculpt, models, undo, session, scratch);
	if (sculpt->look_asked && !sculpt->looking) {
		path = worn(scene, models);
		land = path != NULL ? grid(models, path) : NULL;
		if (land != NULL && sculpt->look_kind < land->kind_count) {
			memcpy(sculpt->looking_path, path, strlen(path) + 1);
			sculpt->looking_kind = sculpt->look_kind;
			sculpt->looking_before = land->looks[sculpt->look_kind];
			sculpt->looking = true;
		}
	}
	if (sculpt->look_asked && sculpt->looking)
		voe_editor_models_look_set(models, sculpt->looking_path,
					   sculpt->looking_kind,
					   sculpt->look_to);
	if (sculpt->looking && !sculpt->look_held)
		look_close(sculpt, models, undo, session, scratch);
	sculpt->look_asked = false;
	sculpt->look_held = false;
	VOE_BASE_ASSERT(!sculpt->looking || sculpt->looking_path[0] != '\0',
			"a look edit with no landscape");
}

// Whether `land` has layer `layer` and kind `kind`.
static bool grow_fits(const voe_assets_landscape *land, uint32_t layer,
		      uint32_t kind)
{
	return land != NULL && layer < land->layer_count &&
	       kind < land->kind_count;
}

// The open grow edit closed: when its density changed, a grow stroke handed
// to the undo line and the project marked edited.
static void grow_close(voe_editor_sculpt *sculpt,
		       const voe_editor_models *models,
		       struct voe_editor_undo *undo,
		       struct voe_editor_session *session, voe_base_arena *scratch)
{
	const voe_assets_landscape *land = grid(models, sculpt->growing_path);
	const uint32_t layer = sculpt->growing_layer;
	const uint32_t kind = sculpt->growing_kind;

	VOE_BASE_ASSERT(sculpt->growing, "closing no grow edit");
	if (grow_fits(land, layer, kind) &&
	    land->grows[layer][kind] != sculpt->growing_before) {
		voe_editor_undo_stroke(undo, session->project, scratch,
				       voe_editor_stroke_new_grow(
					       sculpt->growing_path, layer, kind,
					       sculpt->growing_before,
					       land->grows[layer][kind]));
		voe_editor_session_edited(session);
	}
	sculpt->growing = false;
	VOE_BASE_ASSERT(!sculpt->growing, "a grow edit left open");
}

// The Layers read's ask settled as looks_settle settles a look's: another
// layer's or kind's edit closed first, one opened on the selection's loaded
// landscape, the density set live, closed with no control held. A selection
// with no loaded landscape, or a layer or kind it no longer has, drops it.
static void grows_settle(voe_editor_sculpt *sculpt,
			 const voe_editor_scene *scene,
			 voe_editor_models *models,
			 struct voe_editor_undo *undo,
			 struct voe_editor_session *session,
			 voe_base_arena *scratch)
{
	const char *path;
	const voe_assets_landscape *land;

	VOE_BASE_ASSERT(sculpt->grow_to <= VOE_EDITOR_SCULPT_GROW_MAX,
			"a growth asked past its most");
	if (sculpt->grow_asked && sculpt->growing &&
	    (sculpt->grow_layer != sculpt->growing_layer ||
	     sculpt->grow_kind != sculpt->growing_kind))
		grow_close(sculpt, models, undo, session, scratch);
	if (sculpt->grow_asked && !sculpt->growing) {
		path = worn(scene, models);
		land = path != NULL ? grid(models, path) : NULL;
		if (grow_fits(land, sculpt->grow_layer, sculpt->grow_kind)) {
			memcpy(sculpt->growing_path, path, strlen(path) + 1);
			sculpt->growing_layer = sculpt->grow_layer;
			sculpt->growing_kind = sculpt->grow_kind;
			sculpt->growing_before =
				land->grows[sculpt->grow_layer][sculpt->grow_kind];
			sculpt->growing = true;
		}
	}
	if (sculpt->grow_asked && sculpt->growing &&
	    grow_fits(grid(models, sculpt->growing_path), sculpt->growing_layer,
		      sculpt->growing_kind))
		voe_editor_models_grow_set(models, sculpt->growing_path,
					   sculpt->growing_layer,
					   sculpt->growing_kind, sculpt->grow_to);
	if (sculpt->growing && !sculpt->grow_held)
		grow_close(sculpt, models, undo, session, scratch);
	sculpt->grow_asked = false;
	sculpt->grow_held = false;
	VOE_BASE_ASSERT(!sculpt->growing || sculpt->growing_path[0] != '\0',
			"a grow edit with no landscape");
}

// The look and grow edits are settled first, whether or not a brush is
// chosen: the Foliage and Layers sections' controls are not a brush.
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
	looks_settle(sculpt, scene, models, undo, session, scratch);
	grows_settle(sculpt, scene, models, undo, session, scratch);
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
