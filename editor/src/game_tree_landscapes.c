// landscapes.c's text: every `.landscape` under a project's Assets/ as saved,
// cooked into C arrays and named in voe_game_landscapes_cooked (game/
// landscapes.h, ADR-0379 point 7, ADR-0402 point 8). Called by game_tree.c's
// write; see game_tree.h.
//
//     #include <game/landscapes.h>
//
//     static const int32_t landscape_0[25] = { ... };
//     static const uint8_t landscape_0_paint[100] = { ... };
//     static const char *const landscape_0_layers[] = {
//             "Assets/Dirt.material",
//             "Assets/Grass.material",
//     };
//     static const char *const landscape_0_kinds[] = {
//             "Assets/Pine.glb",
//     };
//     static const voe_assets_landscape_look landscape_0_looks[] = {
//             {2000, 20, 180, 0, false},
//     };
//     static const uint16_t landscape_0_grows[] = {
//             0,
//             40,
//     };
//     static const voe_assets_landscape_copy landscape_0_copies[2] = {
//             {0, -1500, 16384, 1000, 0, 0},
//             {9000, 2500, 0, 1200, 0, 0},
//     };
//     const voe_game_landscapes voe_game_landscapes_cooked = {
//             (const voe_game_landscape[]){
//                     { "Assets/Hill.landscape", 0x1p+8f, 4, landscape_0,
//                       landscape_0_layers, 2, landscape_0_paint,
//                       landscape_0_kinds, 1, landscape_0_copies, 2,
//                       landscape_0_looks, landscape_0_grows },
//             },
//             1,
//     };
//
// None is `{ NULL, 0 }`. The size is printed `%a`, so the float the game reads
// is exactly the one the file read gave. A landscape with no layers names
// `NULL, 0, NULL`; one with layers but no [Paint] is cooked all layer 0, its
// paint built in the scratch first. The last six fields are the foliage (0442
// point 1, 0476 points 1-2): kind paths and count, `NULL, 0` with no kinds;
// copies and count, `NULL, 0` with none; each kind's look as the grid holds it,
// NULL with no kinds; and each layer's density of each kind in tenths, layer
// by layer, NULL when no layer grows any. Kinds are cooked without copies, so
// Play and a shipped game grow what the editor grows. Paths are escaped as C
// strings.
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

// The scratch one landscape is read and cooked in, from a Created landscape's
// points at 64 bytes each: its text, heights, 8 bytes of paint and both cooked,
// and the most copies at 40 bytes each: their line in the file and cooked,
// the most kinds' looks at 128 bytes each: their keys and their cooked line,
// and every grow at 24 bytes: its key and its cooked number.
// A block size, not a limit: a larger landscape chains blocks.
#define LANDSCAPE_SCRATCH                                                        \
	((size_t)(VOE_ASSETS_LANDSCAPE_CELLS + 1u) * (VOE_ASSETS_LANDSCAPE_CELLS + 1u) * 64u + \
	 (size_t)VOE_ASSETS_LANDSCAPE_COPIES * 40u +                             \
	 (size_t)VOE_ASSETS_LANDSCAPE_KINDS * 128u +                             \
	 (size_t)VOE_ASSETS_LANDSCAPE_LAYERS * VOE_ASSETS_LANDSCAPE_KINDS * 24u)

// One landscape cooked: its arrays' text and what its table entry names.
struct landscape_cooked {
	const char *text;
	size_t size;
	float metres;
	uint32_t cells;
	uint32_t layer_count;
	uint32_t kind_count;
	uint32_t copy_count;
	bool grown;
};

// Every point of a cells-a-side grid all layer 0: both RGBA8 planes in scratch,
// as assets/landscape.h lays them out, for a file with layers and no [Paint].
static uint8_t *landscape_ground_paint(uint32_t cells, voe_base_arena *scratch)
{
	size_t points = (size_t)(cells + 1) * (cells + 1);
	uint8_t *paint = voe_base_arena_push(scratch, points * 8);

	VOE_BASE_ASSERT(cells > 0 && cells <= VOE_ASSETS_LANDSCAPE_CELLS_MAX,
			"ground paint for no grid");
	memset(paint, 0, points * 8);
	for (size_t i = 0; i < points; i++)
		paint[i * 4] = 255;
	VOE_BASE_ASSERT(paint[(points - 1) * 4] == 255, "ground paint left a point bare");
	return paint;
}

