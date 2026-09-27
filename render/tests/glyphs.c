// The letters on the element path: a glyph record reading a field sheet, drawn
// in the same draw command as the rectangles around it. Eight claims, each one
// a wrong implementation would get wrong quietly:
//
// - a solid and a glyph are one draw command;
// - a glyph reads the sheet, and the right way round in both axes;
// - a glyph is clipped exactly as a solid is;
// - a glyph naming no sheet draws a solid rectangle, on purpose;
// - paint order holds across the two kinds, both ways round;
// - a stroke thinner than a pixel keeps a pixel (ADR-0183);
// - a stroke whose edges lie on pixel boundaries keeps its width (ADR-0184);
// - a stroke already wider than a pixel gains no row (ADR-0212).
//
// IT READS THE PICTURE BACK, through tests/element_scene.h, because a sheet
// read the wrong way round or a cutoff one byte off leaves the bookkeeping
// perfect.
//
// THE STROKE SHEETS ARE WRITTEN AS text/src/raster.c WRITES AN ATLAS — 0.5 plus
// the signed distance over twice the spread, rounded to a byte — because the
// aligned stroke's promise is decided within a byte of the cutoff, and a
// hand-picked rounding could keep it falsely. This folder does not depend on
// `text`, so the arithmetic is written out in bar_sheet() rather than called.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "element_scene.h"

#include <assert.h>

// One element room more than the busiest claim here submits, which is two;
// the same room elements.c opens with, and no mesh at all.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 2,
	.shadings = 2,
	.elements = 4,
	.passes = 1,
};

// THE CARD'S HEADLINE CLAIM: a rectangle and a letter are the same draw command.
// A solid in the top-left quadrant and a glyph in the bottom-right, and the
// frame holds exactly one draw. The picture is checked as well, because a draw
// count of one over a frame that drew nothing would also be one.
//
// A LETTER AND A FILL ARE ONE DRAW COMMAND, WHICH IS WHAT THE GLYPH KIND EXISTS
// TO SAY. A solid and a glyph in one frame, and the frame holds one draw. That
// is the headline claim and it is the assert, not the picture, that proves it —
// two kinds drawn correctly in two draws would look identical.
static void a_solid_and_a_glyph_are_one_draw(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HALF, RED)));
	// A piece of the sheet that is all interior, so this glyph is a full
	// quadrant of the record's colour and the count is exact.
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(HALF, HALF, HALF, HALF, GREEN,
			      scene->sheet.index, SHEET_INSIDE)));

	VOE_TEST_CHECK(draw_everything(device));
	// ONE. Two kinds, one buffer, one draw.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	VOE_TEST_CHECK(close_frame(device));

	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	VOE_TEST_CHECK_INT(count_in(image, 0, 0, HALF, HALF, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, HALF, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
	// And neither kind wrote anywhere but its own quadrant.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
}

// The interior of the shape is the record's colour and the paper outside it is
// not. One glyph over the whole surface reading the whole sheet, so the picture
// is the sheet's own corner pattern: the bottom-right quarter drawn and the
// other three not.
//
// IT IS ALSO THE ONE TEST THAT SAYS WHICH WAY ROUND THE SHEET RECTANGLE GOES.
// Read mirrored in u the drawn quarter is on the left; read flipped in v it is
// at the top. Both fail here and neither would fail on a stripe.
//
// THE COUNTS SKIP THE BAND WHERE THE FIELD CROSSES. FIELD sampling is linear by
// design (see voe_render_sampling), so between the outside texels and the
// inside ones there is a strip where the interpolated value is passing through
// a half and which side of the threshold a pixel lands on is arithmetic on a
// texel centre rather than a claim worth making. The regions counted are well
// inside and well outside it, which is where the answer is exact.
static void a_glyph_reads_the_sheet(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;
	// Well inside the sheet's inner quarter and well outside it, skipping
	// the band where the linear field crosses the threshold. See above.
	const int low = 6;
	const int high = 10;
	const int corner = low * low;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, GREEN, scene->sheet.index,
			      SHEET_WHOLE)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	// The interior: the record's colour, and every pixel of it.
	VOE_TEST_CHECK_INT(count_in(image, high, high, SIDE, SIDE, IS_GREEN),
			   corner);
	// The paper: three corners of it, none of them written at all. A
	// threshold that let the outside through would fill these.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, low, low, NEITHER), corner);
	VOE_TEST_CHECK_INT(count_in(image, high, 0, SIDE, low, NEITHER),
			   corner);
	VOE_TEST_CHECK_INT(count_in(image, 0, high, low, SIDE, NEITHER),
			   corner);
}

