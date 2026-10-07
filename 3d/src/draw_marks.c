// The editor's marks drawn over the world: a scene camera's box and frustum,
// every sun's circle and arrow, every point light's three circles, every bare
// place's diamond, the brush's two rings, every light blocker's box, one entity's
// silhouette, a collider's and the selected blocker's lines and the gizmo's two
// meshes, arrows or rings,
// each as this frame's transient geometry and one or two draws, at the entity's
// world place. See draw_marks.h.
//
// All of them are in metres about the frame's eye already (ADR-0250), so every
// object here has identity matrices; the record is the caller's unlit one and the colour the caller's.
#include "draw_marks.h"

#include <3d/brush_marker.h>
#include <3d/camera_marker.h>
#include <3d/collider_marker.h>
#include <3d/gizmo.h>
#include <3d/gizmo_rings.h>
#include <3d/light_blocker.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/outline.h>
#include <3d/place_marker.h>
#include <3d/point_light_marker.h>
#include <3d/sun_marker.h>
#include <base/assert.h>
#include <base/error.h>
#include <ecs/component.h>
#include <physics/shape.h>
#include <scene/camera_component.h>
#include <scene/light_blocker_component.h>
#include <scene/light_component.h>
#include <scene/point_light_component.h>
#include <scene/transform_component.h>

// An unlit record at the identity, in one opaque colour: what every mark is
// drawn with.
static voe_render_object mark_object(voe_3d_material material,
				     voe_math_float3 colour)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = material.shading.index,
		.colour = { colour.x, colour.y, colour.z, 1.0f },
	};
}

// The scene camera's marker as this frame's geometry and one draw, in world
// metres about the eye like the outline's quads, so both matrices are the identity (0223).
// A zeroed entity, a dead one, and one without a camera or a transform draw
// nothing; so does a refused transient range, which render reports.
void voe_3d_draw_marks_camera(const voe_ecs_world *world,
			      voe_render_device *device, voe_base_arena *arena,
			      voe_3d_frame frame)
{
	voe_3d_camera_marked marker = frame.marker;
	const voe_scene_camera *lens;
	voe_scene_transform pose;
	voe_3d_outline_mesh mesh;
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing a camera marker to no device");
	VOE_BASE_ASSERT(arena != NULL, "a camera marker with no arena");

	if (!voe_ecs_entity_alive(world, marker.entity))
		return;
	lens = voe_scene_camera_get(world, marker.entity);
	if (lens == NULL || voe_scene_transform_get(world, marker.entity) == NULL)
		return;
	pose = voe_scene_transform_world(world, marker.entity);
	if (voe_3d_camera_marker_quads(pose, *lens, frame.view, frame.eye,
				       marker.size,
				       marker.pixels, arena, &mesh) &&
	    voe_render_geometry_create_transient(device, mesh.vertices,
						 mesh.vertex_count, mesh.indices,
						 mesh.index_count, &quads,
						 &error))
		(void)voe_render_frame_draw(
			device, quads,
			mark_object(marker.material, marker.colour));
}

// Whether the world registered the table under `key`, walked as pick.c's
// has_store is, because voe_ecs_component_type asserts on a key nothing
// registered.
static bool has_store(const voe_ecs_world *world, const struct voe_ecs_key *key)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++)
		if (voe_ecs_component_key(world,
					  voe_ecs_component_type_at(world, i)) ==
		    key)
			return true;
	return false;
}

// One mesh of light markers as this frame's geometry and one draw; an empty one
// is no draw, and a refused range draws nothing, which render reports.
static void draw_marker_lines(voe_render_device *device,
			      voe_3d_outline_mesh mesh,
			      voe_3d_material material, voe_math_float3 colour)
{
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing light markers to no device");
	VOE_BASE_ASSERT(mesh.index_count == 0 ||
				(mesh.vertices != NULL && mesh.indices != NULL),
			"light markers with no arrays behind them");

	if (mesh.index_count == 0)
		return;
	if (voe_render_geometry_create_transient(device, mesh.vertices,
						 mesh.vertex_count, mesh.indices,
						 mesh.index_count, &quads, &error))
		(void)voe_render_frame_draw(device, quads,
					    mark_object(material, colour));
}

