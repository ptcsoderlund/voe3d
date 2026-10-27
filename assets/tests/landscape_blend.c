// A kind's blend with the ground (0481 point 4): a kind with `blend0=1` and
// `blendheight0=120` reads as blending at 120 cm, and a kind with neither as
// not blending at 0; written, the text has exactly those two blend keys and
// reads back equal field by field; `blend0=2`, `blendheight0=4` and
// `blendheight0=1001` are each refused MALFORMED; and the default look, written,
// has neither key.
//
// The texts are written by hand at four cells, the smallest a file may have.
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// Two kinds; kind 0's look keys go in, then the heights follow.
static const char *const two_kinds =
	"[Landscape]\nsize=64\ncells=4\n[Foliage]\nkind0=\"a.glb\"\n"
	"kind1=\"b.glb\"\n%s\n[Heights]\nrow0=0 0 0 0 0\nrow1=0 0 0 0 0\n"
	"row2=0 0 0 0 0\nrow3=0 0 0 0 0\nrow4=0 0 0 0 0\n";

static bool read_kinds(voe_base_arena *arena, const char *look_keys,
		       voe_assets_landscape *out, voe_base_error *error)
{
	char text[512];
	const int length = snprintf(text, sizeof(text), two_kinds, look_keys);

	VOE_TEST_CHECK(length > 0 && (size_t)length < sizeof(text));
	return voe_assets_landscape_read(text, (size_t)length, arena, out,
					 error);
}

static bool same_look(voe_assets_landscape_look a, voe_assets_landscape_look b)
{
	return a.size == b.size && a.size_vary == b.size_vary &&
	       a.turn_vary == b.turn_vary && a.tilt == b.tilt &&
	       a.shadowless == b.shadowless && a.blends == b.blends &&
	       a.blend_height == b.blend_height;
}

// How many times `needle` is in `text`.
static size_t occurrences(const char *text, const char *needle)
{
	size_t count = 0;

	for (const char *at = strstr(text, needle); at != NULL;
	     at = strstr(at + 1, needle))
		count++;
	return count;
}

static void a_blending_kind_round_trips(voe_base_arena *arena)
{
	voe_assets_landscape out;
	voe_assets_landscape back;
	voe_assets_landscape_text written;

	VOE_TEST_CHECK(read_kinds(arena, "blend0=1\nblendheight0=120\n", &out,
				  NULL));
	VOE_TEST_CHECK_INT(out.kind_count, 2);
	VOE_TEST_CHECK(out.looks[0].blends);
	VOE_TEST_CHECK_INT(out.looks[0].blend_height, 120);
	VOE_TEST_CHECK(!out.looks[1].blends);
	VOE_TEST_CHECK_INT(out.looks[1].blend_height, 0);

	written = voe_assets_landscape_write(&out, arena);
	VOE_TEST_CHECK_INT(occurrences(written.text, "blend"), 2);
	VOE_TEST_CHECK_INT(occurrences(written.text, "blend0=1\n"), 1);
	VOE_TEST_CHECK_INT(occurrences(written.text, "blendheight0=120\n"), 1);
	VOE_TEST_CHECK(voe_assets_landscape_read(written.text, written.size,
						 arena, &back, NULL));
	VOE_TEST_CHECK_INT(back.kind_count, 2);
	VOE_TEST_CHECK(same_look(back.looks[0], out.looks[0]));
	VOE_TEST_CHECK(same_look(back.looks[1], out.looks[1]));
}

static void bad_blends_are_malformed(voe_base_arena *arena)
{
	const char *const bad[] = { "blend0=2\n", "blendheight0=4\n",
				    "blendheight0=1001\n" };

	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		voe_assets_landscape out = { .size = 1.0f };
		voe_base_error error = VOE_BASE_OK;

		VOE_TEST_CHECK(!read_kinds(arena, bad[i], &out, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
		VOE_TEST_CHECK(out.size == 1.0f && out.heights == NULL);
	}
}

static void the_default_look_writes_no_blend(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);
	const voe_assets_landscape_look fallback =
		voe_assets_landscape_look_default();
	voe_assets_landscape_text written;

	VOE_TEST_CHECK(!fallback.blends);
	VOE_TEST_CHECK_INT(fallback.blend_height, 0);
	memcpy(grid.kinds[0], "a.glb", sizeof("a.glb"));
	grid.kind_count = 1;
	grid.looks[0] = fallback;
	written = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(written.text, "kind0=\"a.glb\"\n") != NULL);
	VOE_TEST_CHECK(strstr(written.text, "blend") == NULL);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	a_blending_kind_round_trips(arena);
	bad_blends_are_malformed(arena);
	the_default_look_writes_no_blend(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