// A glyph is clipped exactly as a solid is, and the same way round: an element
// over the whole surface, drawing everywhere the sheet is concerned, clipped to
// its top half. This is what a scroll area will be made of.
static void a_glyph_is_clipped_like_a_solid(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;
	voe_render_element element = glyph(0, 0, SIDE, SIDE, GREEN,
					   scene->sheet.index, SHEET_INSIDE);

	element.clip = (voe_math_float4){ 0.0f, 0.0f, SIDE, HALF };

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(device, element));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, HALF, IS_GREEN),
			   SIDE * HALF);
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, SIDE, SIDE, NEITHER),
			   SIDE * HALF);
}

// A glyph naming no sheet. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one
// white pixel: the median of white is white, the threshold passes it, and the
// element comes out as a solid rectangle of its own colour.
//
// THIS TEST IS HERE BECAUSE THAT FAILURE LOOKS LIKE SUCCESS. A record that
// forgot its texture index draws a plausible rectangle rather than anything
// wrong, so what the empty id does is written down in voe_render_element and
// asserted here, and changing it means changing this line on purpose.
//
// AND A GLYPH THAT NAMED NO SHEET DRAWS A SOLID RECTANGLE, ASSERTED SO IT
// CANNOT CHANGE QUIETLY. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one
// white pixel, so such a record medians to white, thresholds to one and comes
// out as a plausible-looking rectangle rather than as anything that fails. It
// is the worst failure on this path to find by looking, so it is the one with a
// test naming it.
static void a_glyph_with_no_sheet_draws_a_solid_rectangle(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, RED, VOE_RENDER_NO_TEXTURE,
			      SHEET_WHOLE)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);
}

// Paint order holds across the two kinds and not only within one: a solid over
// a glyph and a glyph over a solid, and in both the second one submitted is
// what is seen. One order alone would pass on an implementation that sorted by
// kind, which is exactly the thing an interface must not have done to it.
static void paint_order_holds_across_kinds(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, GREEN, scene->sheet.index,
			      SHEET_INSIDE)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, GREEN, scene->sheet.index,
			      SHEET_INSIDE)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_GREEN),
			   SIDE * SIDE);
}

// A bar sheet: a horizontal stroke `half_width` texels either side of the
// sheet's middle row line, its field written as text/src/raster.c writes one —
// 0.5 plus the signed distance over twice the spread, clamped and rounded to a
// byte — so that the cutoff meets the numbers a real atlas hands it. Uploaded
// DATA and FIELD as the corner sheet is; the caller destroys it.
//
// AND THE CUTOFF IS HELD TO ITS THREE PROMISES WITH BAR SHEETS OF THEIR OWN
// (ADR-0183, ADR-0212): a stroke half a pixel tall lying across a pixel boundary
// keeps a pixel in every column, a stroke whose edges lie on pixel boundaries
// keeps exactly its width, and a stroke already wider than a pixel gains no row
// wherever it falls. Each bar's field is written and rounded to bytes as
// text/src/raster.c writes an atlas, because the second promise is decided
// within a byte of the cutoff and a hand-picked rounding could keep it falsely.
#define BAR_SPREAD 4.0f
#define BAR_MAX_BYTES (32 * 16 * 4)

static bool bar_sheet(voe_render_device *device, int width, int height,
		      float half_width, voe_render_texture *out)
{
	static unsigned char texels[BAR_MAX_BYTES];
	voe_base_error error = VOE_BASE_OK;
	const float centre = height / 2.0f;

	assert(width * height * 4 <= BAR_MAX_BYTES);
	assert(half_width > 0.0f);
	for (int y = 0; y < height; y++) {
		float from_centre = (y + 0.5f) - centre;
		float value = 0.5f + (half_width - (from_centre < 0.0f ?
						    -from_centre : from_centre)) /
					     (2.0f * BAR_SPREAD);

		value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
		for (int x = 0; x < width; x++) {
			unsigned char *at = texels + (y * width + x) * 4;

			at[0] = at[1] = at[2] =
				(unsigned char)(value * 255.0f + 0.5f);
			at[3] = 255;
		}
	}
	return voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
					 VOE_RENDER_SAMPLING_FIELD,
					 (uint32_t)width, (uint32_t)height,
					 texels, out, &error);
}

