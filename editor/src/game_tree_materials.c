// materials.c's text: every `.material` under a project's Assets/ as saved,
// written as one voe_game_material initializer each in
// voe_game_materials_cooked (game/materials.h, ADR-0399 point 7), then every
// `.parts` as one voe_game_model_parts each in voe_game_parts_cooked, named by
// its `.glb` (0420 point 1). Called by game_tree.c's write; see game_tree.h.
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
//     const voe_game_parts voe_game_parts_cooked = {
//             (const voe_game_model_parts[]){
//                     { "Assets/wall.glb", { .count = 2, .materials = {
//                             "Assets/brick.material",
//                             "",
//                       },
//                       .levels = { .count = 4, .strength = VOE_ASSETS_LEVELS_MEDIUM,
//                                   .swap = VOE_ASSETS_LEVELS_SWAP_NORMAL } } },
//             },
//             1,
//     };
//
// Neither is `{ NULL, 0 }`. Floats are printed `%a`, so the value the game
// reads is exactly the one the file read gave; the path and map strings are
// escaped as game_tree.c escapes a C string literal. A file that will not read
// or parse is refused, naming it.
//
// Each file is read and parsed in one scratch arena rewound after it; the
// parsed values hold their paths in their own rooms, so nothing outlives the
// rewind. The entries are joined by re-formatting the text so far per file,
// quadratic in the count; an entry is a few hundred bytes, so a join like
// game_tree_landscapes.c's would lift it if a project ever needs it.
#include "game_tree.h"

#include "game_tree_find.h"

#include <assets/material.h>
#include <assets/parts.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/path.h>

#include <string.h>

// The scratch one file is read in. A block size, not a limit.
#define MATERIAL_SCRATCH (64u * 1024u)

// The ending a parts file is found by, whose bytes its `.glb` replaces.
#define PARTS_ENDING ".parts"

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

// file read in scratch and parsed into out, scratch rewound. False with why
// naming file on a refused read or parse.
static bool parts_cook(const char *file, voe_base_arena *scratch,
		       voe_assets_parts *out, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const uint8_t *bytes;
	size_t size;
	bool done = false;

	VOE_BASE_ASSERT(file != NULL && out != NULL, "cooking no parts");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(file, scratch, &size, NULL);
	if (bytes != NULL)
		done = voe_assets_parts_read((const char *)bytes, size, out, NULL);
	if (!done)
		voe_editor_notice_from_report(why, file);
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(scratch).used == mark.used,
			"a parts cook kept scratch");
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
		"\t\t  .ormmap = \"%s\", .cutout = %af, .double_sided = %s,\n"
		"\t\t  .emission = { %af, %af, %af }, .emission_strength = %af,\n"
		"\t\t  .emissionmap = \"%s\" } },\n",
		voe_editor_game_tree_c_string(arena, path),
		m->shader == VOE_ASSETS_MATERIAL_UNLIT ? "VOE_ASSETS_MATERIAL_UNLIT"
						       : "VOE_ASSETS_MATERIAL_LIT",
		(double)m->colour[0], (double)m->colour[1], (double)m->colour[2],
		(double)m->roughness, (double)m->metal, (double)m->repeat,
		voe_editor_game_tree_c_string(arena, m->colormap),
		voe_editor_game_tree_c_string(arena, m->normalmap),
		voe_editor_game_tree_c_string(arena, m->ormmap),
		(double)m->cutout, m->double_sided ? "true" : "false",
		(double)m->emission[0], (double)m->emission[1],
		(double)m->emission[2], (double)m->emission_strength,
		voe_editor_game_tree_c_string(arena, m->emissionmap));
	VOE_BASE_ASSERT(out[0] != '\0', "a material entry with no text");
	return out;
}

// The enumerator's name for strength, as the cooked text spells it.
static const char *strength_name(voe_assets_levels_strength strength)
{
	VOE_BASE_ASSERT(strength <= VOE_ASSETS_LEVELS_STRONG, "a strength past the three");
	if (strength == VOE_ASSETS_LEVELS_GENTLE)
		return "VOE_ASSETS_LEVELS_GENTLE";
	return strength == VOE_ASSETS_LEVELS_MEDIUM ? "VOE_ASSETS_LEVELS_MEDIUM"
						    : "VOE_ASSETS_LEVELS_STRONG";
}

// The enumerator's name for swap, as the cooked text spells it.
static const char *swap_name(voe_assets_levels_swap swap)
{
	static const char *const names[] = {
		"VOE_ASSETS_LEVELS_SWAP_NEAR", "VOE_ASSETS_LEVELS_SWAP_NORMAL",
		"VOE_ASSETS_LEVELS_SWAP_FAR", "VOE_ASSETS_LEVELS_SWAP_FARTHER"
	};

	VOE_BASE_ASSERT(swap <= VOE_ASSETS_LEVELS_SWAP_FARTHER, "a swap past the four");
	return names[swap];
}

