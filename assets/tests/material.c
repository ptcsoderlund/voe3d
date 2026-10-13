// The material file: an empty `[Material]` section reads as the defaults,
// cut-out, sides and emission included; a full file round-trips through write
// and read field for field and written again byte for byte; and an unknown
// shader, a bad number, a cutout outside 0 to 1, a negative emission or
// strength, a `double_sided` not true or false and an overlong path are
// refused MALFORMED with `out` left as it was.
#include <assets/material.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

static bool read_text(const char *text, voe_assets_material_file *out,
		      voe_base_error *error)
{
	return voe_assets_material_read(text, strlen(text), out, error);
}

// Refused MALFORMED, and `out` left as it was.
static void malformed(const char *text)
{
	voe_assets_material_file out = { .roughness = 7.0f };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!read_text(text, &out, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	VOE_TEST_CHECK(out.roughness == 7.0f && out.repeat == 0.0f);
}

static bool same(const voe_assets_material_file *a, const voe_assets_material_file *b)
{
	return a->shader == b->shader && a->colour[0] == b->colour[0] &&
	       a->colour[1] == b->colour[1] && a->colour[2] == b->colour[2] &&
	       a->roughness == b->roughness && a->metal == b->metal &&
	       a->repeat == b->repeat &&
	       strcmp(a->colormap, b->colormap) == 0 &&
	       strcmp(a->normalmap, b->normalmap) == 0 &&
	       strcmp(a->ormmap, b->ormmap) == 0 && a->cutout == b->cutout &&
	       a->double_sided == b->double_sided &&
	       a->emission[0] == b->emission[0] &&
	       a->emission[1] == b->emission[1] &&
	       a->emission[2] == b->emission[2] &&
	       a->emission_strength == b->emission_strength &&
	       strcmp(a->emissionmap, b->emissionmap) == 0;
}

static void an_empty_section_reads_the_defaults(void)
{
	voe_assets_material_file out;
	const voe_assets_material_file expected = voe_assets_material_default();

	VOE_TEST_CHECK(read_text("[Material]\n", &out, NULL));
	VOE_TEST_CHECK(same(&out, &expected));
	VOE_TEST_CHECK_INT(out.shader, VOE_ASSETS_MATERIAL_LIT);
	VOE_TEST_CHECK(out.colour[0] == 1.0f && out.colour[1] == 1.0f &&
		       out.colour[2] == 1.0f);
	VOE_TEST_CHECK(out.roughness == 0.5f && out.metal == 0.0f &&
		       out.repeat == 1.0f);
	VOE_TEST_CHECK(out.colormap[0] == '\0' && out.normalmap[0] == '\0' &&
		       out.ormmap[0] == '\0');
	VOE_TEST_CHECK(out.cutout == 0.0f && !out.double_sided);
	VOE_TEST_CHECK(out.emission[0] == 0.0f && out.emission[1] == 0.0f &&
		       out.emission[2] == 0.0f);
	VOE_TEST_CHECK(out.emission_strength == 1.0f &&
		       out.emissionmap[0] == '\0');
	// One key given, the rest their defaults.
	VOE_TEST_CHECK(read_text("[Material]\nmetal=0.25\n", &out, NULL));
	VOE_TEST_CHECK(out.metal == 0.25f && out.roughness == 0.5f);
	VOE_TEST_CHECK(read_text("[Material]\normmap=\"a.png\"\n", &out, NULL));
	VOE_TEST_CHECK(strcmp(out.ormmap, "a.png") == 0 &&
		       out.colormap[0] == '\0');
}

static void a_full_file_round_trips(voe_base_arena *arena)
{
	voe_assets_material_file full = {
		.shader = VOE_ASSETS_MATERIAL_UNLIT,
		.colour = { 0.1f, 1.0f / 3.0f, 2.0e-7f },
		.roughness = 0.123456789f,
		.metal = 1.0f,
		.repeat = 1000.0f,
		.cutout = 0.333333343f,
		.double_sided = true,
		.emission = { 1.0f, 0.5f, 1.0e-6f },
		.emission_strength = 12.5f,
	};
	voe_assets_material_file back = voe_assets_material_default();
	voe_assets_material_text text;
	voe_assets_material_text again;

	snprintf(full.colormap, sizeof full.colormap, "%s", "Assets/brick wall.png");
	snprintf(full.normalmap, sizeof full.normalmap, "%s",
		 "Assets/\"odd\" \\ name.png");
	memset(full.ormmap, 'r', VOE_ASSETS_MATERIAL_PATH - 1);
	full.ormmap[VOE_ASSETS_MATERIAL_PATH - 1] = '\0';
	snprintf(full.emissionmap, sizeof full.emissionmap, "%s",
		 "Assets/glow.png");

	text = voe_assets_material_write(&full, arena);
	VOE_TEST_CHECK_INT(strlen(text.text), text.size);
	VOE_TEST_CHECK(strstr(text.text, "shader=unlit\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "\normmap=") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "\ndouble_sided=true\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "\nemissionmap=\"Assets/glow.png\"\n") !=
		       NULL);
	VOE_TEST_CHECK(voe_assets_material_read(text.text, text.size, &back,
						NULL));
	VOE_TEST_CHECK(same(&back, &full));
	// Written again, the same bytes.
	again = voe_assets_material_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(memcmp(again.text, text.text, text.size) == 0);

	// The defaults, too.
	full = voe_assets_material_default();
	text = voe_assets_material_write(&full, arena);
	VOE_TEST_CHECK(read_text(text.text, &back, NULL));
	VOE_TEST_CHECK(same(&back, &full));
}

static void bad_values_are_malformed(void)
{
	char text[512];

	malformed("[Material]\nshader=shiny\n");
	malformed("[Material]\nroughness=half\n");
	malformed("[Material]\nroughness=0.5x\n");
	malformed("[Material]\ncolour=1 1\n");
	malformed("[Material]\ncolour=1 1 1 1\n");
	malformed("[Material]\nrepeat=inf\n");
	malformed("[Material]\ncutout=-0.1\n");
	malformed("[Material]\ncutout=1.5\n");
	malformed("[Material]\nemission=1 -1 0\n");
	malformed("[Material]\nemission_strength=-2\n");
	malformed("[Material]\ndouble_sided=yes\n");
	malformed("[Material]\ndouble_sided=\n");
	malformed("[Other]\n");
	malformed("not sectioned\n");
	snprintf(text, sizeof(text), "[Material]\ncolormap=%0128d\n", 0);
	malformed(text);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);

	an_empty_section_reads_the_defaults();
	a_full_file_round_trips(arena);
	bad_values_are_malformed();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
