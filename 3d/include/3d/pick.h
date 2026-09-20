// What is under this pixel: the ray a view's camera sends through a point of
// its picture, and the entity that ray meets first.
//
//     voe_3d_ray ray = voe_3d_pick_ray(camera, size, point);
//     float distance;
//     voe_ecs_entity hit = voe_3d_pick(world, &geometries, ray, &distance);
//     if (hit.generation != 0)
//             ... // the frontmost shaped entity the ray met
//
// IT IS THIS FOLDER'S QUESTION AND NOT THE EDITOR'S. Answering it needs the
// shape table, the transform table and the projection matrix, and this is the
// folder that holds all three; the editor is a leaf that names what it needs and
// never reaches into a folder for a gap of its own (`editor/editor.md`). A view
// that wants a click named calls this, the way it already calls
// voe_3d_draw_system_frame to draw.
//
// IT WALKS THE SHAPE TABLE ALONE, AND THAT IS THE WHOLE OF WHAT A PROJECT DRAWS
// TODAY (ADR-0202). A mesh and a material are runtime-only components
// (3d/mesh_component.h, 3d/material_component.h), so no scene file holds one and
// nothing in the editor imports a model: every entity a person can click on is a
// shaped one. The mesh table's turn comes with the card that gives the editor an
// import, against the triangles the importer read and kept the same way.
//
// HOW A HIT IS MEASURED. The entity's world matrix is inverted once and carries
// the ray into the shape's own space — the origin as a point, the direction as a
// direction — and the kind's triangles are tested there, which is where they
// are. The direction is carried over WITHOUT being normalised again, on purpose:
// the parameter along the ray is then the same number in both spaces, so the
// distances of two entities scaled differently are comparable and the one handed
// back is in metres.
//
// WHICH ENTITIES ARE SKIPPED. A shaped entity with no transform, because it is
// nowhere and the draw system does not draw it either (3d/shape_component.h); a
// kind this build does not know, which draws nothing and so picks nothing
// (3d/shape_geometry.h); and one whose world matrix has a determinant of
// nothing, which is a thing scaled away to nothing — it is drawn as nothing, its
// matrix cannot be inverted (math/float4x4.h asserts on a singular one), and
// there is no ray in its space to cast.
//
// TIES ARE BROKEN BY TABLE ORDER, AND THAT IS NOT WORTH A RULE. Two surfaces at
// exactly the same distance along the ray is two coplanar faces under one pixel;
// whichever came first in the shape table wins, and no person can tell which
// they meant.
//
// IT WALKS EVERY ROW EVERY CALL. There is no acceleration structure and none is
// wanted: a click is not a per-frame operation, and a scene with a thousand
// shapes is a few hundred thousand triangle tests once, when a person presses a
// button. A view that picked every frame — a hover highlight over a large
// project — is what would want a tree here, built from the same store.
#pragma once

#include <3d/shape_geometry.h>

#include <ecs/world.h>

#include <math/float2.h>
#include <math/float3.h>

#include <render/device.h>

#include <scene/camera_component.h>

// A ray in world space. `direction` is unit length, so a parameter along it is a
// distance in metres.
typedef struct {
	voe_math_float3 origin;
	voe_math_float3 direction;
} voe_3d_ray;

// The ray through `point` of a picture `size` pixels big drawn with `camera`.
//
// `point` is in that picture's own pixels, x right and y down from its top-left
// corner, which is the shape every pointer in this engine already has — the
// editor's main.c divides a window pointer into a panel's own space the same way
// (ADR-0141 point 4).
//
// It builds the same two matrices the pass was opened with —
// voe_3d_projection(camera, width / height) and voe_scene_camera_view(camera) —
// inverts their product once, and takes the pixel's near point (clip z 1,
// because depth runs backwards) and its far point (clip z 0) through it, each
// divided by its own w. The origin is the near point and the direction is the
// normalised difference. There is no Y negation written out anywhere in it: the
// flip is the one line that turns point.y into a clip coordinate, and the half
// pixel in it is the pixel's own centre.
//
// A size with no area, or a camera whose planes are the wrong way round, is the
// caller's bug and asserts, as voe_3d_projection's already does.
voe_3d_ray voe_3d_pick_ray(voe_scene_camera camera, voe_platform_size size,
			   voe_math_float2 point);

// The frontmost entity `ray` meets, or a zeroed entity when it meets none — and
// a zeroed entity is never a live one (ecs/world.h), which is what clears a
// selection.
//
// `distance` may be NULL; when it is not, it is how far along the ray the hit
// is, in metres from the ray's own origin — which for a pick ray is the point on
// the near plane and not the eye — and is not touched when nothing was hit.
voe_ecs_entity voe_3d_pick(const voe_ecs_world *world,
			   const voe_3d_shape_geometries *geometries,
			   voe_3d_ray ray, float *distance);
