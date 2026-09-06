// The grid arithmetic and the loop that uploads it. See sheet.h for what a sheet
// is and what it deliberately is not.
//
// THE DIVISION IS THE WHOLE OF IT, and it is written out rather than folded into
// the loop below so that the one thing worth checking can be checked without a
// graphics card — see tests/sheet.c. A cell is 1/columns wide and 1/rows high,
// and the frame's own cell is the column it is in times the width and the row it
// is in times the height.
#include <sprite/sheet.h>

#include <base/assert.h>

void voe_sprite_sheet_frame(voe_sprite_sheet sheet, uint32_t frame,
			    voe_3d_material *material)
{
	uint32_t column;
	uint32_t row;

	VOE_BASE_ASSERT(material != NULL, "framing nothing as a material");
	VOE_BASE_ASSERT(sheet.columns > 0 && sheet.rows > 0,
			"a sprite sheet with no cells in it");
	VOE_BASE_ASSERT(sheet.frames <= sheet.columns * sheet.rows,
			"a sprite sheet claiming more frames than it has cells");
	VOE_BASE_ASSERT(frame < sheet.frames,
			"asking a sprite sheet for a frame it does not hold");

	column = frame % sheet.columns;
	row = frame / sheet.columns;

	material->base_colour_uv_scale = (voe_math_float2){
		1.0f / (float)sheet.columns,
		1.0f / (float)sheet.rows,
	};
	material->base_colour_uv_offset = (voe_math_float2){
		(float)column * material->base_colour_uv_scale.x,
		(float)row * material->base_colour_uv_scale.y,
	};
}

bool voe_sprite_sheet_upload(voe_render_device *device, voe_sprite_sheet sheet,
			     voe_3d_material material, voe_3d_material *out,
			     voe_base_error *error)
{
	VOE_BASE_ASSERT(device != NULL, "uploading a sprite sheet to no device");
	VOE_BASE_ASSERT(out != NULL, "uploading a sprite sheet into nothing");

	for (uint32_t frame = 0; frame < sheet.frames; frame++) {
		// The template, copied, then pointed at its own cell. Whatever
		// the caller said about colour, alpha mode, lighting and the
		// texture is every frame's; the rectangle is the frame's.
		out[frame] = material;
		voe_sprite_sheet_frame(sheet, frame, &out[frame]);
		if (!voe_3d_material_upload(device, &out[frame], error))
			return false;
	}
	return true;
}