// Every point light with a transform, marked as the sun is (0320 point 7) at
// its current world place, the lag the sun's marker uses: the selected one as a
// draw of its own in `selected_colour`, the rest gathered into one geometry in
// `colour`, each lamp's indices moved past the vertices already gathered. Not
// shown, no point light table and a picture with no area draw none.
void voe_3d_draw_marks_point_lights(const voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *arena, voe_3d_frame frame)
{
	voe_3d_point_lights_marked marked = frame.point_lights;
	voe_3d_outline_mesh selected = { 0 };
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	VOE_BASE_ASSERT(device != NULL, "drawing lamp markers to no device");
	VOE_BASE_ASSERT(arena != NULL, "lamp markers with no arena");

	if (!marked.shown || !has_store(world, &voe_scene_point_light_key))
		return;
	uint32_t count = voe_scene_point_light_count(world);
	const voe_ecs_entity *owners = voe_scene_point_light_entities(world);

	if (count == 0)
		return;
	vertices = voe_base_arena_push(
		arena, sizeof *vertices * count * VOE_3D_POINT_LIGHT_MARKER_VERTICES);
	indices = voe_base_arena_push(
		arena, sizeof *indices * count * VOE_3D_POINT_LIGHT_MARKER_INDICES);
	for (uint32_t i = 0; i < count; i++) {
		voe_3d_outline_mesh mesh;

		if (voe_scene_transform_get(world, owners[i]) == NULL)
			continue;
		if (!voe_3d_point_light_marker_quads(
			    voe_scene_transform_world(world, owners[i]).position,
			    frame.view, frame.eye, marked.size, marked.pixels,
			    arena, &mesh))
			return;
		if (owners[i].index == marked.selected.index &&
		    owners[i].generation == marked.selected.generation) {
			selected = mesh;
			continue;
		}
		for (uint32_t j = 0; j < mesh.index_count; j++)
			indices[index_count + j] = mesh.indices[j] + vertex_count;
		for (uint32_t j = 0; j < mesh.vertex_count; j++)
			vertices[vertex_count + j] = mesh.vertices[j];
		index_count += mesh.index_count;
		vertex_count += mesh.vertex_count;
	}
	VOE_BASE_ASSERT(vertex_count <= count * VOE_3D_POINT_LIGHT_MARKER_VERTICES,
			"more lamp marker vertices than were made room for");
	draw_marker_lines(device,
			  (voe_3d_outline_mesh){ vertices, vertex_count, indices,
						 index_count },
			  marked.material, marked.colour);
	draw_marker_lines(device, selected, marked.material,
			  marked.selected_colour);
}

