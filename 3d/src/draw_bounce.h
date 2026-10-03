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
// in the light.
//
// A STALE SPHERE IS WHERE A CASTER WAS AND IS. A caster whose transform at lag 1
// (the previous table's) differs from lag 0 in position or rotation marks a
// sphere of VOE_3D_BOUNCE_REACH at each place, about the frame's eye; render
// captures the probes in it again. A world with no previous table, or a caster
// never remembered, blends as its current row and marks nothing.
#pragma once

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <ecs/world.h>
#include <math/float4.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// Whether a material casts: lit, and not blended (0258 point 5). draw_shadows.c.
bool voe_3d_draw_casts(const voe_3d_material *material);

// Every caster drawn into the shadow or capture pass that is open, at the
// frame's lag about its eye. False when render refuses a draw. draw_shadows.c.
bool voe_3d_draw_casters(voe_ecs_world *world, voe_render_device *device,
			 const voe_3d_frame *frame);

// Whether the world's light row casts (0324 point 4); no row never does.
// draw_shadows.c.
bool voe_3d_draw_light_casts(const voe_ecs_world *world);

// The stale spheres of this step into `spheres`, xyz about the frame's eye and
// w VOE_3D_BOUNCE_REACH, two for each caster that moved; the count, never past
// `room`. A moved caster that does not fit is left out.
uint32_t voe_3d_bounce_stale(const voe_ecs_world *world,
			     const voe_3d_frame *frame, voe_math_float4 *spheres,
			     uint32_t room);

// Fits the probe volume to the frame's eye, not its view, begins `frame->target`'s
// bounce with this step's stale spheres, the frame's light with the light row's
// `bounces` and `bounce_strength` (nought with no row) and `frame->points`;
// then opens capture passes while render opens one, drawing the casters into
// each. When the light row casts it opens the bounce shadow pass with the sun
// view of the fitted grid (voe_3d_bounce_grid_sun) and, when render opens it,
// draws the casters into it (0329); then relights. False when a pass or a draw
// is refused.
[[nodiscard]] bool voe_3d_draw_bounce(voe_ecs_world *world,
				      voe_render_device *device,
				      voe_3d_frame *frame);
