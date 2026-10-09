// The probe bounce of one frame and the spheres of the volume that went stale
// (ADR-0326 points 2, 4 and 8). Internal to 3d: voe_3d_draw_system_shadows
// calls voe_3d_draw_bounce after its point-shadow pass when a light bounces;
// the test reads the spheres.
//
//     if (!voe_3d_draw_bounce(world, device, frame))
//             return false;                   // render said why on stderr
//
// THE CASTERS ARE THE CASCADES' AND SO IS THE WALK. draw_shadows.c owns who
// casts and how a caster is drawn; it is declared here, not copied, so the
// probes' pictures and the shadow maps can never disagree about what stands
// in the light. A capture draws every caster; a sun's map only those the
// blockers holding the sun hold, as its cascades (0361 point 2).
//
// A STALE SPHERE IS A CASTER'S OWN SIZE (0389 point 7): about the frame's eye,
// of w its world bounding radius, render adding the probes' reach. A caster
// whose transform at lag 1 (the previous table's) differs from lag 0 in
// position, rotation or scale marks one at each place. One never remembered in
// a world that has a previous table is new, and one the shape system changed
// this step recoloured; each marks one where it is. A world with no previous
// table marks only recolours. A removed caster marks one where the frame's
// memory saw it last (0394), bounce_casters.c's.
//
// THE GRID IS FITTED TO THE LEVEL, NOT THE EYE (0331): to the still casters'
// box (0332 point 1), so a camera that moves never moves it.
#pragma once

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <ecs/world.h>
#include <math/double3.h>
#include <math/float4.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// Whether a material casts: lit, and not blended (0258 point 5). draw_shadows.c.
bool voe_3d_draw_casts(const voe_3d_material *material);

// Every caster drawn into the shadow or capture pass that is open, at the
// frame's lag about its eye. With `within` not 0, only a caster whose world
// matrix's origin, about the eye at the lag as a light's place is, has a mask
// over `frame->blockers`' kept records holding every bit of `within` (0361
// point 2); `within` 0 draws every caster and takes no mask. A landscape
// caster draws at most `landscape_nodes` nodes, the cast's capacity (0397).
// False when render refuses a draw. draw_shadows.c.
bool voe_3d_draw_casters(voe_ecs_world *world, voe_render_device *device,
			 const voe_3d_frame *frame, uint32_t within,
			 uint32_t landscape_nodes);

// Whether light row `light`, in table order, casts (0324 point 4); false past
// the count. draw_shadows.c.
bool voe_3d_draw_light_casts(const voe_ecs_world *world, uint32_t light);

// The stale spheres of this step into `spheres`, xyz about the frame's eye and
// w the caster's world bounding radius at that lag, half the diagonal of its
// world box grown as voe_3d_bounce_box grows one from `device`'s geometry
// boxes: two for each caster that moved (position by a millimetre, rotation or
// scale by 1e-4), one for each new or recoloured one; the count, never past
// `room`. A sphere that does not fit is left out, and a caster whose ids name
// no geometry marks none.
uint32_t voe_3d_bounce_stale(const voe_ecs_world *world,
			     const voe_render_device *device,
			     const voe_3d_frame *frame,
			     voe_math_float4 *spheres, uint32_t room);

// Every caster a capture draws, as voe_3d_bounce_stale marks them: world-layer
// meshes with a casting material and a transform, then, with `frame->models`,
// model rows with a transform and a casting part, the frame's `hidden` left
// out. Each goes to `visit` with `context`, its mesh's geometry, and its
// model's entry or NULL; the walk stops when `visit` returns false.
void voe_3d_bounce_walk(const voe_ecs_world *world, const voe_3d_frame *frame,
			bool (*visit)(void *context, voe_ecs_entity entity,
				      voe_render_geometry geometry,
				      const voe_3d_model_entry *model),
			void *context);

// A caster's place at `lag` into `centre`, in double about the world origin,
// and its world bounding radius there: half the diagonal of the box of
// `geometry` when `model` is NULL, else of `model`'s casting parts, a
// landscape's its pyramid's box in `models`, grown as voe_3d_bounce_box grows
// one; nought when no id names anything.
float voe_3d_bounce_caster_sphere(const voe_ecs_world *world,
				  const voe_render_device *device,
				  voe_ecs_entity entity,
				  voe_render_geometry geometry,
				  const voe_3d_models *models,
				  const voe_3d_model_entry *model, float lag,
				  voe_math_double3 *centre);

// The removed casters' spheres into `spheres` (0394): one for each caster
// `frame->casters` remembers that this walk no longer visits, at its
// remembered centre about the frame's eye, w its remembered radius; the
// count, never past `room`. Then the memory holds this frame's casters, centre
// and radius at lag 0 as voe_3d_bounce_caster_sphere gives them, those of no
// size left out. With `frame->casters` NULL, none and nothing written.
// bounce_casters.c.
uint32_t voe_3d_bounce_removed(const voe_ecs_world *world,
			       const voe_render_device *device,
			       const voe_3d_frame *frame,
			       voe_math_float4 *spheres, uint32_t room);

// The world box, in double about the world origin, of every caster drawn into
// a capture pass (meshes and the frame's model parts) that did not move this
// step, into `min` and `max`; false and nothing written when none is still. A
// mesh whose id names nothing is skipped (0332 point 1).
bool voe_3d_bounce_box(const voe_ecs_world *world,
		       const voe_render_device *device,
		       const voe_3d_frame *frame, voe_math_double3 *min,
		       voe_math_double3 *max);

// Fits the probe volume to the still casters' box (voe_3d_bounce_box), not the
// eye, begins `frame->target`'s bounce at the fitted spacing with this step's
// stale spheres and after them the removed casters' (voe_3d_bounce_removed),
// in the room left, the frame's light with light row 0's
// `bounces` and `bounce_strength` (nought with no row), the frame's further
// lights as `more` (0357 point 1) and `frame->points`; then opens capture
// passes while render opens one, drawing the casters into each. Then one bounce
// shadow pass per casting sun: for sun i whose light row i casts, the sun view
// of the fitted grid along its direction (voe_3d_bounce_grid_sun) and, when
// render opens it, the casters its blockers hold drawn into it (0329, 0357
// point 4, 0361 point 2); then
// relights. That is volume 0; then each nest finer than the level grid, coarse
// to fine, the same at voe_3d_bounce_grid_nest from voe_render_bounce_placed,
// the j-th of n volumes capturing until the call's count reaches
// VOE_RENDER_BOUNCE_CAPTURE_PASSES − (n − 1 − j) (0389 points 1 to 3); every
// one stops at VOE_3D_BOUNCE_MOVING_PASSES when `frame->eye` is more than 1 mm
// on an axis from the eye `frame->casters` remembers, which then takes
// `frame->eye` whatever the outcome (0397 point 2). False
// when a pass or a draw is refused; every volume after that one is still
// begun, opening no pass, so each queues this step's stale spheres.
[[nodiscard]] bool voe_3d_draw_bounce(voe_ecs_world *world,
				      voe_render_device *device,
				      voe_3d_frame *frame);