// Every transform's owner voe_3d_place_marker_wanted answers, marked as the
// point lights are (0365 points 1-2) at its current world place: the selected
// one alone in `selected_colour`, the rest gathered into one geometry in
// `colour`. Not shown, no transform table and a picture with no area draw none.
void voe_3d_draw_marks_places(const voe_ecs_world *world,
			      voe_render_device *device, voe_base_arena *arena,
			      voe_3d_frame frame)
{
	voe_3d_rows_marked marked = frame.places;
	voe_3d_outline_mesh selected = { 0 };
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	VOE_BASE_ASSERT(device != NULL, "drawing place markers to no device");
	VOE_BASE_ASSERT(arena != NULL, "place markers with no arena");

	if (!marked.shown || !has_store(world, &voe_scene_transform_key))
		return;
	uint32_t count = voe_scene_transform_count(world);
	const voe_ecs_entity *owners = voe_scene_transform_entities(world);

	if (count == 0)
		return;
	vertices = voe_base_arena_push(
		arena, sizeof *vertices * count * VOE_3D_PLACE_MARKER_VERTICES);
	indices = voe_base_arena_push(
		arena, sizeof *indices * count * VOE_3D_PLACE_MARKER_INDICES);
	for (uint32_t i = 0; i < count; i++) {
		voe_3d_outline_mesh mesh;

		if (!voe_3d_place_marker_wanted(world, owners[i]))
			continue;
		if (!voe_3d_place_marker_quads(
			    voe_scene_transform_world(world, owners[i]).position,
			    frame.view, frame.eye, marked.size, marked.pixels,
			    arena, &mesh))
			return;
		if (owners[i].index == marked.selected.index &&
		    owners[i].generation == marked.selected.generation) {
			selected = mesh;
			continue;
		}
		for (uint32_t j = 0; j < mesh.index_count; j++)
			indices[index_count + j] = mesh.indices[j] + vertex_count;
		for (uint32_t j = 0; j < mesh.vertex_count; j++)
			vertices[vertex_count + j] = mesh.vertices[j];
		index_count += mesh.index_count;
		vertex_count += mesh.vertex_count;
	}
	VOE_BASE_ASSERT(vertex_count <= count * VOE_3D_PLACE_MARKER_VERTICES,
			"more place marker vertices than were made room for");
	draw_marker_lines(device,
			  (voe_3d_outline_mesh){ vertices, vertex_count, indices,
						 index_count },
			  marked.material, marked.colour);
	draw_marker_lines(device, selected, marked.material,
			  marked.selected_colour);
}

// The brush's two rings on the landscape `frame.brush` names (0379 point 3),
// at its current world place, as one geometry and one draw in the world's
// depth. A zeroed or dead entity, no model table, no store, no loaded
// landscape, no transform and a picture with no area draw none.
void voe_3d_draw_marks_brush(const voe_ecs_world *world,
			     voe_render_device *device, voe_base_arena *arena,
			     voe_3d_frame frame)
{
	voe_3d_brush_marked brush = frame.brush;
	const voe_3d_model *worn;
	const voe_3d_model_entry *entry;
	voe_3d_outline_mesh mesh;

	VOE_BASE_ASSERT(device != NULL, "drawing a brush to no device");
	VOE_BASE_ASSERT(arena != NULL, "a brush with no arena");

	if (frame.models == NULL || !voe_ecs_entity_alive(world, brush.entity) ||
	    !has_store(world, &voe_3d_model_key) ||
	    voe_scene_transform_get(world, brush.entity) == NULL)
		return;
	worn = voe_3d_model_get(world, brush.entity);
	if (worn == NULL)
		return;
	entry = voe_3d_models_find(frame.models, worn->path);
	if (entry == NULL || !entry->loaded || entry->landscape == NULL)
		return;
	if (voe_3d_brush_marker_quads(
		    entry->landscape,
		    voe_scene_transform_world(world, brush.entity), brush.x,
		    brush.z, brush.radius, brush.inner, frame.view, frame.eye,
		    brush.size, brush.pixels, arena, &mesh))
		draw_marker_lines(device, mesh, brush.material, brush.colour);
}

