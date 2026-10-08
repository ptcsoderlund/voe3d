// landscapes.c's text: every `.landscape` under a project's Assets/ as saved,
// cooked into one int32_t array each and named in voe_game_landscapes_cooked
// (game/landscapes.h, ADR-0379 point 7). Called by game_tree.c's write; see
// game_tree.h.
//
//     #include <game/landscapes.h>
//
//     static const int32_t landscape_0[25] = { ... };
//     const voe_game_landscapes voe_game_landscapes_cooked = {
//             (const voe_game_landscape[]){
//                     { "Assets/Hill.landscape", 0x1p+8f, 4, landscape_0 },
//             },
//             1,
//     };
//
// None is `{ NULL, 0 }`. The size is printed `%a`, so the float the game reads
// is exactly the one the file read gave.
//
// Each file is read, parsed and cooked in one scratch arena rewound after it,
// sized in blocks; only the cooked text is copied out. The arrays are joined
// once at the end, so many landscapes cost one copy each, not a growing
// string copied per file.
#include "game_tree.h"

#include "game_tree_find.h"

#include <assets/landscape.h>
#include <authoring/landscape_cook.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/path.h>

#include <string.h>

// The scratch one landscape is read and cooked in. A block size, not a limit.
#define LANDSCAPE_SCRATCH (8u * 1024u * 1024u)

// One landscape cooked: its array's text and what its table entry names.
struct landscape_cooked {
	const char *text;
	size_t size;
	float metres;
	uint32_t cells;
};

// file read in scratch and cooked as landscape_<n>, the text copied into arena
// and scratch rewound. False with why naming file on a refused read or parse.
static bool landscape_cook(const char *file, uint32_t n, voe_base_arena *scratch,
			   voe_base_arena *arena, struct landscape_cooked *out,
			   voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const uint8_t *bytes;
	size_t size;
	voe_assets_landscape landscape;
	bool done = false;

	VOE_BASE_ASSERT(file != NULL && out != NULL, "cooking no landscape");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(file, scratch, &size, NULL);
	if (bytes != NULL)
		done = voe_assets_landscape_read((const char *)bytes, size, scratch,
						 &landscape, NULL);
	if (!done) {
		voe_editor_notice_from_report(why, file);
	} else {
		voe_authoring_text cooked = voe_authoring_landscape_cook(
			&landscape,
			voe_editor_game_tree_format(scratch, "landscape_%u", n), scratch);
		char *text = voe_base_arena_push(arena, cooked.size + 1);

		VOE_BASE_ASSERT(text != NULL, "no room for a cooked landscape");
		memcpy(text, cooked.text, cooked.size);
		text[cooked.size] = '\0';
		*out = (struct landscape_cooked){ text, cooked.size, landscape.size,
						  landscape.cells };
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(scratch).used == mark.used,
			"a landscape cook kept scratch");
	return done;
}

// head, every cooked array in order, then tail, as one string in arena.
static const char *landscapes_join(const char *head,
				   const struct landscape_cooked *cooked,
				   uint32_t count, const char *tail,
				   voe_base_arena *arena)
{
	size_t size = strlen(head) + strlen(tail);
	char *out;
	size_t at;

	for (uint32_t i = 0; i < count; i++)
		size += cooked[i].size + 1;
	out = voe_base_arena_push(arena, size + 1);
	VOE_BASE_ASSERT(out != NULL, "no room for landscapes.c");
	at = strlen(head);
	memcpy(out, head, at);
	for (uint32_t i = 0; i < count; i++) {
		memcpy(out + at, cooked[i].text, cooked[i].size);
		at += cooked[i].size;
		out[at++] = '\n';
	}
	memcpy(out + at, tail, strlen(tail) + 1);
	VOE_BASE_ASSERT(at + strlen(tail) == size, "landscapes.c outgrew its room");
	return out;
}

const char *voe_editor_game_tree_landscapes_source(const char *folder,
						   voe_base_arena *arena,
						   voe_editor_notice *why)
{
	const char *assets = voe_platform_path_join(arena, folder, "Assets");
	struct voe_editor_game_tree_found found;
	struct landscape_cooked *cooked;
	voe_base_arena *scratch;
	const char *entries = "";
	bool done = true;

	VOE_BASE_ASSERT(folder != NULL && why != NULL, "cooking landscapes nowhere");
	if (!voe_editor_game_tree_find(assets, ".landscape", arena, &found, why))
		return NULL;
	cooked = voe_base_arena_push(arena, (found.count + 1) * sizeof *cooked);
	VOE_BASE_ASSERT(cooked != NULL, "no room for the cooked landscapes");
	scratch = voe_base_arena_new(LANDSCAPE_SCRATCH);
	for (uint32_t i = 0; i < found.count && done; i++) {
		const char *path = found.items[i].path;

		done = landscape_cook(voe_platform_path_join(arena, assets, path), i,
				      scratch, arena, &cooked[i], why);
		if (done)
			entries = voe_editor_game_tree_format(
				arena, "%s\t\t{ \"Assets/%s\", %af, %u, landscape_%u },\n",
				entries, voe_editor_game_tree_c_string(arena, path),
				(double)cooked[i].metres, cooked[i].cells, i);
	}
	voe_base_arena_destroy(scratch);
	if (!done)
		return NULL;
	return landscapes_join(
		"#include <game/landscapes.h>\n#include <stddef.h>\n\n", cooked, found.count,
		found.count == 0
			? "const voe_game_landscapes voe_game_landscapes_cooked = { NULL, 0 };\n"
			: voe_editor_game_tree_format(
				  arena,
				  "const voe_game_landscapes voe_game_landscapes_cooked = {\n"
				  "\t(const voe_game_landscape[]){\n%s\t},\n\t%u,\n};\n",
				  entries, found.count),
		arena);
}