// landscape's paint cooked as landscape_<n>_paint and its layer paths as
// landscape_<n>_layers, after heights, as one string in scratch.
static const char *landscape_layers_text(voe_assets_landscape *landscape, uint32_t n,
					 const char *heights, voe_base_arena *scratch)
{
	voe_authoring_text paint;
	const char *text;

	VOE_BASE_ASSERT(landscape->layer_count > 0 &&
				landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"cooking layers of a landscape with none");
	if (landscape->paint == NULL)
		landscape->paint = landscape_ground_paint(landscape->cells, scratch);
	paint = voe_authoring_landscape_cook_paint(
		landscape, voe_editor_game_tree_format(scratch, "landscape_%u_paint", n),
		scratch);
	text = voe_editor_game_tree_format(
		scratch, "%s%sstatic const char *const landscape_%u_layers[] = {\n", heights,
		paint.text, n);
	for (uint32_t k = 0; k < landscape->layer_count; k++)
		text = voe_editor_game_tree_format(
			scratch, "%s\t\"%s\",\n", text,
			voe_editor_game_tree_c_string(scratch, landscape->layers[k]));
	text = voe_editor_game_tree_format(scratch, "%s};\n", text);
	VOE_BASE_ASSERT(text != NULL, "no layers text");
	return text;
}

// Whether any of landscape's layers grows any of its kinds.
static bool landscape_grown(const voe_assets_landscape *landscape)
{
	for (uint32_t l = 0; l < landscape->layer_count; l++)
		for (uint32_t k = 0; k < landscape->kind_count; k++)
			if (landscape->grows[l][k] > 0)
				return true;
	return false;
}

// Each layer's density of each kind as landscape_<n>_grows, one line a layer,
// as one string in scratch.
static const char *landscape_grows_text(const voe_assets_landscape *landscape, uint32_t n,
					voe_base_arena *scratch)
{
	const char *text = voe_editor_game_tree_format(
		scratch, "static const uint16_t landscape_%u_grows[] = {\n", n);

	VOE_BASE_ASSERT(landscape->layer_count > 0 && landscape->kind_count > 0,
			"cooking growth with no layers or no kinds");
	for (uint32_t l = 0; l < landscape->layer_count; l++) {
		text = voe_editor_game_tree_format(scratch, "%s\t", text);
		for (uint32_t k = 0; k < landscape->kind_count; k++)
			text = voe_editor_game_tree_format(scratch, "%s%u,%s", text,
							   (unsigned)landscape->grows[l][k],
							   k + 1 == landscape->kind_count ? "\n"
											  : " ");
	}
	return voe_editor_game_tree_format(scratch, "%s};\n", text);
}