// Every light row with a transform, marked as the point lights are (0360 point
// 1) at its current world place, in table order and past the fourth too: the
// selected one alone in `selected_colour`, the rest gathered into one geometry
// in `colour`, each sun's indices moved past the vertices already gathered. Not
// shown, no light table and a picture with no area draw none.
void voe_3d_draw_marks_sun(const voe_ecs_world *world,
			   voe_render_device *device, voe_base_arena *arena,
			   voe_3d_frame frame)
{
	voe_3d_sun_marked marked = frame.sun;
	voe_3d_outline_mesh selected = { 0 };
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	VOE_BASE_ASSERT(device != NULL, "drawing sun markers to no device");
	VOE_BASE_ASSERT(arena != NULL, "sun markers with no arena");

	if (!marked.shown || !has_store(world, &voe_scene_light_key))
		return;
	uint32_t count = voe_scene_light_count(world);
	const voe_ecs_entity *owners = voe_scene_light_entities(world);

	if (count == 0)
		return;
	vertices = voe_base_arena_push(
		arena, sizeof *vertices * count * VOE_3D_SUN_MARKER_VERTICES);
	indices = voe_base_arena_push(
		arena, sizeof *indices * count * VOE_3D_SUN_MARKER_INDICES);
	for (uint32_t i = 0; i < count; i++) {
		voe_3d_outline_mesh mesh;

		if (voe_scene_transform_get(world, owners[i]) == NULL)
			continue;
		if (!voe_3d_sun_marker_quads(
			    voe_scene_transform_world(world, owners[i]),
			    frame.view, frame.eye, marked.size, marked.pixels,
			    arena, &mesh))
			return;
		if (owners[i].index == marked.selected.index &&
		    owners[i].generation == marked.selected.generation) {
			selected = mesh;
			continue;
		}
		for (uint32_t j = 0; j < mesh.index_count; j++)
			indices[index_count + j] = mesh.indices[j] + vertex_count;
		for (uint32_t j = 0; j < mesh.vertex_count; j++)
			vertices[vertex_count + j] = mesh.vertices[j];
		index_count += mesh.index_count;
		vertex_count += mesh.vertex_count;
	}
	VOE_BASE_ASSERT(vertex_count <= count * VOE_3D_SUN_MARKER_VERTICES,
			"more sun marker vertices than were made room for");
	draw_marker_lines(device,
			  (voe_3d_outline_mesh){ vertices, vertex_count, indices,
						 index_count },
			  marked.material, marked.colour);
	draw_marker_lines(device, selected, marked.material,
			  marked.selected_colour);
}

// THE OUTLINE IS DRAWN AFTER THE OVERLAY AND AGAINST AN EMPTY DEPTH BUFFER,
// WHICH IS WHAT MAKES IT SHOW THROUGH. Nothing already drawn can be in front of
// it, while the entity it belongs to was drawn where it really is, in its own
// layer, untouched. A world with nothing above it has not cleared depth, so this
// clears it: the clear is what the outline is drawn against, and it is made once
// either way. See `outlined` in 3d/draw_system.h and ADR-0203.
//
// A REFUSED TRANSIENT RANGE DRAWS NO OUTLINE AND NOTHING ELSE CHANGES. render
// has already said so on stderr, and the rest of the frame is drawn, ended and
// presented — the same rule every other draw follows.
//
// Whether depth is emptied now: `cleared`, or true once this has cleared it.
bool voe_3d_draw_marks_outline(const voe_ecs_world *world,
			       voe_render_device *device, voe_base_arena *arena,
			       voe_3d_frame frame, bool cleared)
{
	voe_3d_outline_mesh outline;
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing an outline to no device");
	VOE_BASE_ASSERT(arena != NULL, "an outline with no arena");

	if (!voe_ecs_entity_alive(world, frame.outlined.entity))
		return cleared;
	if (!voe_3d_outline_quads(world, frame.outlined, frame.view,
				  frame.eye, arena, &outline) ||
	    !voe_render_geometry_create_transient(
		    device, outline.vertices, outline.vertex_count,
		    outline.indices, outline.index_count, &quads, &error))
		return cleared;

	if (!cleared)
		voe_render_frame_clear_depth(device);
	(void)voe_render_frame_draw(device, quads,
				    mark_object(frame.outlined.material,
						frame.outlined.colour));
	return true;
}

