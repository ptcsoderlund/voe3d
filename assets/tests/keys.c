// The material and parts key tables against their own readers, so a table
// cannot promise what its reader refuses (0511): every row's default, set
// under its section, is read (material by the strict reader); every material
// row marked refused is refused a value above its greatest; and every row has
// a section, key, kind and meaning. A pattern's `<…>` takes each index 0 to 9
// in turn until one reads, so it stands for the first index the reader takes.
#include <assets/material.h>
#include <assets/parts.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// The highest index tried in place of a pattern's `<…>`.
#define INDEX_TRIED 9

static bool material_reads(const char *text)
{
	voe_assets_material_file out;

	return voe_assets_material_read_strict(text, strlen(text), &out, NULL);
}

// A `[Parts]` section is required, so one comes before any other section.
static bool parts_reads(const char *text)
{
	static voe_assets_parts out;
	char whole[512];

	snprintf(whole, sizeof(whole), "%s%s",
		 strncmp(text, "[Parts]\n", 8) == 0 ? "" : "[Parts]\n", text);
	return voe_assets_parts_read(whole, strlen(whole), &out, NULL);
}

// Whether `row`'s key set to `value` under its section reads, the key's
// `<…>`, if any, taken as each index up to INDEX_TRIED until one does.
static bool row_reads(const voe_assets_documented_key *row, const char *value,
		      bool (*reads)(const char *text))
{
	const char *open = strchr(row->key, '<');
	const char *close = open == NULL ? NULL : strchr(open, '>');
	char key[64];
	char text[256];

	for (uint32_t index = 0; index <= INDEX_TRIED; index++) {
		if (close == NULL)
			snprintf(key, sizeof(key), "%s", row->key);
		else
			snprintf(key, sizeof(key), "%.*s%u%s",
				 (int)(open - row->key), row->key,
				 (unsigned)index, close + 1);
		snprintf(text, sizeof(text), "[%s]\n%s=%s\n", row->section, key,
			 value);
		if (reads(text))
			return true;
		if (close == NULL)
			return false;
	}
	return false;
}

static void check_rows(const voe_assets_documented_key *rows, uint32_t count,
		       bool (*reads)(const char *text), bool check_refused)
{
	VOE_TEST_CHECK(rows != NULL && count > 0);
	for (uint32_t i = 0; i < count; i++) {
		const voe_assets_documented_key *row = &rows[i];
		char above[32];

		VOE_TEST_CHECK(row->section != NULL && row->section[0] != '\0');
		VOE_TEST_CHECK(row->key != NULL && row->key[0] != '\0');
		VOE_TEST_CHECK(row->kind != NULL && row->kind[0] != '\0');
		VOE_TEST_CHECK(row->says != NULL && row->says[0] != '\0');
		VOE_TEST_CHECK(row->default_text != NULL && row->unit != NULL);
		VOE_TEST_CHECK(row->least <= row->greatest);
		if (row->default_text[0] != '\0' &&
		    !row_reads(row, row->default_text, reads)) {
			fprintf(stderr, "a default not read: %s=%s\n", row->key,
				row->default_text);
			VOE_TEST_CHECK(false);
		}
		if (!check_refused || !row->refused)
			continue;
		snprintf(above, sizeof(above), "%.9g", row->greatest + 1.0);
		if (row_reads(row, above, reads)) {
			fprintf(stderr, "a refused range read: %s=%s\n", row->key,
				above);
			VOE_TEST_CHECK(false);
		}
	}
}

static void material_keys_read_strictly(void)
{
	uint32_t count = 0;
	const voe_assets_documented_key *rows = voe_assets_material_keys(&count);

	VOE_TEST_CHECK_INT(count, 13);
	check_rows(rows, count, material_reads, true);
}

static void parts_keys_read(void)
{
	uint32_t count = 0;
	const voe_assets_documented_key *rows = voe_assets_parts_keys(&count);

	VOE_TEST_CHECK_INT(count, 4);
	check_rows(rows, count, parts_reads, false);
}

int main(void)
{
	material_keys_read_strictly();
	parts_keys_read();
	return voe_test_result();
}
