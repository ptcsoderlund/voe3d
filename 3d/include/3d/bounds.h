// How big an entity and everything under it is in the world, and how far an
// eye stands to frame that size (0371 points 1 and 2).
//
//     voe_math_double3 centre;
//     float radius;
//     if (voe_3d_bounds(world, &geometries, models, entity, &centre, &radius))
//             distance = voe_3d_bounds_distance(radius, fov_y, aspect,
//                                               2.0f / 3.0f);
//
// IT IS THIS FOLDER'S QUESTION AND NOT THE EDITOR'S, for the reason pick.h gives
// for the ray: the shape table, the model store and the shapes' own triangles
// are here, and the transforms are a dependency this folder already has. The
// editor names what it wants framed and calls this.
//
// WHAT HAS A SIZE: shapes, by their kind's triangles, and models, by their
// loaded entry's (3d/shape_geometry.h, 3d/models.h), a landscape by its heights'
// box (3d/landscape.h), on the entity and on every
// entity under it (scene/parent_component.h). Water, particles, markers and
// colliders have none; a row with no transform is nowhere and has none either.
// A NULL store, a model with no loaded entry, a kind with no geometry and a
// table the world never registered count nothing, as in pick.c.
//
// THE BOX IS AXIS-ALIGNED IN THE WORLD AND HANDED BACK AS A SPHERE: its middle
// and half its diagonal. A sphere looks the same from every side, so the
// distance that frames it does not depend on the angle a view keeps.
//
// ADR-0250: every vertex is placed by its entity's world matrix about one double
// origin, the first counted entity's world position, and the box is kept in
// float about it; only the centre goes back to double. A tree 100 km out keeps
// its millimetres as long as it is not itself kilometres across.
//
// IT WALKS EVERY SHAPE AND MODEL ROW AND EVERY VERTEX OF EACH, once per call.
// Framing is a key press, not a per-frame question; a caller asking every frame
// is what would want the box cached per entry.
#pragma once

#include <3d/models.h>
#include <3d/shape_geometry.h>

#include <ecs/world.h>

#include <math/double3.h>

#include <stdbool.h>

// The sphere round every shape's and loaded model's vertices on `root` and
// under it, each through its world transform. False, `centre` and `radius`
// untouched, when nothing counted. `models` may be NULL.
[[nodiscard]] bool voe_3d_bounds(const voe_ecs_world *world,
				 const voe_3d_shape_geometries *geometries,
				 const voe_3d_models *models, voe_ecs_entity root,
				 voe_math_double3 *centre, float *radius);

// How far from the sphere's centre an eye stands for a sphere of `radius` to
// span `fill` of the narrower of a picture's height and width, seen through a
// vertical field of view `fov_y` at width over height `aspect`. The half angle
// is the smaller of fov_y / 2 and atan(aspect · tan(fov_y / 2)); the distance is
// radius / sin(atan(fill · tan h)).
float voe_3d_bounds_distance(float radius, float fov_y, float aspect,
			     float fill);