// Every light blocker with a transform but the selected one, its box at the
// frame's lag as voe_3d_collider_marker_quads' lines gathered into one geometry
// in `colour`, in the world's depth so a wall in front hides them (0365 point
// 5). Not shown, no blocker table and a picture with no area draw none.
void voe_3d_draw_marks_light_blockers(const voe_ecs_world *world,
				      voe_render_device *device,
				      voe_base_arena *arena, voe_3d_frame frame)
{
	voe_3d_rows_marked marked = frame.light_blockers;
	voe_render_vertex *vertices;
	uint32_t *indices;
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	VOE_BASE_ASSERT(device != NULL, "drawing blocker lines to no device");
	VOE_BASE_ASSERT(arena != NULL, "blocker lines with no arena");

	if (!marked.shown || !has_store(world, &voe_scene_light_blocker_key))
		return;
	uint32_t count = voe_scene_light_blocker_count(world);
	const voe_ecs_entity *owners = voe_scene_light_blocker_entities(world);

	if (count == 0)
		return;
	vertices = voe_base_arena_push(
		arena, sizeof *vertices * count * VOE_3D_COLLIDER_MARKER_VERTICES);
	indices = voe_base_arena_push(
		arena, sizeof *indices * count * VOE_3D_COLLIDER_MARKER_INDICES);
	for (uint32_t i = 0; i < count; i++) {
		voe_physics_shape shape;
		voe_3d_outline_mesh mesh;

		if (owners[i].index == marked.selected.index &&
		    owners[i].generation == marked.selected.generation)
			continue;
		if (!voe_3d_light_blocker_shape(world, owners[i], frame.lag,
						&shape))
			continue;
		if (!voe_3d_collider_marker_quads(shape, frame.view, frame.eye,
						  marked.size, marked.pixels,
						  arena, &mesh))
			return;
		for (uint32_t j = 0; j < mesh.index_count; j++)
			indices[index_count + j] = mesh.indices[j] + vertex_count;
		for (uint32_t j = 0; j < mesh.vertex_count; j++)
			vertices[vertex_count + j] = mesh.vertices[j];
		index_count += mesh.index_count;
		vertex_count += mesh.vertex_count;
	}
	VOE_BASE_ASSERT(vertex_count <= count * VOE_3D_COLLIDER_MARKER_VERTICES,
			"more blocker line vertices than were made room for");
	draw_marker_lines(device,
			  (voe_3d_outline_mesh){ vertices, vertex_count, indices,
						 index_count },
			  marked.material, marked.colour);
}

// THE COLLIDER'S AND THE SELECTED BLOCKER'S LINES SHARE THE OUTLINE'S CLEAR
// (0253, 0347 point 5). `shape` is drawn as voe_3d_collider_marker_quads' lines
// `pixels` wide on a picture of `size` against the depth the outline was,
// clearing it only when nothing has, so they show through what stands in
// front, in `material` and `colour`. A refused transient range draws nothing,
// which render reports.
static void draw_shape_lines(voe_render_device *device, voe_base_arena *arena,
			     voe_3d_frame frame, voe_physics_shape shape,
			     voe_3d_material material, voe_math_float3 colour,
			     float pixels, voe_platform_size size, bool cleared)
{
	voe_3d_outline_mesh mesh;
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing shape lines to no device");
	VOE_BASE_ASSERT(arena != NULL, "shape lines with no arena");

	if (!voe_3d_collider_marker_quads(shape, frame.view, frame.eye, size,
					  pixels, arena, &mesh) ||
	    !voe_render_geometry_create_transient(
		    device, mesh.vertices, mesh.vertex_count, mesh.indices,
		    mesh.index_count, &quads, &error))
		return;

	if (!cleared)
		voe_render_frame_clear_depth(device);
	(void)voe_render_frame_draw(device, quads,
				    mark_object(material, colour));
}

// The collider's lines (0253). A zeroed entity, a dead one and one with no
// collider draw nothing.
void voe_3d_draw_marks_collider(const voe_ecs_world *world,
				voe_render_device *device, voe_base_arena *arena,
				voe_3d_frame frame, bool cleared)
{
	voe_physics_shape shape;

	VOE_BASE_ASSERT(device != NULL, "drawing a collider to no device");
	VOE_BASE_ASSERT(arena != NULL, "a collider's lines with no arena");

	if (!voe_ecs_entity_alive(world, frame.collider.entity) ||
	    !voe_physics_shape_of(world, frame.collider.entity, &shape))
		return;
	draw_shape_lines(device, arena, frame, shape, frame.outlined.material,
			 frame.outlined.colour, frame.collider.pixels,
			 frame.collider.size, cleared);
}

