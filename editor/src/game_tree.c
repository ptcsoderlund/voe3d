// The game tree written and its argument lists built — see game_tree.h.
//
// A file is compared against what is on disk before it is written, so its
// timestamp only moves when its bytes do. Text put into a generated file is
// escaped for where it goes: the project's name as a C string literal in
// main.c, the engine's and the code's paths and the name as quoted CMake
// arguments in CMakeLists.txt. Code/ is looked for in the project's own listing before it is
// listed, so a project with none reports nothing.
//
// An argument list is at most ARGUMENTS entries, the NULL included, in one
// struct pushed into the caller's arena.
#include "game_tree.h"

#include "toolchain.h"

#include <authoring/scene_cook.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ARGUMENTS 16

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

// `#include "<name>.h"` for every .h in Code/, then the cooked scene.
static const char *scene_source(const char *folder,
				const voe_authoring_text *cooked,
				voe_base_arena *arena)
{
	voe_platform_folder_listing code = code_list(folder, arena);
	const char *out = "";

	for (uint32_t i = 0; i < code.count; i++) {
		if (entry_ends(&code.entries[i], ".h"))
			out = format(arena, "%s#include \"%s\"\n", out,
				     escaped(arena, code.entries[i].name, false));
	}
	out = format(arena, "%s%.*s", out, (int)cooked->size, cooked->text);
	VOE_BASE_ASSERT(out[0] != '\0', "a scene with no text");
	return out;
}

static const char *main_source(const voe_editor_project *project,
			       voe_base_arena *arena)
{
	const char *name = voe_editor_project_name(project);

	VOE_BASE_ASSERT(name != NULL, "a game tree for an untitled project");
	return format(arena,
		      "#include <game/run.h>\n\n"
		      "int main(void)\n{\n\treturn voe_game_run(\"%s\");\n}\n",
		      escaped(arena, name, false));
}

// The three files, each in <folder>/Build/game/, the scene cooked last.
static bool game_files_write(const voe_editor_project *project,
			     const char *game, voe_base_arena *arena,
			     voe_editor_notice *why)
{
	const char *scene = voe_platform_path_join(arena, game, "scene.c");
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
	return file_write_changed(scene, scene_source(project->folder, &cooked, arena),
				  arena, why);
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
