// The game tree written and its argument lists built — see game_tree.h.
//
// A file is compared against what is on disk before it is written, so its
// timestamp only moves when its bytes do. Text put into a generated file is
// escaped for where it goes: the project's name as a C string literal in
// main.c beside its game window's numbers, the engine's and the code's paths
// and the name as quoted CMake arguments in CMakeLists.txt. Code/ is looked for
// in the project's own listing before it is listed, so a project with none
// reports nothing.
//
// The prefabs under Assets/ are found with an explicit stack of listings, one
// per folder level, so a folder deeper than PREFAB_DEPTH (a symlink loop
// included) is refused rather than followed; a larger constant lifts it. Each
// prefab is read and cooked in one scratch arena rewound after it, sized in
// blocks as a project's world is.
//
// An argument list is at most ARGUMENTS entries, the NULL included, in one
// struct pushed into the caller's arena.
#include "game_tree.h"

#include "toolchain.h"

#include <authoring/prefab.h>
#include <authoring/scene_cook.h>
#include <authoring/scene_read.h>
#include <base/assert.h>
#include <base/report.h>
#include <game/prefabs.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARGUMENTS 16

// How many folder levels under Assets/ are looked in for prefabs, Assets/ one.
#define PREFAB_DEPTH 16

// The scratch one prefab is read and cooked in. A block size, not a limit.
#define PREFAB_SCRATCH (4u * 1024u * 1024u)

typedef struct {
	const char *items[ARGUMENTS];
	int count;
} arguments;

// Formats into arena, measured first so no fixed buffer guesses at a path.
[[gnu::format(printf, 2, 3)]]
static const char *format(voe_base_arena *arena, const char *pattern, ...)
{
	va_list measuring;
	va_list writing;
	int length;
	char *out;

	va_start(measuring, pattern);
	va_copy(writing, measuring);
	length = vsnprintf(NULL, 0, pattern, measuring);
	va_end(measuring);
	VOE_BASE_ASSERT(length >= 0, "a game tree text that could not be formatted");

	out = voe_base_arena_push(arena, (size_t)length + 1);
	(void)vsnprintf(out, (size_t)length + 1, pattern, writing);
	va_end(writing);
	VOE_BASE_ASSERT(out[length] == '\0', "a formatted text left unterminated");
	return out;
}

// text for inside a C string literal (cmake false): `"` and `\` escaped, every
// byte outside printable ASCII as three octal digits. For inside a quoted
// CMake argument (cmake true): `"`, `\` and `$` escaped with a backslash.
static const char *escaped(voe_base_arena *arena, const char *text, bool cmake)
{
	size_t length = strlen(text);
	char *out = voe_base_arena_push(arena, length * 4 + 1);
	size_t at = 0;

	VOE_BASE_ASSERT(out != NULL, "no room to escape a text");
	for (size_t i = 0; i < length; i++) {
		unsigned char c = (unsigned char)text[i];

		if (c == '"' || c == '\\' || (cmake && c == '$')) {
			out[at++] = '\\';
			out[at++] = (char)c;
		} else if (!cmake && (c < 0x20 || c > 0x7e)) {
			at += (size_t)snprintf(out + at, 5, "\\%03o", c);
		} else {
			out[at++] = (char)c;
		}
	}
	out[at] = '\0';
	VOE_BASE_ASSERT(at <= length * 4, "an escaped text outgrew its room");
	return out;
}

// Whether path is a folder, looked for in its parent's listing, because
// listing a path that is not there reports an error on stderr.
static bool folder_there(const char *path, voe_base_arena *arena)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	const char *name = voe_platform_path_name(path);
	voe_platform_folder_listing listing;
	bool there = false;

	VOE_BASE_ASSERT(path != NULL && name != NULL, "looking for no folder");
	if (voe_platform_folder_list(voe_platform_path_parent(arena, path), arena,
				     &listing, NULL))
		for (uint32_t i = 0; i < listing.count && !there; i++)
			there = listing.entries[i].folder &&
				strcmp(listing.entries[i].name, name) == 0;
	voe_base_arena_rewind(arena, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(arena).used == mark.used,
			"a folder lookup kept memory");
	return there;
}

