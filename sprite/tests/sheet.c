// The grid arithmetic: where each cell of a sheet is, and that framing a
// material changes nothing else about it.
//
// FOUR ACROSS AND TWO DOWN, AND NOT A SQUARE, because a square sheet passes with
// the column and the row swapped and this one does not. The four corners are
// what is checked — frame 0, the end of the first row, the start of the second
// and the last — because a wrong division is a wrong answer at an edge before it
// is a wrong answer in the middle.
//
// AND ONE SHEET WHOSE LAST ROW IS SHORT, because `frames` is not columns * rows
// and a reader could reasonably think it was.
//
// Needs no graphics card: a material is a plain struct and framing one is
// arithmetic. What the GPU does with the rectangle is dev's to look at.
#include <sprite/sheet.h>

#include <testing/test.h>

#define TOLERANCE 1e-6f

// A material with something in every field the framing must not touch, so that
// "changes nothing else" is a claim with evidence rather than an assumption.
static voe_3d_material a_material(void)
{
	voe_3d_material material = {
		.base_colour = { 0.25f, 0.5f, 0.75f, 1.0f },
		.metallic = 0.125f,
		.roughness = 0.875f,
		.alpha_mode = VOE_RENDER_ALPHA_CUTOUT,
		.alpha_cutoff = 0.5f,
		.unlit = true,
	};

	return material;
}

static void check_frame(voe_sprite_sheet sheet, uint32_t frame,
			float offset_x, float offset_y, float scale_x,
			float scale_y)
{
	voe_3d_material material = a_material();

	voe_sprite_sheet_frame(sheet, frame, &material);

	VOE_TEST_CHECK_FLOAT(material.base_colour_uv_offset.x, offset_x,
			     TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material.base_colour_uv_offset.y, offset_y,
			     TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material.base_colour_uv_scale.x, scale_x,
			     TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material.base_colour_uv_scale.y, scale_y,
			     TOLERANCE);
}

int main(void)
{
	voe_sprite_sheet sheet = { .columns = 4, .rows = 2, .frames = 8 };
	voe_sprite_sheet short_row = { .columns = 4, .rows = 2, .frames = 6 };
	voe_3d_material material = a_material();

	// Left to right, then top to bottom, with (0, 0) the top-left of the
	// picture. A quarter wide and a half high, everywhere.
	check_frame(sheet, 0, 0.0f, 0.0f, 0.25f, 0.5f);
	check_frame(sheet, 3, 0.75f, 0.0f, 0.25f, 0.5f);
	check_frame(sheet, 4, 0.0f, 0.5f, 0.25f, 0.5f);
	check_frame(sheet, 7, 0.75f, 0.5f, 0.25f, 0.5f);

	// A short last row changes where the frames stop, not where they are:
	// the cells are the grid's and the fifth frame is still the first of the
	// second row.
	check_frame(short_row, 4, 0.0f, 0.5f, 0.25f, 0.5f);
	check_frame(short_row, 5, 0.25f, 0.5f, 0.25f, 0.5f);

	// The rest of the material is the caller's and stays exactly as it was.
	voe_sprite_sheet_frame(sheet, 5, &material);
	VOE_TEST_CHECK_FLOAT(material.base_colour.x, 0.25f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material.base_colour.w, 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material.metallic, 0.125f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(material.roughness, 0.875f, TOLERANCE);
	VOE_TEST_CHECK_INT(material.alpha_mode, VOE_RENDER_ALPHA_CUTOUT);
	VOE_TEST_CHECK_FLOAT(material.alpha_cutoff, 0.5f, TOLERANCE);
	VOE_TEST_CHECK(material.unlit);

	return voe_test_result();
}
