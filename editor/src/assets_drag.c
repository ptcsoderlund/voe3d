// The drag's start from the held row, the outcome at the pointer each frame
// for the ghost, and its release into a view, onto the Inspector, onto a
// folder row or Up, or nowhere. See the header for the six outcomes, the point
// a placed model lands at and why the Inspector drop replaces the path.
#include "assets_drag.h"

#include "assets_manage.h"
#include "entities.h"
#include "scene_list.h"

#include <3d/pick.h>

#include <base/arena.h>
#include <base/assert.h>

#include <platform/path.h>

#include <stdio.h>
#include <string.h>

// How far along the ray a model lands that meets nothing and no ground.
#define DROP_METRES 10.0
// A move's scratch block, made at the release only; a size, not a limit.
#define MOVE_SCRATCH (64u * 1024u)

// The held row's path into `drag`, `\` from a joined subfolder made `/`, and
// its kind. False when it does not fit its kind's row, or the drag's for a
// row that only moves.
static bool path_hold(voe_editor_assets_drag *drag,
		      const voe_editor_assets *assets)
{
	size_t room = assets->held_prefab  ? VOE_SCENE_PREFAB_PATH :
		      assets->held_picture ? VOE_3D_EMITTER_TEXTURE :
		      assets->held_model   ? VOE_3D_MODEL_PATH :
					     sizeof drag->path;
	int length = assets->shown[0] == '\0' ?
			     snprintf(drag->path, sizeof drag->path,
				      "Assets/%s", assets->held) :
			     snprintf(drag->path, sizeof drag->path,
				      "Assets/%s/%s", assets->shown,
				      assets->held);

	VOE_BASE_ASSERT(room <= sizeof drag->path, "a row longer than the drag");
	if (length < 0 || (size_t)length >= room)
		return false;
	drag->prefab = assets->held_prefab;
	drag->picture = assets->held_picture;
	drag->moves_only = !assets->held_model && !assets->held_prefab &&
			   !assets->held_picture;
	for (int i = 0; i < length; i++)
		if (drag->path[i] == '\\')
			drag->path[i] = '/';
	return true;
}

// The hit, else y = 0 in front of the eye, else DROP_METRES along the ray.
static voe_math_double3 drop_point(voe_ecs_world *world,
				   const voe_3d_shape_geometries *geometries,
				   const voe_3d_models *models, voe_3d_ray ray)
{
	float distance = 0.0f;
	double along = DROP_METRES;

	if (voe_3d_pick(world, geometries, models, ray, &distance).generation !=
	    0)
		along = distance;
	else if (ray.direction.y != 0.0f && -ray.origin.y / ray.direction.y > 0.0)
		along = -ray.origin.y / ray.direction.y;
	return (voe_math_double3){ ray.origin.x + ray.direction.x * along,
				   ray.origin.y + ray.direction.y * along,
				   ray.origin.z + ray.direction.z * along };
}

// A new thing wearing the held path, or a placed copy of the held prefab,
// where the view's ray lands, selected.
static bool drop_into_view(const voe_editor_assets_drag *drag,
			   voe_editor_scene *scene,
			   const voe_editor_view *view, voe_math_float2 point,
			   const voe_3d_shape_geometries *geometries,
			   const voe_3d_models *models)
{
	voe_ecs_entity made;
	voe_3d_ray ray = voe_3d_pick_ray(
		voe_editor_view_pass_camera(view, (voe_render_light){ 0 }).view,
		view->eye,
		(voe_platform_size){ (int)view->width, (int)view->height },
		point);
	voe_math_double3 at = drop_point(scene->world, geometries, models, ray);

	if (drag->prefab ?
		    !voe_editor_entities_prefab_add(scene->world, drag->path, at,
						    &made) :
		    !voe_editor_entities_model_add(scene->world, drag->path, at,
						   &made))
		return false;
	voe_editor_scene_select(scene, made);
	return true;
}

// The header's six outcomes; NOTHING is the one a ghost shows refused.
typedef enum {
	OUTCOME_NOTHING,
	OUTCOME_MODEL_INTO_VIEW,
	OUTCOME_MODEL_ONTO_INSPECTOR,
	OUTCOME_PREFAB_INTO_VIEW,
	OUTCOME_PICTURE_ONTO_INSPECTOR,
	OUTCOME_MOVE,
} outcome_kind;

// What a release at a pointer would do, the view and point it lands in, a
// move's path relative to `Assets/`, and, for a prefab over a view while one
// is open, why nothing, for the notice.
typedef struct {
	outcome_kind kind;
	uint32_t view;
	voe_math_float2 point;
	char to[VOE_EDITOR_ASSETS_PATH];
	const char *why;
} outcome;

