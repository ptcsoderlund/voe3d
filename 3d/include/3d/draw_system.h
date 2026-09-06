// The system that turns the tables into draws. One frame, one call: it finds the
// camera and the sun, works out the matrices, and issues one draw per mesh —
// solid ones in table order, see-through ones afterwards and furthest away
// first.
//
//     if (!voe_3d_draw_system_run(world, gpu, scratch,
//                                 voe_platform_window_size(window)))
//             break;                          // the GPU stopped answering
//
// ONE DRAW PER MESH, FROM THE CPU. No instancing, no indirect command buffer, no
// culling, and no extract step into a second layout. Each of those is a change
// to this one loop and each is a later card; what this card guarantees is that
// the data is already in the shape they need — geometry in shared pools, one
// record per object in one buffer, textures by id.
//
// TWO PASSES, AND WHICH ONE AN ENTITY IS IN IS ITS MATERIAL'S ALPHA MODE. Opaque
// and cutout go first, in table order, which is safe because the depth buffer
// resolves them per pixel. Blended goes second, sorted furthest away first,
// because blending is not commutative and the blended pipeline does not write
// depth — so the order these are issued in *is* the picture. See
// 3d/depth_sort.h for the sort and the sign it turns on, and
// render/include/render/device.h for why the depth write is off.
//
// AND TWO LAYERS, WHICH RUN ACROSS THE TWO PASSES RATHER THAN INSIDE THEM. A
// drawable says whether it is in the world or above it (3d/mesh_component.h);
// the whole world is drawn, its two passes in that order, then depth is cleared,
// then the overlay is drawn — its own two passes, in the same order, by the same
// rules. So there are four groups and one depth clear between the second and the
// third, and nothing about blending, the sort or shading differs between the
// halves. An overlay object keeps a real position in metres and is seen through
// the same camera: this layer is "always on top" and is not screen space, and no
// orthographic projection exists anywhere in this engine.
//
// THE OVERLAY STILL OCCLUDES ITSELF, WHICH IS WHY THIS IS A CLEAR AND NOT THE
// DEPTH TEST TURNED OFF. Two overlapping panels above the world have to hide
// each other the way two panels in the world do; a layer that stopped testing
// depth would draw them in table order and look correct until there were two of
// them. A depth range split was the other alternative and it spends the precision
// the reversed-depth convention exists to buy.
//
// THE LAYER SAYS WHEN, NEVER HOW. It does not imply unlit, does not imply
// blended and does not change a material — whether a surface is lit is `unlit`
// on its material, and an overlay object with a lit material is lit. Text is
// both unlit and in the overlay, and those are two decisions that happen to
// agree.
//
// THE SORT IS PER OBJECT AND NOT PER TRIANGLE. One key per entity: the
// view-space depth of its origin. Two see-through things that interpenetrate,
// and a long thin one seen end-on, come out wrong, and that is the trade taken
// rather than an oversight — per-fragment sorting and order-independent
// transparency are both refused by name.
//
// IT WANTS AN ARENA BECAUSE THE SORTS NEED SOMEWHERE TO WORK. Working memory is
// an arena passed in and there is no default one (rule 11), so the caller hands
// over scratch; this rewinds every frame to exactly what it was handed, keeps
// nothing, and a caller may pass the same arena it uses for anything else. What
// it takes is bounded by the number of meshes in the world — three groups' worth
// of it, because which group an entity is in is not known until the walk has
// finished and each of them therefore has room for all of them.
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

#include <base/arena.h>
#include <ecs/world.h>
#include <render/device.h>

// False means the device cannot draw any more and the program should stop
// asking — the same meaning `render`'s frame calls give it. A window with no
// area draws nothing and returns true.
//
// `arena` is scratch for this frame's sorts and the groups they order, and
// nothing survives the call: it is rewound to the mark this took on the way in,
// on every path out.
[[nodiscard]] bool voe_3d_draw_system_run(voe_ecs_world *world,
					  voe_render_device *device,
					  voe_base_arena *arena,
					  voe_platform_size size);