// One table entry for the parts file at Assets/path holding p, named by the
// `.glb` beside it, in arena.
static const char *parts_entry(voe_base_arena *arena, const char *path,
			       const voe_assets_parts *p)
{
	const char *names = "";
	size_t stem;

	VOE_BASE_ASSERT(path != NULL && strlen(path) >= strlen(PARTS_ENDING),
			"an entry for parts with no ending");
	VOE_BASE_ASSERT(p != NULL && p->count <= VOE_ASSETS_PARTS,
			"an entry for parts past their room");
	stem = strlen(path) - strlen(PARTS_ENDING);
	for (uint32_t i = 0; i < p->count; i++)
		names = voe_editor_game_tree_format(
			arena, "%s\t\t\t\"%s\",\n", names,
			voe_editor_game_tree_c_string(arena, p->materials[i]));
	return voe_editor_game_tree_format(
		arena,
		"\t\t{ \"%s\", { .count = %u, .materials = {\n%s\t\t  },\n"
		"\t\t  .levels = { .count = %u, .strength = %s, .swap = %s } } },\n",
		voe_editor_game_tree_c_string(
			arena, voe_editor_game_tree_format(arena, "Assets/%.*s.glb",
							   (int)stem, path)),
		p->count, names, p->levels.count, strength_name(p->levels.strength),
		swap_name(p->levels.swap));
}

// The definition of name, a table of type whose rows are row, holding entries.
static const char *table_text(voe_base_arena *arena, const char *type,
			      const char *name, const char *row,
			      const char *entries, uint32_t count)
{
	if (count == 0)
		return voe_editor_game_tree_format(arena, "const %s %s = { NULL, 0 };\n",
						   type, name);
	return voe_editor_game_tree_format(
		arena, "const %s %s = {\n\t(const %s[]){\n%s\t},\n\t%u,\n};\n", type,
		name, row, entries, count);
}

// voe_game_materials_cooked's definition for every `.material` under assets.
static const char *materials_table(const char *assets, voe_base_arena *arena,
				   voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_editor_game_tree_found found;
	voe_assets_material_file material;
	const char *entries = "";

	if (!voe_editor_game_tree_find(assets, ".material", arena, &found, why))
		return NULL;
	for (uint32_t i = 0; i < found.count; i++) {
		const char *path = found.items[i].path;

		if (!material_cook(voe_platform_path_join(arena, assets, path),
				   scratch, &material, why))
			return NULL;
		entries = voe_editor_game_tree_format(
			arena, "%s%s", entries, material_entry(arena, path, &material));
	}
	return table_text(arena, "voe_game_materials", "voe_game_materials_cooked",
			  "voe_game_material", entries, found.count);
}

// voe_game_parts_cooked's definition for every `.parts` under assets.
static const char *parts_table(const char *assets, voe_base_arena *arena,
			       voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_editor_game_tree_found found;
	voe_assets_parts parts;
	const char *entries = "";

	if (!voe_editor_game_tree_find(assets, PARTS_ENDING, arena, &found, why))
		return NULL;
	for (uint32_t i = 0; i < found.count; i++) {
		const char *path = found.items[i].path;

		if (!parts_cook(voe_platform_path_join(arena, assets, path), scratch,
				&parts, why))
			return NULL;
		entries = voe_editor_game_tree_format(arena, "%s%s", entries,
						      parts_entry(arena, path, &parts));
	}
	return table_text(arena, "voe_game_parts", "voe_game_parts_cooked",
			  "voe_game_model_parts", entries, found.count);
}

const char *voe_editor_game_tree_materials_source(const char *folder,
						  voe_base_arena *arena,
						  voe_editor_notice *why)
{
	const char *assets;
	const char *materials;
	const char *parts = NULL;
	voe_base_arena *scratch;

	VOE_BASE_ASSERT(folder != NULL && why != NULL, "cooking materials nowhere");
	assets = voe_platform_path_join(arena, folder, "Assets");
	scratch = voe_base_arena_new(MATERIAL_SCRATCH);
	materials = materials_table(assets, arena, scratch, why);
	if (materials != NULL)
		parts = parts_table(assets, arena, scratch, why);
	voe_base_arena_destroy(scratch);
	if (parts == NULL)
		return NULL;
	return voe_editor_game_tree_format(
		arena, "#include <game/materials.h>\n#include <stddef.h>\n\n%s\n%s",
		materials, parts);
}