// Where the dragged row moves to when released at `pointer`, relative to
// `Assets/`, into `to`: into the folder row under it or the shown folder's
// parent under Up. False over anything else, when it does not fit, or when
// it would land in or below the dragged row itself.
static bool move_target(const voe_editor_assets_drag *drag,
			const voe_editor_assets *assets,
			voe_math_float2 pointer, char *to, size_t size)
{
	const voe_editor_assets_at under =
		voe_editor_assets_row_at(assets, pointer);
	const char *from = drag->path + strlen("Assets/");
	const size_t from_length = strlen(from);
	char folder[VOE_EDITOR_ASSETS_PATH];
	int length;

	VOE_BASE_ASSERT(strncmp(drag->path, "Assets/", 7) == 0,
			"a dragged row outside Assets/");
	VOE_BASE_ASSERT(to != NULL && size > 0, "a move target into nowhere");
	length = snprintf(folder, sizeof folder, "%s", assets->shown);
	if (length < 0 || (size_t)length >= sizeof folder)
		return false;
	for (char *c = folder; *c != '\0'; c++)
		if (*c == '\\')
			*c = '/';
	if (under.kind == VOE_EDITOR_ASSETS_AT_UP) {
		char *last = strrchr(folder, '/');

		*(last != NULL ? last : folder) = '\0';
	} else if (under.kind == VOE_EDITOR_ASSETS_AT_ROW &&
		   assets->rows[under.row].folder) {
		const size_t used = strlen(folder);

		length = snprintf(folder + used, sizeof folder - used, "%s%s",
				  used == 0 ? "" : "/",
				  assets->rows[under.row].name);
		if (length < 0 || (size_t)length >= sizeof folder - used)
			return false;
	} else {
		return false;
	}
	length = folder[0] == '\0' ?
			 snprintf(to, size, "%s", voe_platform_path_name(from)) :
			 snprintf(to, size, "%s/%s", folder,
				  voe_platform_path_name(from));
	if (length < 0 || (size_t)length >= size)
		return false;
	return !(strncmp(to, from, from_length) == 0 &&
		 to[from_length] == '/');
}

// The one answer the release and the ghost both read.
static outcome outcome_at(const voe_editor_assets_drag *drag,
			  const voe_editor_session *session,
			  const voe_editor_scene *scene,
			  const voe_editor_views *views,
			  const voe_editor_dock_root *root,
			  const voe_editor_topbar *bar, voe_math_float2 pointer,
			  bool blocked)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	outcome out = { .kind = OUTCOME_NOTHING };

	VOE_BASE_ASSERT(drag != NULL && session != NULL && scene != NULL,
			"an outcome of no drag");
	VOE_BASE_ASSERT(views != NULL && root != NULL && bar != NULL,
			"an outcome over no editor");
	if (blocked)
		return out;
	if (move_target(drag, &scene->assets, pointer, out.to, sizeof out.to))
		out.kind = OUTCOME_MOVE;
	else if (drag->moves_only)
		return out;
	else if (voe_editor_views_under(views, pointer, &out.view, &out.point)) {
		if (!drag->prefab && !drag->picture)
			out.kind = OUTCOME_MODEL_INTO_VIEW;
		else if (drag->prefab && session->project->prefab[0] == '\0')
			out.kind = OUTCOME_PREFAB_INTO_VIEW;
		else if (drag->prefab)
			out.why = "A prefab is not placed while one is open.";
	} else if (voe_editor_dock_over_panel(
			   root, voe_editor_topbar_high(bar, root->size.y),
			   VOE_EDITOR_PANEL_INSPECTOR, pointer) &&
		   !drag->prefab &&
		   !voe_editor_inspector_is_part(scene->world, selected, NULL)) {
		if (drag->picture &&
		    voe_3d_emitter_get(scene->world, selected) != NULL)
			out.kind = OUTCOME_PICTURE_ONTO_INSPECTOR;
		else if (!drag->picture &&
			 voe_3d_model_get(scene->world, selected) != NULL)
			out.kind = OUTCOME_MODEL_ONTO_INSPECTOR;
	}
	return out;
}

// The selected emitter's row with `path` as its texture, submitted whole.
static bool texture_swap(voe_ecs_world *world, voe_ecs_entity entity,
			 const char *path)
{
	const voe_3d_emitter *emitter = voe_3d_emitter_get(world, entity);
	voe_3d_emitter_intent intent = { .entity = entity };

	VOE_BASE_ASSERT(emitter != NULL, "a texture for no emitter");
	VOE_BASE_ASSERT(path != NULL && strlen(path) < VOE_3D_EMITTER_TEXTURE,
			"a texture path longer than its room");
	intent.emitter = *emitter;
	snprintf(intent.emitter.texture, sizeof intent.emitter.texture, "%s",
		 path);
	return voe_3d_emitter_submit(world, intent);
}

