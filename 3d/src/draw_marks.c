// The editor's marks drawn over the world: a scene camera's box and frustum, a
// sun's circle and arrow, one entity's silhouette, a collider's lines and the
// gizmo's two meshes, arrows or rings, each as this frame's transient geometry
// and one or two draws, at the entity's world place. See draw_marks.h.
//
// All five are in metres about the frame's eye already (ADR-0250), so every
// object here has identity matrices; the record is the caller's unlit one and the colour the caller's.
#include "draw_marks.h"

#include <3d/camera_marker.h>
#include <3d/collider_marker.h>
#include <3d/gizmo.h>
#include <3d/gizmo_rings.h>
#include <3d/outline.h>
#include <3d/sun_marker.h>
#include <base/assert.h>
#include <base/error.h>
#include <physics/shape.h>
#include <scene/camera_component.h>
#include <scene/light_component.h>
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

// The sun's marker, drawn as the camera's is (0274): this frame's geometry and
// one draw, in world metres about the eye. A zeroed entity, a dead one, and one
// without a light or a transform draw nothing; so does a refused range.
void voe_3d_draw_marks_sun(const voe_ecs_world *world,
			   voe_render_device *device, voe_base_arena *arena,
			   voe_3d_frame frame)
{
	voe_3d_sun_marked sun = frame.sun;
	voe_scene_transform pose;
	voe_3d_outline_mesh mesh;
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing a sun marker to no device");
	VOE_BASE_ASSERT(arena != NULL, "a sun marker with no arena");

	if (!voe_ecs_entity_alive(world, sun.entity) ||
	    voe_scene_light_get(world, sun.entity) == NULL)
		return;
	if (voe_scene_transform_get(world, sun.entity) == NULL)
		return;
	pose = voe_scene_transform_world(world, sun.entity);
	if (voe_3d_sun_marker_quads(pose, frame.view, frame.eye, sun.size,
				    sun.pixels, arena, &mesh) &&
	    voe_render_geometry_create_transient(device, mesh.vertices,
						 mesh.vertex_count, mesh.indices,
						 mesh.index_count, &quads,
						 &error))
		(void)voe_render_frame_draw(device, quads,
					    mark_object(sun.material,
							sun.colour));
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

// THE COLLIDER'S LINES SHARE THE OUTLINE'S CLEAR AND ITS LOOK (0253). They are
// drawn against the depth the outline was, clearing it only when nothing has,
// so they show through what stands in front, in the outline's material and
// colour. A zeroed entity, a dead one and one with no collider draw nothing;
// so does a refused transient range, which render reports.
void voe_3d_draw_marks_collider(const voe_ecs_world *world,
				voe_render_device *device, voe_base_arena *arena,
				voe_3d_frame frame, bool cleared)
{
	voe_physics_shape shape;
	voe_3d_outline_mesh mesh;
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(device != NULL, "drawing a collider to no device");
	VOE_BASE_ASSERT(arena != NULL, "a collider's lines with no arena");

	if (!voe_ecs_entity_alive(world, frame.collider.entity) ||
	    !voe_physics_shape_of(world, frame.collider.entity, &shape))
		return;
	if (!voe_3d_collider_marker_quads(shape, frame.view, frame.eye,
					  frame.collider.size,
					  frame.collider.pixels, arena, &mesh) ||
	    !voe_render_geometry_create_transient(
		    device, mesh.vertices, mesh.vertex_count, mesh.indices,
		    mesh.index_count, &quads, &error))
		return;

	if (!cleared)
		voe_render_frame_clear_depth(device);
	(void)voe_render_frame_draw(device, quads,
				    mark_object(frame.outlined.material,
						frame.outlined.colour));
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