// landscape's kind paths as landscape_<n>_kinds and their looks as
// landscape_<n>_looks, then its growth as landscape_<n>_grows when grown and
// its copies as landscape_<n>_copies when it has any, after before, as one
// string in scratch. The kinds are gathered apart and joined to before once, so
// before is copied once, not per kind.
static const char *landscape_foliage_text(const voe_assets_landscape *landscape,
					  uint32_t n, bool grown, const char *before,
					  voe_base_arena *scratch)
{
	const char *kinds = "";
	const char *looks = "";
	const char *grows = grown ? landscape_grows_text(landscape, n, scratch) : "";
	const char *copies = "";

	VOE_BASE_ASSERT(landscape->kind_count > 0 &&
				landscape->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"cooking foliage of a landscape with none");
	for (uint32_t k = 0; k < landscape->kind_count; k++) {
		const voe_assets_landscape_look *look = &landscape->looks[k];

		kinds = voe_editor_game_tree_format(
			scratch, "%s\t\"%s\",\n", kinds,
			voe_editor_game_tree_c_string(scratch, landscape->kinds[k]));
		looks = voe_editor_game_tree_format(
			scratch, "%s\t{%u, %u, %u, %u, %s},\n", looks, (unsigned)look->size,
			(unsigned)look->size_vary, (unsigned)look->turn_vary,
			(unsigned)look->tilt, look->shadowless ? "true" : "false");
	}
	if (landscape->copy_count > 0) {
		copies = voe_authoring_landscape_cook_copies(
				 landscape,
				 voe_editor_game_tree_format(scratch, "landscape_%u_copies", n),
				 scratch)
				 .text;
		VOE_BASE_ASSERT(copies != NULL, "no copies text");
	}
	return voe_editor_game_tree_format(
		scratch,
		"%sstatic const char *const landscape_%u_kinds[] = {\n%s};\n"
		"static const voe_assets_landscape_look landscape_%u_looks[] = {\n%s};\n%s%s",
		before, n, kinds, n, looks, grows, copies);
}

// landscape_<n>'s table entry, path its path under Assets/: size, cells and
// heights, then layers and paint, then kinds, copies, looks and grows, NULLs
// where none.
static const char *landscape_entry(const char *path, uint32_t n,
				   const struct landscape_cooked *cooked,
				   voe_base_arena *arena)
{
	const char *layers =
		cooked->layer_count == 0
			? "NULL, 0, NULL"
			: voe_editor_game_tree_format(
				  arena, "landscape_%u_layers, %u, landscape_%u_paint", n,
				  cooked->layer_count, n);
	const char *kinds = cooked->kind_count == 0
				    ? "NULL, 0"
				    : voe_editor_game_tree_format(arena, "landscape_%u_kinds, %u",
								  n, cooked->kind_count);
	const char *copies = cooked->copy_count == 0
				     ? "NULL, 0"
				     : voe_editor_game_tree_format(arena, "landscape_%u_copies, %u",
								   n, cooked->copy_count);
	const char *looks = cooked->kind_count == 0
				    ? "NULL"
				    : voe_editor_game_tree_format(arena, "landscape_%u_looks", n);
	const char *grows = !cooked->grown
				    ? "NULL"
				    : voe_editor_game_tree_format(arena, "landscape_%u_grows", n);

	VOE_BASE_ASSERT(path != NULL, "a landscape entry with no path");
	VOE_BASE_ASSERT(cooked->kind_count > 0 || cooked->copy_count == 0,
			"copies with no kinds");
	return voe_editor_game_tree_format(
		arena,
		"\t\t{ \"Assets/%s\", %af, %u, landscape_%u, %s,\n\t\t  %s, %s, %s, %s },\n",
		voe_editor_game_tree_c_string(arena, path), (double)cooked->metres,
		cooked->cells, n, layers, kinds, copies, looks, grows);
}

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
		voe_authoring_text heights = voe_authoring_landscape_cook(
			&landscape,
			voe_editor_game_tree_format(scratch, "landscape_%u", n), scratch);
		const char *cooked = heights.text;
		bool grown = landscape_grown(&landscape);
		size_t length;
		char *text;

		if (landscape.layer_count > 0)
			cooked = landscape_layers_text(&landscape, n, cooked, scratch);
		if (landscape.kind_count > 0)
			cooked = landscape_foliage_text(&landscape, n, grown, cooked, scratch);
		length = strlen(cooked);
		text = voe_base_arena_push(arena, length + 1);
		VOE_BASE_ASSERT(text != NULL, "no room for a cooked landscape");
		memcpy(text, cooked, length + 1);
		*out = (struct landscape_cooked){ text,
						  length,
						  landscape.size,
						  landscape.cells,
						  landscape.layer_count,
						  landscape.kind_count,
						  landscape.copy_count,
						  grown };
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
		if (!done)
			break;
		entries = voe_editor_game_tree_format(arena, "%s%s", entries,
						      landscape_entry(path, i, &cooked[i], arena));
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
