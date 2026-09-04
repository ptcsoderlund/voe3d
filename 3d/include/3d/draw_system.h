// The system that turns the tables into draws. One frame, one call: it finds the
// camera and the sun, works out the matrices, and issues one draw per mesh in
// table order.
//
//     if (!voe_3d_draw_system_run(world, gpu, voe_platform_window_size(window)))
//             break;                          // the GPU stopped answering
//
// ONE DRAW PER MESH, FROM THE CPU, IN TABLE ORDER. No sorting, no instancing, no
// indirect command buffer, no culling, and no extract step into a second layout.
// Each of those is a change to this one loop and each is a later card; what this
// card guarantees is that the data is already in the shape they need — geometry
// in shared pools, one record per object in one buffer, textures by id.
//
// TABLE ORDER IS SAFE BECAUSE EVERYTHING HERE IS OPAQUE. The depth buffer
// resolves visibility per pixel, so the order opaque draws are issued in does
// not change the image. Only blended geometry is order-dependent, and nothing on
// this card is blended — adding transparency is its own decision and it starts
// with sorting.
//
// GROUPING IS THE ENGINE'S AND NEVER THE USER'S. There is no component, flag or
// authoring concept for putting objects into batches by hand: the engine knows
// the grouping key — the same pipeline — better than a person does, and the day
// this loop groups anything it will do it from what is already in the tables.
//
// AN ENTITY IS DRAWN WHEN IT HAS ALL THREE OF MESH, TRANSFORM AND MATERIAL. A
// mesh with no transform has nowhere to be and a mesh with no material has no
// record to shade with, so both are skipped rather than guessed at — a guessed
// material would draw with somebody else's record, which looks like a bug in the
// importer.
//
// IT NEEDS EXACTLY ONE CAMERA, AND MORE THAN ONE IS A BUG RATHER THAN A CHOICE.
// A second camera means a second target and a second frame, which is the card
// that introduces a viewport; until then a world with two of them has a mistake
// in it and this says so.
//
// AND EXACTLY ONE LIGHT, FOR A DIFFERENT REASON: THERE IS ONE SUN. This engine
// lights a frame with one directional light (render/device.h), so a second one
// in the world would be silently ignored — which is worse than being told. A
// world with none asserts as well rather than drawing everything black: a scene
// with no sun in it is a program that forgot to make one far more often than it
// is a deliberately unlit picture, and black is the one result that looks like a
// broken renderer instead of a missing line. Registering the table is part of
// building a world that can be drawn; see scene/light_system.h.
//
// EVERY DRAWN OBJECT CARRIES A NORMAL MATRIX AS WELL AS A WORLD MATRIX, and this
// loop is what computes it — one inverse per object per frame. See
// 3d/normal_matrix.h for why a normal cannot be carried by the world matrix, and
// what it looks like on screen when it is.
//
// THE WINDOW'S SIZE IS PASSED THROUGH AND IS THE ASPECT RATIO'S SOURCE. Its type
// arrives from `render`'s public header, which is the folder that speaks to the
// window's answer; `3d` does not include `platform` and does not ask a window
// anything.
#pragma once

#include <ecs/world.h>
#include <render/device.h>

// False means the device cannot draw any more and the program should stop
// asking — the same meaning `render`'s frame calls give it. A window with no
// area draws nothing and returns true.
[[nodiscard]] bool voe_3d_draw_system_run(voe_ecs_world *world,
					  voe_render_device *device,
					  voe_platform_size size);
