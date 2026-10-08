// The landscape cook: that the array is named and sized as
// authoring/landscape_cook.h promises, and that each height comes out as the
// whole millimetres the file would hold.
#include <authoring/landscape_cook.h>

#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

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

int main(void)
{
	test_cook_names_the_array_and_its_length();
	test_cook_writes_whole_millimetres();
	return voe_test_result();
}
