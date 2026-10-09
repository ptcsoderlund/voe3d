// materials.c's text: every `.material` under a project's Assets/ as saved,
// written as one voe_game_material initializer each in
// voe_game_materials_cooked (game/materials.h, ADR-0399 point 7). Called by
// game_tree.c's write; see game_tree.h.
//
//     #include <game/materials.h>
//     #include <stddef.h>
//
//     const voe_game_materials voe_game_materials_cooked = {
//             (const voe_game_material[]){
//                     { "Assets/brick.material", { .shader = VOE_ASSETS_MATERIAL_LIT,
//                       .colour = { 0x1p+0f, 0x1p+0f, 0x1p+0f }, ... } },
//             },
//             1,
//     };
//
// None is `{ NULL, 0 }`. Floats are printed `%a`, so the value the game reads
// is exactly the one the file read gave; the path and map strings are escaped
// as game_tree.c escapes a C string literal.
//
// Each file is read and parsed in one scratch arena rewound after it; the
// parsed values hold their maps in their own rooms, so nothing outlives the
// rewind. The entries are joined by re-formatting the text so far per file,
// quadratic in the count; a material's entry is a few hundred bytes, so a
// join like game_tree_landscapes.c's would lift it if a project ever needs it.
#include "game_tree.h"

#include "game_tree_find.h"

#include <assets/material.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/path.h>

// The scratch one material is read in. A block size, not a limit.
#define MATERIAL_SCRATCH (64u * 1024u)

// file read in scratch and parsed into out, scratch rewound. False with why
// naming file on a refused read or parse.
static bool material_cook(const char *file, voe_base_arena *scratch,
			  voe_assets_material_file *out, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const uint8_t *bytes;
	size_t size;
	bool done = false;

	VOE_BASE_ASSERT(file != NULL && out != NULL, "cooking no material");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(file, scratch, &size, NULL);
	if (bytes != NULL)
		done = voe_assets_material_read((const char *)bytes, size, out, NULL);
	if (!done)
		voe_editor_notice_from_report(why, file);
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(scratch).used == mark.used,
			"a material cook kept scratch");
	return done;
}

// One table entry for the material at Assets/path holding m, in arena.
static const char *material_entry(voe_base_arena *arena, const char *path,
				  const voe_assets_material_file *m)
{
	const char *out;

	VOE_BASE_ASSERT(path != NULL && m != NULL, "an entry for no material");
	out = voe_editor_game_tree_format(
		arena,
		"\t\t{ \"Assets/%s\", { .shader = %s,\n"
		"\t\t  .colour = { %af, %af, %af }, .roughness = %af, .metal = %af,\n"
		"\t\t  .repeat = %af, .colormap = \"%s\", .normalmap = \"%s\",\n"
		"\t\t  .ormmap = \"%s\" } },\n",
		voe_editor_game_tree_c_string(arena, path),
		m->shader == VOE_ASSETS_MATERIAL_UNLIT ? "VOE_ASSETS_MATERIAL_UNLIT"
						       : "VOE_ASSETS_MATERIAL_LIT",
		(double)m->colour[0], (double)m->colour[1], (double)m->colour[2],
		(double)m->roughness, (double)m->metal, (double)m->repeat,
		voe_editor_game_tree_c_string(arena, m->colormap),
		voe_editor_game_tree_c_string(arena, m->normalmap),
		voe_editor_game_tree_c_string(arena, m->ormmap));
	VOE_BASE_ASSERT(out[0] != '\0', "a material entry with no text");
	return out;
}

const char *voe_editor_game_tree_materials_source(const char *folder,
						  voe_base_arena *arena,
						  voe_editor_notice *why)
{
	const char *assets = voe_platform_path_join(arena, folder, "Assets");
	struct voe_editor_game_tree_found found;
	voe_assets_material_file material;
	voe_base_arena *scratch;
	const char *entries = "";
	bool done = true;

	VOE_BASE_ASSERT(folder != NULL && why != NULL, "cooking materials nowhere");
	if (!voe_editor_game_tree_find(assets, ".material", arena, &found, why))
		return NULL;
	scratch = voe_base_arena_new(MATERIAL_SCRATCH);
	for (uint32_t i = 0; i < found.count && done; i++) {
		const char *path = found.items[i].path;

		done = material_cook(voe_platform_path_join(arena, assets, path),
				     scratch, &material, why);
		if (done)
			entries = voe_editor_game_tree_format(
				arena, "%s%s", entries,
				material_entry(arena, path, &material));
	}
	voe_base_arena_destroy(scratch);
	if (!done)
		return NULL;
	return voe_editor_game_tree_format(
		arena, "#include <game/materials.h>\n#include <stddef.h>\n\n%s",
		found.count == 0
			? "const voe_game_materials voe_game_materials_cooked = { NULL, 0 };\n"
			: voe_editor_game_tree_format(
				  arena,
				  "const voe_game_materials voe_game_materials_cooked = {\n"
				  "\t(const voe_game_material[]){\n%s\t},\n\t%u,\n};\n",
				  entries, found.count));
}