// A stroke thinner than a pixel, lying across a pixel boundary, keeps a pixel
// (ADR-0183). A bar two texels tall at four texels to a pixel is half a pixel
// tall, and its centre sits exactly on the line between rows 7 and 8, so the
// nearest pixel centres are a quarter of a pixel outside it on both sides: at
// the field's half nothing at all is drawn, and the letter loses the limb.
static void thin_stroke_keeps_a_pixel(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_texture bar;
	// 32 by 16 texels onto 8 by 4 pixels: four texels a pixel both ways,
	// and the sheet's middle row line lands at pixel row 6 + 2.
	const int left = 4;
	const int width = 8;

	VOE_TEST_CHECK(bar_sheet(device, 32, 16, 1.0f, &bar));
	frame = voe_render_frame_current(device);
	if (!open_frame(device)) {
		voe_render_texture_destroy(device, bar);
		return;
	}
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(left, 6, width, 4, GREEN, bar.index, SHEET_WHOLE)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);

	for (int x = left; x < left + width; x++)
		VOE_TEST_CHECK(count_in(scene->pixels, x, 0, x + 1, SIDE,
					IS_GREEN) >= 1);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, bar));
}

// A stroke whose edges lie on pixel boundaries keeps its true width (ADR-0184):
// eight texels tall at one texel a pixel, rows 4 to 11. The nearest outside
// pixel centres are exactly half a pixel out, reading about 0.439 after byte
// rounding, and the cutoff sits half a pixel out less one byte step, about
// 0.441, so they stay paper — eight rows, not nine or ten.
static void aligned_stroke_keeps_its_width(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_texture bar;

	VOE_TEST_CHECK(bar_sheet(device, 8, 16, 4.0f, &bar));
	frame = voe_render_frame_current(device);
	if (!open_frame(device)) {
		voe_render_texture_destroy(device, bar);
		return;
	}
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(4, 0, 8, SIDE, GREEN, bar.index, SHEET_WHOLE)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);

	VOE_TEST_CHECK_INT(count_in(scene->pixels, HALF, 0, HALF + 1, SIDE,
				    IS_GREEN), 8);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, HALF, 4, HALF + 1, 12,
				    IS_GREEN), 8);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, bar));
}

// A stroke already wider than a pixel gains no row (ADR-0212): four texels tall
// at one texel a pixel, its centre a quarter of a pixel above a pixel centre, so
// the true outline lands between pixel centres and nothing on the grid decides
// it. Pixel centres sit 0.25, 0.75, 1.25 and 1.75 pixels inside the outline and
// 2.25 and 2.75 outside it; filtered and rounded to bytes those read about
// 0.686, 0.655, 0.592 and 0.530 inside and 0.470 and 0.408 outside, and at one
// texel a pixel the cutoff is the half exactly — four rows. With ADR-0184's
// fixed half a pixel the cutoff was about 0.441 and the 0.470 row came in too,
// which is the fifth row this claim refuses.
static void a_wide_stroke_gains_no_row(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_texture bar;

	VOE_TEST_CHECK(bar_sheet(device, SIDE, SIDE, 2.0f, &bar));
	frame = voe_render_frame_current(device);
	if (!open_frame(device)) {
		voe_render_texture_destroy(device, bar);
		return;
	}
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, -0.25f, SIDE, SIDE, GREEN, bar.index,
			      SHEET_WHOLE)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);

	VOE_TEST_CHECK_INT(count_in(scene->pixels, HALF, 0, HALF + 1, SIDE,
				    IS_GREEN), 4);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, HALF, 6, HALF + 1, 10,
				    IS_GREEN), 4);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, bar));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	if (!open_scene(&scene, arena, CAPACITIES)) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// The sheet, uploaded exactly as text/src/font.c uploads its atlas:
	// DATA because the texels are numbers rather than colour, and FIELD
	// because a field is reconstructed by filtering and not read as a grid.
	// Neither is optional and getting either wrong changes where the edge
	// lands.
	{
		unsigned char texels[FIELD_BYTES];

		field_texels(texels);
		VOE_TEST_CHECK(voe_render_texture_create(
			scene.device, VOE_RENDER_TEXTURE_DATA,
			VOE_RENDER_SAMPLING_FIELD, FIELD_SIDE, FIELD_SIDE,
			texels, &scene.sheet, &error));
	}

	if (scene.pixels != NULL) {
		a_solid_and_a_glyph_are_one_draw(&scene);
		a_glyph_reads_the_sheet(&scene);
		a_glyph_is_clipped_like_a_solid(&scene);
		a_glyph_with_no_sheet_draws_a_solid_rectangle(&scene);
		paint_order_holds_across_kinds(&scene);
		thin_stroke_keeps_a_pixel(&scene);
		aligned_stroke_keeps_its_width(&scene);
		a_wide_stroke_gains_no_row(&scene);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	close_scene(&scene);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