// path already a folder, or made one level. False with why naming it.
static bool folder_ensure(const char *path, voe_base_arena *arena,
			  voe_editor_notice *why)
{
	VOE_BASE_ASSERT(path != NULL, "making no folder");
	VOE_BASE_ASSERT(why != NULL, "making a folder with nowhere to say why");
	if (folder_there(path, arena))
		return true;
	voe_base_report_error_clear();
	if (voe_platform_folder_create(path, NULL))
		return true;
	voe_editor_notice_from_report(why, path);
	return false;
}

// text written to path unless the file there already holds exactly it.
static bool file_write_changed(const char *path, const char *text,
			       voe_base_arena *arena, voe_editor_notice *why)
{
	size_t size = strlen(text);
	size_t count;
	const uint8_t *old;

	VOE_BASE_ASSERT(size > 0, "writing an empty game tree file");
	VOE_BASE_ASSERT(why != NULL, "writing a file with nowhere to say why");
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

static const char *cmake_lists(const voe_editor_project *project,
			       voe_base_arena *arena)
{
	const char *code = voe_platform_path_join(arena, project->folder, "Code");
	const char *name = voe_editor_project_name(project);

	VOE_BASE_ASSERT(name != NULL, "a game tree for an untitled project");
	return format(arena,
		      "cmake_minimum_required(VERSION 3.28)\n"
		      "project(voe_game_tree C)\n"
		      "set(VOE_ENGINE \"%s\")\n"
		      "set(VOE_PROJECT_CODE \"%s\")\n"
		      "set(VOE_GAME_NAME \"%s\")\n"
		      "include(\"${VOE_ENGINE}/cmake/game.cmake\")\n",
		      escaped(arena, VOE_TOOLCHAIN_ENGINE, true),
		      escaped(arena, code, true), escaped(arena, name, true));
}

// <folder>/Code/'s entries, sorted by name; none when it is not a folder.
static voe_platform_folder_listing code_list(const char *folder,
					     voe_base_arena *arena)
{
	voe_platform_folder_listing listing = { 0 };
	bool found = false;

	VOE_BASE_ASSERT(folder != NULL && arena != NULL, "listing no project's code");
	if (!voe_platform_folder_list(folder, arena, &listing, NULL)) {
		voe_base_report_error_clear();
		return (voe_platform_folder_listing){ 0 };
	}
	for (uint32_t i = 0; i < listing.count && !found; i++)
		found = listing.entries[i].folder &&
			strcmp(listing.entries[i].name, "Code") == 0;
	listing = (voe_platform_folder_listing){ 0 };
	if (found && !voe_platform_folder_list(voe_platform_path_join(arena, folder, "Code"),
					       arena, &listing, NULL)) {
		voe_base_report_error_clear();
		listing = (voe_platform_folder_listing){ 0 };
	}
	VOE_BASE_ASSERT(listing.count == 0 || listing.entries != NULL,
			"a code listing with entries and nowhere to hold them");
	return listing;
}

// Whether entry is a file whose name ends in suffix, a name longer than it.
static bool entry_ends(const voe_platform_folder_entry *entry, const char *suffix)
{
	size_t length = strlen(entry->name);
	size_t tail = strlen(suffix);

	VOE_BASE_ASSERT(tail > 0, "matching an empty suffix");
	return !entry->folder && length > tail &&
	       strcmp(entry->name + length - tail, suffix) == 0;
}

// `#include "<name>.h"` for every .h in Code/, sorted by name; "" for none.
static const char *code_includes(const char *folder, voe_base_arena *arena)
{
	voe_platform_folder_listing code = code_list(folder, arena);
	const char *out = "";

	for (uint32_t i = 0; i < code.count; i++) {
		if (entry_ends(&code.entries[i], ".h"))
			out = format(arena, "%s#include \"%s\"\n", out,
				     escaped(arena, code.entries[i].name, false));
	}
	VOE_BASE_ASSERT(out != NULL, "no code includes made");
	return out;
}

// Code/'s includes, then the cooked scene.
static const char *scene_source(const char *folder,
				const voe_authoring_text *cooked,
				voe_base_arena *arena)
{
	const char *out = format(arena, "%s%.*s", code_includes(folder, arena),
				 (int)cooked->size, cooked->text);

	VOE_BASE_ASSERT(out[0] != '\0', "a scene with no text");
	return out;
}

// The paths of the prefabs found, under Assets/ with `/`, in arena.
struct prefab_path {
	const char *path;
};

struct prefab_paths {
	struct prefab_path *items;
	uint32_t count;
	uint32_t room;
};

// One folder level being walked: its path under Assets/ ("" for Assets/), its
// listing and the next entry to look at.
struct prefab_folder {
	const char *relative;
	voe_platform_folder_listing listing;
	uint32_t next;
};

// Whether name ends `.prefab`, in any case, and is longer than it.
static bool prefab_named(const char *name)
{
	static const char suffix[] = ".prefab";
	size_t length = strlen(name);
	size_t tail = sizeof suffix - 1;

	VOE_BASE_ASSERT(tail == 7, "a prefab suffix of the wrong length");
	if (length <= tail)
		return false;
	for (size_t i = 0; i < tail; i++) {
		if (tolower((unsigned char)name[length - tail + i]) != suffix[i])
			return false;
	}
	return true;
}

// path added, the room doubled into arena when it is full.
static void prefab_path_add(struct prefab_paths *paths, voe_base_arena *arena,
			    const char *path)
{
	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "adding no prefab path");
	if (paths->count == paths->room) {
		uint32_t room = paths->room == 0 ? 16 : paths->room * 2;
		struct prefab_path *items =
			voe_base_arena_push(arena, room * sizeof *items);

		VOE_BASE_ASSERT(items != NULL, "no room for the prefab paths");
		if (paths->count > 0)
			memcpy(items, paths->items, paths->count * sizeof *items);
		paths->items = items;
		paths->room = room;
	}
	paths->items[paths->count++].path = path;
	VOE_BASE_ASSERT(paths->count <= paths->room, "prefab paths outgrew their room");
}

