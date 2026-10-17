// The landscape cook: that the array is named and sized as
// authoring/landscape_cook.h promises, and that each height comes out as the
// whole millimetres the file would hold; that paint cooks one plane up to four
// layers, two past, each byte as set; and that copies cook one line each, in
// order, negative x and z included.
#include <authoring/landscape_cook.h>

#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdlib.h>
#include <string.h>

static void test_cook_names_the_array_and_its_length(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_assets_landscape flat = voe_assets_landscape_flat(256.0f, 4, arena);
	voe_authoring_text out =
		voe_authoring_landscape_cook(&flat, "hill_1", arena);
	const char *head = "static const int32_t hill_1[25] = {\n\t";

	VOE_TEST_CHECK(strncmp(out.text, head, strlen(head)) == 0);
	VOE_TEST_CHECK_INT(out.size, strlen(out.text));
	VOE_TEST_CHECK(out.size > strlen(head) + 5);
	VOE_TEST_CHECK(strcmp(out.text + out.size - 5, ",\n};\n") == 0);
	voe_base_arena_destroy(arena);
}

// 0.1204 m rounds to 120, -0.0004 m to 0 (not -0), 2.5 m to 2500.
static void test_cook_writes_whole_millimetres(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_assets_landscape land = voe_assets_landscape_flat(256.0f, 4, arena);

	land.heights[0] = 0.1204f;
	land.heights[1] = -0.0004f;
	land.heights[2] = 2.5f;
	land.heights[24] = -1.2346f;
	voe_authoring_text out =
		voe_authoring_landscape_cook(&land, "hill", arena);

	VOE_TEST_CHECK(strcmp(out.text,
			      "static const int32_t hill[25] = {\n"
			      "\t120, 0, 2500, 0, 0, 0, 0, 0,\n"
			      "\t0, 0, 0, 0, 0, 0, 0, 0,\n"
			      "\t0, 0, 0, 0, 0, 0, 0, 0,\n"
			      "\t-1235,\n"
			      "};\n") == 0);
	voe_base_arena_destroy(arena);
}

// A 4-cell grid all layer 0 but point 7, painted 55 of layer 0 and 200 of
// layer 1, or `layers` layers; returns the cooked bytes' count and fills
// `bytes` with the first 200.
static size_t cook_paint(uint32_t layers, unsigned char bytes[200])
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_assets_landscape land = voe_assets_landscape_flat(256.0f, 4, arena);

	land.layer_count = layers;
	land.paint = voe_base_arena_push(arena, 8 * 25);
	memset(land.paint, 0, 8 * 25);
	for (size_t i = 0; i < 25; i++)
		land.paint[i * 4] = 255;
	land.paint[7 * 4] = 55;
	land.paint[7 * 4 + 1] = 200;
	voe_authoring_text out =
		voe_authoring_landscape_cook_paint(&land, "hill_paint", arena);
	const char *head = "static const uint8_t hill_paint[";
	size_t count = 0;

	VOE_TEST_CHECK(strncmp(out.text, head, strlen(head)) == 0);
	VOE_TEST_CHECK(strcmp(out.text + out.size - 5, ",\n};\n") == 0);
	const char *p = strchr(out.text, '{') + 1;
	char *end;
	for (unsigned long v = strtoul(p, &end, 10); end != p;
	     v = strtoul(p, &end, 10)) {
		if (count < 200)
			bytes[count] = (unsigned char)v;
		count++;
		p = end + 1;
	}
	voe_base_arena_destroy(arena);
	return count;
}

static void test_cook_paint_writes_one_plane_up_to_four_layers(void)
{
	unsigned char bytes[200] = { 0 };

	VOE_TEST_CHECK_INT(cook_paint(2, bytes), 100);
	VOE_TEST_CHECK_INT(bytes[0], 255);
	VOE_TEST_CHECK_INT(bytes[28], 55);
	VOE_TEST_CHECK_INT(bytes[29], 200);
	VOE_TEST_CHECK_INT(bytes[30], 0);
	VOE_TEST_CHECK_INT(bytes[31], 0);
	VOE_TEST_CHECK_INT(bytes[96], 255);
}

static void test_cook_paint_writes_both_planes_past_four_layers(void)
{
	unsigned char bytes[200] = { 0 };

	VOE_TEST_CHECK_INT(cook_paint(5, bytes), 200);
	VOE_TEST_CHECK_INT(bytes[29], 200);
	VOE_TEST_CHECK_INT(bytes[100], 0);
}

// Two copies, the second at negative x and z, each field in its place.
static void test_cook_copies_writes_each_copy_in_order(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_assets_landscape land = voe_assets_landscape_flat(256.0f, 4, arena);
	voe_assets_landscape_copy copies[2] = {
		{ .x = 1500, .z = 32000, .turn = 16384, .scale = 1000,
		  .kind = 0, .lean = 0 },
		{ .x = -64000, .z = -250, .turn = 65535, .scale = 1200,
		  .kind = 1, .lean = 255 },
	};

	land.copies = copies;
	land.copy_count = 2;
	voe_authoring_text out =
		voe_authoring_landscape_cook_copies(&land, "hill_copies", arena);

	VOE_TEST_CHECK(strcmp(out.text,
			      "static const voe_assets_landscape_copy "
			      "hill_copies[2] = {\n"
			      "\t{1500, 32000, 16384, 1000, 0, 0},\n"
			      "\t{-64000, -250, 65535, 1200, 1, 255},\n"
			      "};\n") == 0);
	VOE_TEST_CHECK_INT(out.size, strlen(out.text));
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_cook_names_the_array_and_its_length();
	test_cook_writes_whole_millimetres();
	test_cook_paint_writes_one_plane_up_to_four_layers();
	test_cook_paint_writes_both_planes_past_four_layers();
	test_cook_copies_writes_each_copy_in_order();
	return voe_test_result();
}