// The selected blocker's box as the collider's lines (0347 point 5), the box
// voe_3d_light_blocker_shape gives at the frame's lag, so the lines are the box
// that blocks, in `light_blockers`' selected colour and material. Not shown, a
// zeroed entity, a dead one and one with no blocker or no transform draw
// nothing. A live entity needs the blocker table registered.
void voe_3d_draw_marks_light_blocker(const voe_ecs_world *world,
				     voe_render_device *device,
				     voe_base_arena *arena, voe_3d_frame frame,
				     bool cleared)
{
	voe_3d_rows_marked marked = frame.light_blockers;
	voe_physics_shape shape;

	VOE_BASE_ASSERT(device != NULL, "drawing a blocker to no device");
	VOE_BASE_ASSERT(arena != NULL, "a blocker's lines with no arena");

	if (!marked.shown || !voe_ecs_entity_alive(world, marked.selected) ||
	    !voe_3d_light_blocker_shape(world, marked.selected, frame.lag,
					&shape))
		return;
	draw_shape_lines(device, arena, frame, shape, marked.material,
			 marked.selected_colour, marked.pixels, marked.size,
			 cleared);
}

// One of the gizmo's two meshes, as this frame's geometry and one draw. An
// empty mesh is no draw at all, which is what a gizmo with nothing marked hands
// back for its marked one. A refused transient range draws nothing and changes
// nothing else — render has already said so on stderr.
static void draw_gizmo_mesh(voe_render_device *device, voe_3d_gizmo_mesh mesh,
			    voe_3d_material material, voe_math_float3 colour)
{
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing a gizmo to no device");
	VOE_BASE_ASSERT(mesh.index_count == 0 ||
				(mesh.vertices != NULL && mesh.indices != NULL),
			"a gizmo mesh of triangles with no arrays behind it");

	if (mesh.index_count == 0)
		return;
	if (voe_render_geometry_create_transient(device, mesh.vertices,
						 mesh.vertex_count,
						 mesh.indices, mesh.index_count,
						 &quads, &error))
		(void)voe_render_frame_draw(device, quads,
					    mark_object(material, colour));
}

// THE GIZMO IS AFTER EVEN THE OUTLINE, BEHIND A CLEAR OF ITS OWN. The outline
// is in the same depth buffer and cuts across an arrow that stands in front of
// it, so the gizmo is given an empty buffer too: that second clear is the whole
// of what puts it in front of everything in the picture, and it still occludes
// itself. Two draws, because the marked handle is a colour of its own
// (ADR-0205).
void voe_3d_draw_marks_gizmo(const voe_ecs_world *world,
			     voe_render_device *device, voe_base_arena *arena,
			     voe_3d_frame frame)
{
	voe_scene_transform transform;
	voe_3d_gizmo gizmo;
	voe_3d_gizmo_mesh plain;
	voe_3d_gizmo_mesh marked;

	VOE_BASE_ASSERT(device != NULL, "drawing a gizmo to no device");
	VOE_BASE_ASSERT(arena != NULL, "a gizmo with no arena");

	if (!voe_ecs_entity_alive(world, frame.gizmo.entity))
		return;
	// An entity with nowhere to be has nowhere to stand a gizmo, which is
	// skipped rather than guessed at — the same rule a mesh with no
	// transform is drawn by. It stands at the world place (0281).
	if (voe_scene_transform_get(world, frame.gizmo.entity) == NULL)
		return;
	transform = voe_scene_transform_world(world, frame.gizmo.entity);

	voe_render_frame_clear_depth(device);
	gizmo = voe_3d_gizmo_at(transform.position, frame.view, frame.eye,
				frame.gizmo.size, frame.gizmo.pixels);
	// Arrows or rings (0274): the same gizmo, the same two draws.
	if (frame.gizmo.rings ?
		    !voe_3d_gizmo_rings_quads(gizmo, frame.gizmo.marked, arena,
					      &plain, &marked) :
		    !voe_3d_gizmo_quads(gizmo, frame.gizmo.marked, arena, &plain,
					&marked))
		return;
	draw_gizmo_mesh(device, plain, frame.gizmo.material, frame.gizmo.colour);
	draw_gizmo_mesh(device, marked, frame.gizmo.material,
			frame.gizmo.marked_colour);
}
