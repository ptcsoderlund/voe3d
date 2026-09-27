// The letters on the element path: a glyph record reading a field sheet, drawn
// in the same draw command as the rectangles around it. Nine claims, each one
// a wrong implementation would get wrong quietly:
//
// - a solid and a glyph are one draw command;
// - a glyph reads the sheet, and the right way round in both axes;
// - a glyph is clipped exactly as a solid is;
// - a glyph naming no sheet draws a solid rectangle, on purpose;
// - paint order holds across the two kinds, both ways round;
// - a stroke thinner than a pixel keeps a pixel of ink (ADR-0183, ADR-0269);
// - a stroke whose edges lie on pixel boundaries stays sharp (ADR-0269);
// - a stroke already wider than a pixel keeps its width (ADR-0212, ADR-0269);
// - an edge between pixel centres is partly covered (ADR-0269).
//
// IT READS THE PICTURE BACK, through tests/element_scene.h, because a sheet
// read the wrong way round or a coverage a few per cent off leaves the
// bookkeeping perfect. The stroke claims measure ink, not rows: coverage() turns
// a read byte back into how much of the pixel the letter covers.
//
// THE STROKE SHEETS ARE WRITTEN AS text/src/raster.c WRITES AN ATLAS — 0.5 plus
// the signed distance over twice the spread, rounded to a byte — because the
// aligned stroke's promise is decided within a few per cent of coverage, and a
// hand-picked rounding could keep it falsely. This folder does not depend on
// `text`, so the arithmetic is written out in bar_sheet() rather than called.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "element_scene.h"

#include <assert.h>
#include <math.h>

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
// white pixel, so such a record medians to white, is fully covered and comes
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
// byte — so that the coverage meets the numbers a real atlas hands it. Uploaded
// DATA and FIELD as the corner sheet is; the caller destroys it.
//
// AND THE COVERAGE IS HELD TO ITS PROMISES WITH BAR SHEETS OF THEIR OWN
// (ADR-0183, ADR-0212, ADR-0269): a stroke half a pixel tall lying across a pixel
// boundary keeps a pixel of ink in every column, a stroke whose edges lie on
// pixel boundaries stays sharp, a stroke already wider than a pixel keeps its
// width wherever it falls, and an edge between pixel centres is partly covered.
// Each bar's field is rounded to bytes as text/src/raster.c writes an atlas,
// because the sharp stroke's promise is decided within a few per cent of
// coverage and a hand-picked rounding could keep it falsely.
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

// The clear's green, in linear light: CLEAR_GREEN in render/src/pass.c, copied
// because a test reads the picture rather than the engine's private numbers.
#define PAPER_GREEN 0.00857f

// How much of pixel (x, y) a GREEN glyph covers, from the byte read back. The
// headless target is B8G8R8A8_SRGB (element_scene.h), so the green byte is
// decoded out of sRGB first; the premultiplied blend over the clear then makes
// linear green paper + coverage * (1 - paper), which is undone here.
static float coverage(const unsigned char *image, int x, int y)
{
	float encoded = image[(y * SIDE + x) * 4 + 1] / 255.0f;
	float linear = encoded <= 0.04045f ?
		encoded / 12.92f : powf((encoded + 0.055f) / 1.055f, 2.4f);

	assert(x >= 0 && x < SIDE && y >= 0 && y < SIDE);
	return (linear - PAPER_GREEN) / (1.0f - PAPER_GREEN);
}

// The ink in column x: the coverage of every row summed, in pixels.
static float column_ink(const unsigned char *image, int x)
{
	float ink = 0.0f;

	assert(x >= 0 && x < SIDE);
	for (int y = 0; y < SIDE; y++)
		ink += coverage(image, x, y);
	return ink;
}

// A stroke thinner than a pixel, lying across a pixel boundary, keeps its ink
// (ADR-0183, ADR-0212, ADR-0269). A bar two texels tall at four texels to a pixel
// is half a pixel tall, and its centre sits exactly on the line between rows 7
// and 8, so the nearest pixel centres are a quarter of a pixel outside it on both
// sides: with no dilation each would be barely covered and the limb would fade
// to grey. The edge moves out for a thin stroke, so each of those two rows is
// about 0.625 covered and every column holds at least one pixel of ink.
static void thin_stroke_keeps_its_ink(struct scene *scene)
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
		VOE_TEST_CHECK(column_ink(scene->pixels, x) >= 1.0f);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, bar));
}

// A stroke whose edges lie on pixel boundaries stays sharp (ADR-0269): eight
// texels tall at one texel a pixel, rows 4 to 11. At one texel a pixel the edge
// is the outline and the ramp is one pixel wide, so the pixel centres half a
// pixel inside read about 0.99 covered and the ones half a pixel outside about
// 0.01 — large text as sharp as under a hard cut.
static void aligned_stroke_stays_sharp(struct scene *scene)
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

	for (int y = 4; y < 12; y++)
		VOE_TEST_CHECK(coverage(scene->pixels, HALF, y) >= 0.95f);
	VOE_TEST_CHECK(coverage(scene->pixels, HALF, 3) <= 0.05f);
	VOE_TEST_CHECK(coverage(scene->pixels, HALF, 12) <= 0.05f);
	VOE_TEST_CHECK(voe_render_texture_destroy(device, bar));
}

// The wide bar of the next two claims: four texels tall at one texel a pixel,
// its centre a quarter of a pixel above a pixel centre, so the true outline
// lands between pixel centres and nothing on the grid decides it. Drawn over
// the whole surface; the caller reads scene->pixels.
static void draw_wide_bar(struct scene *scene)
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
	VOE_TEST_CHECK(voe_render_texture_destroy(device, bar));
}

// A stroke already wider than a pixel keeps its width (ADR-0212, ADR-0269): at
// one texel a pixel the edge is the outline, so the wide bar's four pixels of
// ink come out as four — about 0.25, 1, 1, 1 and 0.75 down a column. A dilation
// spent where it was not needed would add ink, and a ramp off its centre would
// add or lose some.
static void a_wide_stroke_keeps_its_width(struct scene *scene)
{
	draw_wide_bar(scene);
	for (int x = 0; x < SIDE; x++) {
		float ink = column_ink(scene->pixels, x);

		VOE_TEST_CHECK(ink >= 3.85f && ink <= 4.15f);
	}
}

// An edge between pixel centres is partly covered (ADR-0269): in the wide bar
// the outline lies a quarter of a pixel from the nearest centres, so some row
// reads strictly between 0.2 and 0.8. A hard cut reads only nought or one, which
// is the grain small text had.
static void an_edge_between_pixel_centres_is_partly_covered(struct scene *scene)
{
	bool partly = false;

	draw_wide_bar(scene);
	for (int y = 0; y < SIDE; y++) {
		float covered = coverage(scene->pixels, HALF, y);

		partly = partly || (covered > 0.2f && covered < 0.8f);
	}
	VOE_TEST_CHECK(partly);
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
		thin_stroke_keeps_its_ink(&scene);
		aligned_stroke_stays_sharp(&scene);
		a_wide_stroke_keeps_its_width(&scene);
		an_edge_between_pixel_centres_is_partly_covered(&scene);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	close_scene(&scene);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