// assets/<relative> listed into frame. False with why naming it.
static bool prefab_folder_list(const char *assets, const char *relative,
			       voe_base_arena *arena, struct prefab_folder *frame,
			       voe_editor_notice *why)
{
	const char *path = relative[0] == '\0'
				   ? assets
				   : voe_platform_path_join(arena, assets, relative);

	VOE_BASE_ASSERT(frame != NULL && why != NULL, "listing into no frame");
	*frame = (struct prefab_folder){ .relative = relative };
	voe_base_report_error_clear();
	if (voe_platform_folder_list(path, arena, &frame->listing, NULL))
		return true;
	voe_editor_notice_from_report(why, path);
	return false;
}

// Every `.prefab` file under assets, hidden entries skipped, unsorted; none
// when assets is not a folder. False with why on a folder that will not list
// or one deeper than PREFAB_DEPTH.
static bool prefabs_find(const char *assets, voe_base_arena *arena,
			 struct prefab_paths *out, voe_editor_notice *why)
{
	struct prefab_folder stack[PREFAB_DEPTH];
	uint32_t depth = 1;

	VOE_BASE_ASSERT(assets != NULL && out != NULL, "finding prefabs nowhere");
	*out = (struct prefab_paths){ 0 };
	if (!folder_there(assets, arena))
		return true;
	if (!prefab_folder_list(assets, "", arena, &stack[0], why))
		return false;
	while (depth > 0) {
		struct prefab_folder *top = &stack[depth - 1];
		const voe_platform_folder_entry *entry;
		const char *relative;

		if (top->next == top->listing.count) {
			depth--;
			continue;
		}
		entry = &top->listing.entries[top->next++];
		if (entry->hidden)
			continue;
		relative = top->relative[0] == '\0'
				   ? entry->name
				   : format(arena, "%s/%s", top->relative, entry->name);
		if (!entry->folder) {
			if (prefab_named(entry->name))
				prefab_path_add(out, arena, relative);
			continue;
		}
		if (depth == PREFAB_DEPTH) {
			voe_editor_notice_set(why, "Assets/%s is deeper than %d folders",
					      relative, PREFAB_DEPTH);
			return false;
		}
		if (!prefab_folder_list(assets, relative, arena, &stack[depth], why))
			return false;
		depth++;
	}
	VOE_BASE_ASSERT(out->count <= out->room, "prefab paths outgrew their room");
	return true;
}

static int prefab_path_order(const void *a, const void *b)
{
	const struct prefab_path *left = a;
	const struct prefab_path *right = b;

	VOE_BASE_ASSERT(left->path != NULL && right->path != NULL,
			"ordering no prefab path");
	return strcmp(left->path, right->path);
}

// One prefab cooked: its function's text and how many entities it makes.
struct prefab_cooked {
	const char *text;
	uint32_t entities;
};

