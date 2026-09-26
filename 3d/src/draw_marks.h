// What a pass draws on top of the world for an editor: a scene camera's marker,
// the outline of one entity, a collider's lines and the move gizmo. Internal
// to 3d: voe_3d_draw_system_run calls each once, in this order, after the world.
//
//     voe_3d_draw_marks_camera(world, device, arena, frame);
//     ... the world's blended group, the overlay ...
//     cleared = voe_3d_draw_marks_outline(world, device, arena, frame, cleared);
//     voe_3d_draw_marks_collider(world, device, arena, frame, cleared);
//     voe_3d_draw_marks_gizmo(world, device, arena, frame);
//
// THEY COME AFTER THE WORLD AND EACH BEHIND ITS OWN DEPTH, and draw_system.h
// says why: the camera marker is solid in the world's depth (0223), the outline
// is drawn against an emptied depth buffer so it shows through (ADR-0203), the
// collider's lines against the same one (0253), and the gizmo against a second
// one so nothing, not even the outline, is in front of it (ADR-0205). `cleared` says whether the overlay has already emptied the
// buffer, so the outline's clear is made once either way; the outline hands back
// whether the buffer is emptied now, so the collider's lines share that clear.
//
// Each reads its own record off the frame; a zeroed record, a dead entity or a
// missing transform draws nothing. Their quads are this frame's transient
// geometry and their scratch the caller's arena, which the caller rewinds; a
// refused range draws nothing, and render has said so on stderr.
#pragma once

#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stdbool.h>

void voe_3d_draw_marks_camera(const voe_ecs_world *world,
			      voe_render_device *device, voe_base_arena *arena,
			      voe_3d_frame frame);
bool voe_3d_draw_marks_outline(const voe_ecs_world *world,
			       voe_render_device *device, voe_base_arena *arena,
			       voe_3d_frame frame, bool cleared);
void voe_3d_draw_marks_collider(const voe_ecs_world *world,
				voe_render_device *device, voe_base_arena *arena,
				voe_3d_frame frame, bool cleared);
void voe_3d_draw_marks_gizmo(const voe_ecs_world *world,
			     voe_render_device *device, voe_base_arena *arena,
			     voe_3d_frame frame);
