// The one quad every sprite in this engine is drawn on: a square metre in the XY
// plane, centred on its own origin, with UVs running 0..1 from its top-left
// corner.
//
// ONE GEOMETRY FOR EVERY SPRITE, AND THAT IS THE WHOLE POINT. A geometry cannot
// change after it is created, so nothing about a sprite may live in its mesh —
// which frame of a sheet it shows is its material's UV rect and where it is and
// how big it is is its transform. Create this once at startup and give the id to
// every sprite entity in the world.
//
// IT IS EIGHT VERTICES AND NOT FOUR, BECAUSE THE ENGINE CULLS BACK FACES. A
// single-sided square is invisible from behind, and a sprite is a flat thing in
// a world a camera walks around: a developer who rotates it to face the camera
// never notices, and one who stands it on the ground and orbits watches it
// vanish for half the lap. So it is two squares in the same place, wound
// opposite ways with opposite normals. Exactly one survives culling from any
// given side, so nothing is drawn twice and a see-through sprite never blends
// over itself.
//
// THE BACK FACE SHOWS THE PICTURE MIRRORED, which is what a cardboard cutout
// does and is deliberate. The same texture coordinates are wound the other way
// round rather than being turned over.
//
// THE ENGINE DOES NOT BILLBOARD IT (ADR-0080). A sprite is a quad with an
// ordinary transform, and Z decides overlap like it does for everything else.
// Turning one to face the camera is a rotation built from the camera's
// transform at the call site, in game code — there is no facing
// flag here and there is not going to be one. dev/src/sprites.c shows both of
// the rotations somebody would want.
#pragma once

#include <base/error.h>
#include <render/device.h>

// Uploads the quad to the device's pools and hands back the id that names it.
// Costs eight vertices and twelve indices, once.
//
// False when the pools are full, which is the one way this fails.
[[nodiscard]] bool voe_sprite_quad_create(voe_render_device *device,
					  voe_render_geometry *out,
					  voe_base_error *error);