// The release: into a view, onto the Inspector, or nothing. Whether an edit
// reached the project; a full queue says so in the notice.
static void drop(const voe_editor_assets_drag *drag,
		 voe_editor_session *session, voe_editor_undo *undo,
		 voe_editor_scene *scene, const voe_editor_views *views,
		 const voe_editor_dock_root *root, const voe_editor_topbar *bar,
		 const voe_3d_shape_geometries *geometries,
		 const voe_3d_models *models, voe_math_float2 pointer)
{
	outcome out = outcome_at(drag, session, scene, views, root, bar,
				 pointer, false);
	voe_3d_model_intent swap = { .entity =
					     voe_editor_scene_selected(scene) };
	bool done;

	if (out.kind == OUTCOME_NOTHING) {
		if (out.why != NULL)
			voe_editor_notice_set(&session->notice, "%s", out.why);
		return;
	}
	// A move marks nothing unsaved: it forgets the undo line and keeps the
	// scene's flag itself (assets_manage.h), and a false has said why.
	if (out.kind == OUTCOME_MOVE) {
		voe_base_arena *scratch = voe_base_arena_new(MOVE_SCRATCH);

		(void)voe_editor_assets_move(session, scene, undo, scratch,
					     drag->path + strlen("Assets/"),
					     out.to);
		voe_base_arena_destroy(scratch);
		return;
	}
	if (out.kind == OUTCOME_MODEL_ONTO_INSPECTOR) {
		snprintf(swap.model.path, sizeof swap.model.path, "%s",
			 drag->path);
		done = voe_3d_model_submit(scene->world, swap);
	} else if (out.kind == OUTCOME_PICTURE_ONTO_INSPECTOR) {
		done = texture_swap(scene->world, swap.entity, drag->path);
	} else {
		done = drop_into_view(drag, scene, &views->views[out.view],
				      out.point, geometries, models);
	}
	if (!done) {
		voe_editor_notice_set(&session->notice, "The scene is full.");
		return;
	}
	voe_editor_session_edited(session);
	voe_editor_undo_edited(undo);
}

void voe_editor_assets_drag_read(
	voe_editor_assets_drag *drag, voe_editor_session *session,
	voe_editor_undo *undo, voe_editor_scene *scene,
	const voe_editor_views *views, const voe_editor_dock_root *root,
	const voe_editor_topbar *bar, const voe_3d_shape_geometries *geometries,
	const voe_3d_models *models, voe_math_float2 pointer, bool down,
	bool blocked)
{
	VOE_BASE_ASSERT(drag != NULL && session != NULL && undo != NULL,
			"dragging with nothing to drag into");
	VOE_BASE_ASSERT(scene != NULL && views != NULL && root != NULL &&
				bar != NULL && geometries != NULL,
			"dragging over no editor");

	if (!drag->holding) {
		drag->holding = down && !blocked &&
				scene->assets.held != NULL &&
				path_hold(drag, &scene->assets);
		drag->from = pointer;
		drag->dragging = false;
		drag->refused = false;
		return;
	}
	// The panel's read after this frame then fires no row the drag is over.
	if (drag->dragging)
		voe_editor_assets_held_dragged(&scene->assets);
	if (down) {
		if (voe_math_float2_length(voe_math_float2_sub(
			    pointer, drag->from)) >= VOE_EDITOR_SCENE_DRAG_START) {
			drag->dragging = true;
			voe_editor_assets_held_dragged(&scene->assets);
		}
		drag->refused = drag->dragging &&
				outcome_at(drag, session, scene, views, root,
					   bar, pointer, blocked)
						.kind == OUTCOME_NOTHING;
		return;
	}
	drag->holding = false;
	if (drag->dragging && !blocked)
		drop(drag, session, undo, scene, views, root, bar, geometries,
		     models, pointer);
	drag->dragging = false;
	drag->refused = false;
}

void voe_editor_assets_drag_ghost_draw(voe_ui_context *ui,
				       const voe_ui_theme *dim,
				       const voe_editor_assets_drag *drag,
				       voe_math_float2 at)
{
	VOE_BASE_ASSERT(ui != NULL && dim != NULL && drag != NULL,
			"a ghost of no drag");
	VOE_BASE_ASSERT(!drag->dragging || drag->holding,
			"dragging a row nothing holds");

	if (drag->holding && drag->dragging)
		voe_editor_drag_ghost_draw(ui, dim,
					   voe_platform_path_name(drag->path),
					   drag->refused, at);
}
