// A sprite sheet as a grid of equal cells, and the materials that read one cell
// each.
//
// A SHEET IS ONE TEXTURE, ONE GEOMETRY AND ONE MATERIAL PER FRAME. The frames
// differ in nothing but which rectangle of the picture they read, and that is a
// material's `base_colour_uv_offset` and `base_colour_uv_scale`. Changing frame
// is giving an entity a different material — the shading index is submitted
// every frame already, so it costs nothing and re-uploads nothing.
//
// IT IS NOT WHERE FRAMES ADVANCE OVER TIME. Nothing here holds a clock or a
// current frame. What a sheet is for is having the frames; playing them is the
// program's, until a card says otherwise.
//
// THE CELLS ARE ORDERED LEFT TO RIGHT AND THEN TOP TO BOTTOM, which is how a
// sheet is laid out by every tool that makes one and is the same direction the
// texture coordinates run: (0, 0) is the top-left of the picture.
//
// THE SHEET HAS TO HAVE BEEN CREATED SHARP, and nothing here can check it. A
// texture addressed REPEAT lets a coordinate a hair outside one cell wrap to the
// far side of the sheet and fetch a different sprite entirely; a sheet is not
// tiled, so it is VOE_RENDER_SAMPLING_SHARP. See voe_render_sampling.
//
// AND IT IS A PICTURE, SO IT IS VOE_RENDER_TEXTURE_COLOUR and its material's
// `base_colour_distance_field` is false. The distance-field path belongs to the
// glyph sheet and to nothing else.
#pragma once

#include <3d/material_component.h>
#include <base/error.h>
#include <render/device.h>

#include <stdint.h>

// The grid, and how many of its cells hold a sprite.
//
// `frames` IS NOT ALWAYS columns * rows. A sheet of six frames laid out four
// across is two rows with two empty cells at the end, and asking for a frame
// that is not there is a bug at the call site rather than a rectangle over
// nothing.
typedef struct {
	uint32_t columns;
	uint32_t rows;
	uint32_t frames;
} voe_sprite_sheet;

// Points a material's base colour at one cell of the sheet: its UV offset and
// scale, and nothing else about the material is touched.
//
// A frame at or past `sheet.frames`, a sheet with no columns or no rows, and a
// `frames` larger than the grid holds are all the caller's bug and all assert.
void voe_sprite_sheet_frame(voe_sprite_sheet sheet, uint32_t frame,
			    voe_3d_material *material);

// One material per frame, each a copy of `material` reading its own cell, each
// uploaded and carrying the shading id that came back. `out` is `sheet.frames`
// materials the caller owns — an array on its arena or its stack — and is
// written in frame order.
//
// A STARTUP CALL, because creating a shading record is one: it waits for the GPU
// to go idle. A sheet of many frames is many records, and a device is asked at
// creation for how many it will hold — see voe_render_capacities.
//
// False when `render` has no room for another record, which is the one way this
// fails. The materials written before the failure are left where they are;
// nothing here unwinds, because a startup that failed is a program that is about
// to stop.
[[nodiscard]] bool voe_sprite_sheet_upload(voe_render_device *device,
					   voe_sprite_sheet sheet,
					   voe_3d_material material,
					   voe_3d_material *out,
					   voe_base_error *error);