// file read into a fresh project world in scratch and cooked as prefab_<n>, the
// text copied into arena and scratch rewound. False with why naming file on a
// refused read or cook, or more than VOE_GAME_PREFAB_ENTITIES entities.
static bool prefab_cook(const voe_editor_project *project, const char *file,
			uint32_t n, voe_base_arena *scratch,
			voe_base_arena *arena, struct prefab_cooked *out,
			voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	const uint8_t *bytes;
	size_t size;
	voe_ecs_world *world;
	voe_authoring_kept kept;
	voe_authoring_text cooked;
	bool done = false;

	VOE_BASE_ASSERT(file != NULL && out != NULL, "cooking no prefab");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(file, scratch, &size, NULL);
	if (bytes != NULL) {
		world = voe_editor_project_world_new(project, scratch);
		done = voe_authoring_scene_read((const char *)bytes, size, world,
						scratch, &kept) &&
		       voe_authoring_prefab_cook(world, format(scratch, "prefab_%u", n),
						 scratch, &cooked, &out->entities);
	}
	if (!done) {
		voe_editor_notice_from_report(why, file);
	} else if (out->entities > VOE_GAME_PREFAB_ENTITIES) {
		voe_editor_notice_set(why, "%s makes %u entities, more than %d", file,
				      out->entities, VOE_GAME_PREFAB_ENTITIES);
		done = false;
	} else {
		out->text = format(arena, "%.*s", (int)cooked.size, cooked.text);
	}
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(voe_base_arena_mark(scratch).used == mark.used,
			"a prefab cook kept scratch");
	return done;
}

// `#include <game/prefabs.h>`, Code/'s includes, one function per prefab, then
// the table; count 0 with no entries when there is none. NULL with why on a
// prefab that will not find, read or cook.
static const char *prefabs_source(const voe_editor_project *project,
				  voe_base_arena *arena, voe_editor_notice *why)
{
	const char *assets = voe_platform_path_join(arena, project->folder, "Assets");
	voe_base_arena *scratch;
	struct prefab_paths paths;
	struct prefab_cooked cooked;
	const char *functions = "";
	const char *entries = "";
	bool done = true;

	VOE_BASE_ASSERT(why != NULL, "cooking prefabs with nowhere to say why");
	if (!prefabs_find(assets, arena, &paths, why))
		return NULL;
	if (paths.count > 0)
		qsort(paths.items, paths.count, sizeof *paths.items, prefab_path_order);
	scratch = voe_base_arena_new(PREFAB_SCRATCH);
	for (uint32_t i = 0; i < paths.count; i++) {
		const char *path = paths.items[i].path;

		done = prefab_cook(project, voe_platform_path_join(arena, assets, path),
				   i, scratch, arena, &cooked, why);
		if (!done)
			break;
		functions = format(arena, "%s%s\n", functions, cooked.text);
		entries = format(arena, "%s\t\t{ \"%s\", %u, prefab_%u },\n", entries,
				 escaped(arena, format(arena, "%.*s", (int)(strlen(path) - 7),
						       path), false),
				 cooked.entities, i);
	}
	voe_base_arena_destroy(scratch);
	if (!done)
		return NULL;
	return format(arena,
		      "#include <game/prefabs.h>\n%s\n%s%s", code_includes(project->folder, arena),
		      functions,
		      paths.count == 0
			      ? "const voe_game_prefabs voe_game_prefabs_cooked = { .count = 0 };\n"
			      : format(arena,
				       "const voe_game_prefabs voe_game_prefabs_cooked = {\n"
				       "\t(const voe_game_prefab[]){\n%s\t},\n\t%u,\n};\n",
				       entries, paths.count));
}

static const char *main_source(const voe_editor_project *project,
			       voe_base_arena *arena)
{
	const char *name = voe_editor_project_name(project);
	voe_authoring_project_window window = project->file.window;

	VOE_BASE_ASSERT(name != NULL, "a game tree for an untitled project");
	VOE_BASE_ASSERT(window.width > 0 && window.height > 0, "a game window of no size");
	return format(arena,
		      "#include <game/run.h>\n\n"
		      "int main(void)\n{\n\treturn voe_game_run(\"%s\",\n"
		      "\t\t(voe_game_window){ %d, %d, %s });\n}\n",
		      escaped(arena, name, false), window.width, window.height,
		      window.fullscreen ? "true" : "false");
}

