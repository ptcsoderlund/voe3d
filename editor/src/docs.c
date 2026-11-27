// A project's Docs written — see docs.h.
//
// The three texts are built in scratch: the scene's and the key tables' from
// authoring/reference.h, the index from the program's path, the project's
// folder and the engine folders with an include/ under toolchain.h's engine
// folder, listed one level and looked for with game_tree_find.h. Each is
// compared against the file on disk and written with platform/file.h only when
// it differs. Formatting is game_tree.h's.
#include "docs.h"

#include "game_tree.h"
#include "game_tree_find.h"
#include "toolchain.h"

#include <assets/landscape.h>
#include <assets/material.h>
#include <assets/parts.h>

#include <authoring/project.h>
#include <authoring/reference.h>

#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <string.h>

// The key tables voe3d-files.md is made of, in its order, each title naming
// the extension or file it documents.
static const struct {
	const char *title;
	const voe_assets_documented_key *(*keys)(uint32_t *count);
} key_tables[] = {
	{ "Material files (`.material`)", voe_assets_material_keys },
	{ "Parts files (`.parts`)", voe_assets_parts_keys },
	{ "Landscape files (`.landscape`)", voe_assets_landscape_keys },
	{ "The project file (`project.voe3d`)", voe_authoring_project_keys },
};

// `<docs>/<name>` given text unless it already holds exactly it. False with
// why naming the file on a refused read or write.
static bool doc_write(const char *docs, const char *name, const char *text,
		      voe_base_arena *arena, voe_editor_notice *why)
{
	const char *path = voe_platform_path_join(arena, docs, name);
	size_t size = strlen(text);
	const uint8_t *old;
	size_t count;

	VOE_BASE_ASSERT(size > 0, "writing an empty Docs file");
	voe_base_report_error_clear();
	if (voe_platform_file_exists(path)) {
		old = voe_platform_file_read(path, arena, &count, NULL);
		if (old == NULL)
			goto refused;
		if (count == size && memcmp(old, text, size) == 0)
			return true;
	}
	if (voe_platform_file_write(path, (const uint8_t *)text, size, NULL))
		return true;
refused:
	voe_editor_notice_from_report(why, path);
	return false;
}

// The key tables' texts joined under one heading.
static const char *files_text(voe_base_arena *arena)
{
	const char *out = "# How the asset files and project.voe3d are written\n";
	uint32_t tables = sizeof key_tables / sizeof key_tables[0];

	for (uint32_t i = 0; i < tables; i++) {
		uint32_t count = 0;
		const voe_assets_documented_key *keys = key_tables[i].keys(&count);
		voe_authoring_text text = { 0 };
		bool written = voe_authoring_reference_keys(
			key_tables[i].title, keys, count, arena, &text);

		VOE_BASE_ASSERT(written && text.text != NULL,
				"a key table's text was refused");
		out = voe_editor_game_tree_format(arena, "%s\n%s", out, text.text);
	}
	return out;
}

// One line per engine folder with an include/, naming its `<folder>.md`; NULL
// with why naming the engine folder when it will not list.
static const char *header_folders(voe_base_arena *arena, voe_editor_notice *why)
{
	voe_platform_folder_listing listing = { 0 };
	const char *lines = "";

	voe_base_report_error_clear();
	if (!voe_platform_folder_list(VOE_TOOLCHAIN_ENGINE, arena, &listing, NULL)) {
		voe_editor_notice_from_report(why, VOE_TOOLCHAIN_ENGINE);
		return NULL;
	}
	for (uint32_t i = 0; i < listing.count; i++) {
		const voe_platform_folder_entry *entry = &listing.entries[i];
		const char *folder;
		const char *include;

		if (!entry->folder || entry->hidden)
			continue;
		folder = voe_platform_path_join(arena, VOE_TOOLCHAIN_ENGINE, entry->name);
		include = voe_platform_path_join(arena, folder, "include");
		if (!voe_editor_game_tree_folder_there(include, arena))
			continue;
		lines = voe_editor_game_tree_format(
			arena, "%s- `%s` — `%s`\n", lines, entry->name,
			voe_platform_path_join(
				arena, voe_platform_path_join(arena, include, entry->name),
				voe_editor_game_tree_format(arena, "%s.md", entry->name)));
	}
	VOE_BASE_ASSERT(lines != NULL, "a folder list with no text");
	return lines;
}

