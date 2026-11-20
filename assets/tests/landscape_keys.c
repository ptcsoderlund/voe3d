// The landscape key table against landscape.h and its strict reader (0511):
// the rows for `size`, `cells`, `size<K>` and `blendheight<K>` carry the
// header's constants as least and greatest, and the smallest file the header
// describes, a [Landscape] and cells + 1 rows of [Heights], reads with `size`
// or `cells` at its row's greatest and is refused above it when the row says
// refused.
//
// A file at 2048 cells is about 8 MB of text, built here on the arena: the
// greatest `cells` cannot be proven read with less.
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// The row for `key` in the table, or NULL.
static const voe_assets_documented_key *row_for(const char *key)
{
	uint32_t count = 0;
	const voe_assets_documented_key *rows = voe_assets_landscape_keys(&count);

	for (uint32_t i = 0; i < count; i++)
		if (strcmp(rows[i].key, key) == 0)
			return &rows[i];
	return NULL;
}

static void landscape_keys_ranges(void)
{
	const voe_assets_documented_key *size = row_for("size");
	const voe_assets_documented_key *cells = row_for("cells");
	const voe_assets_documented_key *kind_size = row_for("size<K>");
	const voe_assets_documented_key *blend = row_for("blendheight<K>");

	VOE_TEST_CHECK(size != NULL && cells != NULL && kind_size != NULL &&
		       blend != NULL);
	VOE_TEST_CHECK(size->least == VOE_ASSETS_LANDSCAPE_SIZE_MIN &&
		       size->greatest == VOE_ASSETS_LANDSCAPE_SIZE_MAX);
	VOE_TEST_CHECK(cells->least == 4 &&
		       cells->greatest == VOE_ASSETS_LANDSCAPE_CELLS_MAX);
	VOE_TEST_CHECK(kind_size->least == VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN &&
		       kind_size->greatest == VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX);
	VOE_TEST_CHECK(blend->least == VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN &&
		       blend->greatest == VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX);
	VOE_TEST_CHECK_INT(blend->refused, true);
}

// The smallest landscape at `size` metres and `cells` cells, every height 0,
// read strictly; `error` gets the refusal.
static bool grid_reads(voe_base_arena *arena, double size, uint32_t cells,
		       voe_base_error *error)
{
	const struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	const size_t room = 64 + ((size_t)cells + 1) * (2 * (size_t)cells + 16);
	char *text = voe_base_arena_push(arena, room);
	voe_assets_landscape out;
	size_t used;
	bool read;

	used = (size_t)snprintf(text, room,
				"[Landscape]\nsize=%.9g\ncells=%u\n\n[Heights]\n",
				size, cells);
	for (uint32_t row = 0; row <= cells; row++) {
		used += (size_t)snprintf(text + used, room - used, "row%u=", row);
		for (uint32_t c = 0; c <= cells; c++) {
			text[used++] = '0';
			text[used++] = c < cells ? ' ' : '\n';
		}
	}
	VOE_TEST_CHECK(used < room);
	read = voe_assets_landscape_read_strict(text, used, arena, &out, error);
	voe_base_arena_rewind(arena, mark);
	return read;
}

static void landscape_keys_refused(voe_base_arena *arena)
{
	const voe_assets_documented_key *size = row_for("size");
	const voe_assets_documented_key *cells = row_for("cells");
	voe_base_error error = VOE_BASE_ERROR_MALFORMED;

	VOE_TEST_CHECK(size != NULL && cells != NULL);
	VOE_TEST_CHECK(grid_reads(arena, size->least, 4, NULL));
	VOE_TEST_CHECK(grid_reads(arena, size->greatest, 4, NULL));
	VOE_TEST_CHECK(size->refused);
	VOE_TEST_CHECK(!grid_reads(arena, size->greatest + 1.0, 4, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNSUPPORTED);

	VOE_TEST_CHECK(grid_reads(arena, size->least,
				  (uint32_t)cells->greatest, NULL));
	VOE_TEST_CHECK(cells->refused);
	// Above by 4, so the multiple of 4 the reader also asks for is kept and
	// only the range refuses it.
	error = VOE_BASE_ERROR_MALFORMED;
	VOE_TEST_CHECK(!grid_reads(arena, size->least,
				   (uint32_t)cells->greatest + 4, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNSUPPORTED);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	landscape_keys_ranges();
	landscape_keys_refused(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