// The four files, each in <folder>/Build/game/, the scene and prefabs cooked
// last.
static bool game_files_write(const voe_editor_project *project,
			     const char *game, voe_base_arena *arena,
			     voe_editor_notice *why)
{
	const char *scene = voe_platform_path_join(arena, game, "scene.c");
	const char *prefabs;
	voe_authoring_text cooked;

	if (!file_write_changed(voe_platform_path_join(arena, game, "CMakeLists.txt"),
				cmake_lists(project, arena), arena, why))
		return false;
	if (!file_write_changed(voe_platform_path_join(arena, game, "main.c"),
				main_source(project, arena), arena, why))
		return false;
	if (!voe_authoring_scene_cook(project->world, "game/scene.h",
				      "voe_game_scene_build", arena, &cooked)) {
		voe_editor_notice_from_report(why, scene);
		return false;
	}
	if (!file_write_changed(scene, scene_source(project->folder, &cooked, arena),
				arena, why))
		return false;
	prefabs = prefabs_source(project, arena, why);
	return prefabs != NULL &&
	       file_write_changed(voe_platform_path_join(arena, game, "prefabs.c"),
				  prefabs, arena, why);
}

bool voe_editor_game_tree_write(const voe_editor_project *project,
				voe_base_arena *arena, voe_editor_notice *why)
{
	const char *build;
	const char *game;
	const char *ignore;

	VOE_BASE_ASSERT(project != NULL && project->folder != NULL,
			"a game tree for no project folder");
	VOE_BASE_ASSERT(arena != NULL && why != NULL, "a game tree with no arena or notice");
	voe_base_report_error_clear();

	build = voe_platform_path_join(arena, project->folder, "Build");
	game = voe_platform_path_join(arena, build, "game");
	if (!folder_ensure(build, arena, why) || !folder_ensure(game, arena, why))
		return false;
	if (!game_files_write(project, game, arena, why))
		return false;

	ignore = voe_platform_path_join(arena, project->folder, ".gitignore");
	if (voe_platform_file_exists(ignore))
		return true;
	return file_write_changed(ignore, "/Build/\n/Cache/\n", arena, why);
}

// <folder>/Build/<name>.
static const char *build_path(const char *folder, const char *name,
			      voe_base_arena *arena)
{
	VOE_BASE_ASSERT(folder != NULL, "a game tree path in no folder");
	VOE_BASE_ASSERT(arena != NULL, "a game tree path in no arena");
	return voe_platform_path_join(arena,
				      voe_platform_path_join(arena, folder, "Build"),
				      name);
}

// Where kind is built: Build/debug for GAME, Build/editor for LIBRARY,
// Build/release for RELEASE.
static const char *binary_folder(const char *folder,
				 voe_editor_game_tree_kind kind,
				 voe_base_arena *arena)
{
	static const char *const names[] = {
		[VOE_EDITOR_GAME_TREE_GAME] = "debug",
		[VOE_EDITOR_GAME_TREE_LIBRARY] = "editor",
		[VOE_EDITOR_GAME_TREE_RELEASE] = "release",
	};

	VOE_BASE_ASSERT(kind >= VOE_EDITOR_GAME_TREE_GAME &&
				kind <= VOE_EDITOR_GAME_TREE_RELEASE,
			"a game tree of no kind");
	return build_path(folder, names[kind], arena);
}

bool voe_editor_game_tree_configured(const char *folder,
				     voe_editor_game_tree_kind kind,
				     voe_base_arena *arena)
{
	return voe_platform_file_exists(voe_platform_path_join(
		arena, binary_folder(folder, kind, arena), "CMakeCache.txt"));
}

static void argument_add(arguments *list, const char *argument)
{
	VOE_BASE_ASSERT(list->count < ARGUMENTS - 1, "a game tree argument list overflowed");
	VOE_BASE_ASSERT(argument != NULL, "a NULL game tree argument");
	list->items[list->count++] = argument;
	list->items[list->count] = NULL;
}

// "-D<name>=<value>" added when value is not empty; empty is a -D left out.
static void define_add(arguments *list, voe_base_arena *arena, const char *name,
		       const char *value)
{
	if (value[0] != '\0')
		argument_add(list, format(arena, "-D%s=%s", name, value));
}

static arguments *arguments_new(voe_base_arena *arena)
{
	arguments *list = voe_base_arena_push(arena, sizeof *list);

	VOE_BASE_ASSERT(list != NULL, "no room for a game tree argument list");
	*list = (arguments){0};
	argument_add(list, VOE_TOOLCHAIN_CMAKE);
	return list;
}