// voe3d.md: which file says what, whose files Docs/ holds, the take-in, the
// check and the headers.
static const char *index_text(const char *program, const char *folder,
			      const char *headers, voe_base_arena *arena)
{
	VOE_BASE_ASSERT(program != NULL && folder != NULL && headers != NULL,
			"an index with no program, project or headers");
	return voe_editor_game_tree_format(
		arena,
		"# VOE3D in this project\n\n"
		"How this project's files are written, for a person or an agent "
		"writing them by hand.\n\n"
		"## Which file documents which kind\n\n"
		"| Files | Documented in |\n|---|---|\n"
		"| `*.scene`, `*.prefab` | `voe3d-scene.md` |\n"
		"| `*.material`, `*.parts`, `*.landscape`, `project.voe3d` | "
		"`voe3d-files.md` |\n\n"
		"## Whose files these are\n\n"
		"The `Docs/voe3d*.md` files are the editor's: it rewrites them on "
		"every open, every Refresh and every code reload. Every other file "
		"in `Docs/` is yours; the editor never reads, writes or removes it.\n\n"
		"## Editing while the editor is open\n\n"
		"The editor takes in a file changed on disk, so editing the text "
		"files is enough. A file it refuses is named in its notice and it "
		"keeps what it had; a file with unsaved changes in the editor is "
		"asked about.\n\n"
		"## Checking the files\n\n"
		"```\n\"%s\" --check \"%s\"\n```\n\n"
		"reads every file above strictly, as the editor reads a file it "
		"takes in. It prints one line per refused file, `<path>: <why>`, "
		"then one summary line: `all is well: <N> files checked` or "
		"`<R> of <N> files refused`. It exits 0 when all is well, 1 when "
		"a file is refused or the folder is not a project, and 2 on a "
		"usage mistake. A project whose `Code/` is not built is checked "
		"without its own components, and the check says so.\n\n"
		"## The engine's headers\n\n"
		"Each engine folder's headers are at "
		"`%s/<folder>/include/<folder>/` and are included as "
		"`#include <folder/x.h>`. Look first at each folder's "
		"`<folder>.md` there, which says what each header is for:\n\n%s",
		program, folder, VOE_TOOLCHAIN_ENGINE, headers);
}

// Docs/ there already, or made one level. False with why naming it.
static bool docs_folder(const char *docs, voe_base_arena *arena,
			voe_editor_notice *why)
{
	VOE_BASE_ASSERT(docs != NULL && why != NULL, "making no Docs folder");
	if (voe_editor_game_tree_folder_there(docs, arena))
		return true;
	voe_base_report_error_clear();
	if (voe_platform_folder_create(docs, NULL))
		return true;
	voe_editor_notice_from_report(why, docs);
	return false;
}

// The three texts built, then written, the index last.
static bool docs_write_all(const voe_editor_project *project,
			   voe_base_arena *scratch, voe_editor_notice *why)
{
	const char *docs = voe_platform_path_join(scratch, project->folder, "Docs");
	voe_authoring_text scene = { 0 };
	const char *program;
	const char *headers;

	if (!docs_folder(docs, scratch, why))
		return false;
	voe_base_report_error_clear();
	if (!voe_authoring_reference_scene(project->world, scratch, &scene)) {
		voe_editor_notice_from_report(
			why, voe_platform_path_join(scratch, docs, "voe3d-scene.md"));
		return false;
	}
	program = voe_platform_path_program(scratch);
	if (program == NULL) {
		voe_editor_notice_from_report(
			why, voe_platform_path_join(scratch, docs, "voe3d.md"));
		return false;
	}
	headers = header_folders(scratch, why);
	if (headers == NULL)
		return false;
	return doc_write(docs, "voe3d-scene.md",
			 voe_editor_game_tree_format(
				 scratch, "# How scene and prefab files are written\n\n%s",
				 scene.text),
			 scratch, why) &&
	       doc_write(docs, "voe3d-files.md", files_text(scratch), scratch, why) &&
	       doc_write(docs, "voe3d.md",
			 index_text(program, project->folder, headers, scratch),
			 scratch, why);
}

bool voe_editor_docs_write(const voe_editor_project *project,
			   voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	bool written;

	VOE_BASE_ASSERT(project != NULL && scratch != NULL && why != NULL,
			"writing Docs for no project, or with no scratch or why");
	if (project->folder == NULL)
		return true;
	mark = voe_base_arena_mark(scratch);
	written = docs_write_all(project, scratch, why);
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(scratch).used == mark.used,
			"writing Docs kept scratch memory");
	return written;
}