const char *const *voe_editor_game_tree_configure(const char *folder,
						  voe_editor_game_tree_kind kind,
						  voe_base_arena *arena)
{
	const char *binary = binary_folder(folder, kind, arena);
	arguments *list = arguments_new(arena);

	argument_add(list, "-S");
	argument_add(list, build_path(folder, "game", arena));
	argument_add(list, "-B");
	argument_add(list, binary);
	argument_add(list, "-G");
	argument_add(list, "Ninja");
	argument_add(list, kind == VOE_EDITOR_GAME_TREE_RELEASE
				   ? "-DCMAKE_BUILD_TYPE=Release"
				   : "-DCMAKE_BUILD_TYPE=Debug");
	define_add(list, arena, "CMAKE_C_COMPILER", VOE_TOOLCHAIN_C_COMPILER);
	define_add(list, arena, "CMAKE_MAKE_PROGRAM", VOE_TOOLCHAIN_MAKE_PROGRAM);
	define_add(list, arena, "PKG_CONFIG_EXECUTABLE", VOE_TOOLCHAIN_PKG_CONFIG);
	define_add(list, arena, "VOE_SLANGC", VOE_TOOLCHAIN_SLANGC);
	define_add(list, arena, "VOE_WAYLAND_SCANNER", VOE_TOOLCHAIN_WAYLAND_SCANNER);
	if (kind == VOE_EDITOR_GAME_TREE_LIBRARY) {
		argument_add(list, "-DVOE_GAME_LIBRARY=ON");
		define_add(list, arena, "VOE_EDITOR_IMPORTS",
			   VOE_TOOLCHAIN_EDITOR_IMPORTS);
	}
	return list->items;
}

const char *const *voe_editor_game_tree_build(const char *folder,
					      voe_editor_game_tree_kind kind,
					      voe_base_arena *arena)
{
	const char *binary = binary_folder(folder, kind, arena);
	arguments *list = arguments_new(arena);

	argument_add(list, "--build");
	argument_add(list, binary);
	argument_add(list, "--target");
	argument_add(list, kind == VOE_EDITOR_GAME_TREE_LIBRARY ? "project" : "game");
	return list->items;
}

const char *voe_editor_game_tree_shipped(const char *folder, voe_base_arena *arena)
{
	const char *name = voe_platform_path_name(folder);

	VOE_BASE_ASSERT(name != NULL && name[0] != '\0', "shipping a folder with no name");
	return voe_platform_path_join(arena, build_path(folder, "ship", arena), name);
}

const char *const *voe_editor_game_tree_ship_clear(const char *folder,
						   voe_base_arena *arena)
{
	const char *shipped = voe_editor_game_tree_shipped(folder, arena);
	arguments *list = arguments_new(arena);

	argument_add(list, "-E");
	argument_add(list, "rm");
	argument_add(list, "-rf");
	argument_add(list, shipped);
	return list->items;
}

const char *const *voe_editor_game_tree_install(const char *folder,
						voe_base_arena *arena)
{
	const char *shipped = voe_editor_game_tree_shipped(folder, arena);
	arguments *list = arguments_new(arena);

	argument_add(list, "--install");
	argument_add(list, binary_folder(folder, VOE_EDITOR_GAME_TREE_RELEASE, arena));
	argument_add(list, "--prefix");
	argument_add(list, shipped);
	return list->items;
}

const char *voe_editor_game_tree_program(const char *folder,
					 voe_base_arena *arena)
{
#ifdef _WIN32
	const char *name = "game.exe";
#else
	const char *name = "game";
#endif
	return voe_platform_path_join(arena, build_path(folder, "debug", arena), name);
}

const char *voe_editor_game_tree_library(const char *folder,
					 voe_base_arena *arena)
{
#ifdef _WIN32
	const char *name = "project.dll";
#else
	const char *name = "libproject.so";
#endif
	return voe_platform_path_join(arena, build_path(folder, "editor", arena), name);
}

const char *voe_editor_game_tree_log(const char *folder, voe_base_arena *arena)
{
	return build_path(folder, "build.log", arena);
}

bool voe_editor_game_tree_has_code(const char *folder, voe_base_arena *arena)
{
	voe_platform_folder_listing code = code_list(folder, arena);

	for (uint32_t i = 0; i < code.count; i++) {
		if (entry_ends(&code.entries[i], ".c"))
			return true;
	}
	return false;
}
