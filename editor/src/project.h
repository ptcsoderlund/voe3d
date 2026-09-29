// The project being worked on: its own arena, the world in an arena of its
// own, the kept sections it was read with, the code its world was made with,
// its absolute folder (NULL when untitled) and whether it has unsaved changes
// (ADR-0164).
//
// THREE ARENAS, AND voe_editor_project_destroy IS THE ONLY WAY THEY GO BACK.
// The struct and the folder path live in arena, so New and Open build a whole
// new project, then discard the old one. The world and its tables live in
// world_arena. The kept sections and a read's working memory live in
// scene_arena, cleared by every voe_editor_project_scene_set, which reads
// into the same world again and again. The code is closed after all three.
//
// THE WORLD IS MADE IN ONE PLACE: game/world.h's engine types, then the
// code's own when there is one, whether untitled, opened or swapped.
//
// voe_editor_project_code_set IS A WHOLE NEW WORLD (ADR-0242 point 6), the
// only swap ecs allows: the old world's text is read into a world made with
// the new code, and only then are the old arenas destroyed and the old code
// closed. A type the new code lacks survives as a kept section (0241).
//
// EVERY SCENE TEXT READ INTO A WORLD IS EXPANDED (prefabs.h): new_opened,
// scene_set and code_set each expand every placed copy from the project's
// folder right after a successful read. A prefab that will not read is said
// in why and does not fail the call.
//
// voe_editor_project_new_opened READS project.voe3d, THEN THE SCENE IT NAMES.
// Each step clears base/report.h's kept error first, so a failure's notice is
// that step's own and not one left over from before it. A missing
// project.voe3d is worded "is not a project" rather than repeating whatever
// the operating system said about opening a file that is not there — that
// wording is what a person picking a folder in Open needs to hear. Every
// other failure — a malformed project.voe3d, a scene file that will not open,
// a scene that voe_authoring_scene_read refuses, or a world too small to hold
// it (the same failure, since scene_read reports it the same way) — keeps the
// report's own message, naming the file the step was on. On any failure the
// project's arena is destroyed and NULL comes back; the caller's own project,
// if it has one, is untouched.
//
// voe_editor_project_save WRITES ONLY WHAT THIS PROJECT HAS. An opened
// project (folder is NULL here) writes just the scene, over the file it was
// read from. An untitled project (folder is required) refuses a folder that
// is not empty, then writes main.scene and project.voe3d and adopts the
// folder as its own. Scratch text for both cases comes from a scratch arena
// made and destroyed inside the call — none of it is the project's to keep,
// because the scene text and the project text are turned into files before
// this returns.
//
// THE SCENE FILE IS ALWAYS NAMED main.scene. This is the one project layout
// this editor ever writes (ADR-0164); voe_editor_project_new_opened still
// reads whatever project.voe3d names, because a hand-made or future project
// need not agree.
//
// A PREFAB OPENED IN PLACE OF THE LEVEL (0283 point 8) sets the level aside as
// its scene text, kept sections and unsaved edits included, with its unsaved
// flag, in an arena of its own. Text, because Back reads it through scene_set,
// so every copy is expanded from the prefab files as they are then. Save writes
// the world to `<folder>/<prefab>` alone. code_set swaps only the world, so a
// Refresh keeps both the open prefab and the set-aside level.
#pragma once

#include "code.h"
#include "notice.h"

#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <game/world.h>

#include <scene/prefab_component.h>

#include <stdbool.h>
#include <stddef.h>

// The room a project's world has for drawn entities is
// VOE_GAME_WORLD_MAX_DRAWN, which lives in game/world.h beside the world every
// project and the game build (0237); a device that draws a project's shapes
// sizes its voe_render_capacities from it rather than a second copy.

typedef struct {
	voe_base_arena *arena;
	// The world's own, replaced whole by voe_editor_project_code_set.
	voe_base_arena *world_arena;
	// What a scene read owns: the kept sections below and the read's own
	// working memory, cleared by every re-read — see the header.
	voe_base_arena *scene_arena;
	voe_ecs_world *world;
	// The sections authoring/scene_read.h read and did not understand,
	// read back into voe_authoring_scene_write on every save so a file
	// this program does not fully know survives a trip through it.
	voe_authoring_kept kept;
	// The code the world's project types came from; zeroed while none.
	// The project closes it, after the world it registered into.
	voe_editor_code code;
	// The project's absolute folder, in arena, or NULL for the untitled
	// project: nothing has been saved yet to name one.
	const char *folder;
	bool unsaved;
	// The open prefab's path under folder, or "" while the level is open.
	char prefab[VOE_SCENE_PREFAB_PATH];
	// The level set aside while a prefab is open: its arena (NULL when
	// none), its scene text in it and its unsaved flag.
	voe_base_arena *level_arena;
	const char *level_text;
	size_t level_size;
	bool level_unsaved;
} voe_editor_project;

// The untitled scene: one cube, the light that shows it and the scene's one
// camera, in a fresh world, with no folder. Cannot fail: nothing here uploads
// to a device or opens a file, and the room the three entities need is this
// file's own to size.
voe_editor_project *voe_editor_project_new_untitled(void);

// Opens the project at folder. NULL on failure, with why naming the file the
// failing step was on and what was wrong with it — see the header above. A
// scene with no camera (written before 0218) is given one, with an id above
// every id it holds, and the project comes back marked unsaved. Its placed
// copies are then expanded; one that will not read is said in why.
[[nodiscard]] voe_editor_project *voe_editor_project_new_opened(const char *folder,
								 voe_editor_notice *why);

// Saves project. folder is NULL for an opened project (folder is already set)
// and required for an untitled one. False on failure, with why saying why and
// nothing written; true clears project->unsaved and, for an untitled project,
// sets project->folder to folder. With a prefab open it writes the prefab,
// refusing a world without exactly one entity with no parent, or one that
// voe_editor_prefab_refused (prefabs.h) refuses.
[[nodiscard]] bool voe_editor_project_save(voe_editor_project *project,
					   const char *folder,
					   voe_editor_notice *why);

// The world and project->kept written into arena as scene text — the same
// bytes a save writes to the scene file. False when
// voe_authoring_scene_write refuses, which it reports itself and nothing is
// added to here, so clear base/report.h before the call to explain it.
[[nodiscard]] bool voe_editor_project_scene_text(const voe_editor_project *project,
						 voe_base_arena *arena,
						 voe_authoring_text *out);

// Makes the world hold what `size` bytes of `text` say: every authored entity
// destroyed through the structural queue and applied, then the text read into
// that same world, then its placed copies expanded. True with the world holding
// exactly what the text says, a copy that would not expand said in why; false
// with why filled from the report.
//
// THIS IS THE ONE CALL IN THIS PROGRAM THAT EMPTIES A WORLD, AND IT IS NOT
// New. Every registration, every table, the project's folder and its unsaved
// flag are left exactly as they were; only which entities exist changes. No
// entity handle survives it, so a caller holding one re-finds what it wants by
// authored id. It is called between frames, before the structural queue is
// applied and the systems run, so the rows put back are given their meshes
// before anything draws them.
//
// A REFUSAL CAN ONLY BE A TEXT THIS PROGRAM DID NOT WRITE ITSELF, since a
// world's own text always fits back into the world it came from. The world is
// left half-loaded, and the caller says so rather than carrying on as though
// nothing had happened.
[[nodiscard]] bool voe_editor_project_scene_set(voe_editor_project *project,
						const char *text, size_t size,
						voe_editor_notice *why);

// The world swapped for one made with code, holding what the old one did: its
// text, kept sections included, read into a new world in new arenas, then the
// old arenas destroyed and the old code closed; the new world's placed copies
// are expanded, one that will not said in why. The project takes code
// whatever happens; false, with why from the report, closes it and leaves the
// project as it was. unsaved is untouched. Every entity handle is stale on
// true, so the caller re-finds what it holds by authored id.
[[nodiscard]] bool voe_editor_project_code_set(voe_editor_project *project,
					       voe_editor_code code,
					       voe_editor_notice *why);

// Opens the prefab at `<folder>/<path>` in the level's place: the level set
// aside as text, the world made to hold the prefab, prefab set and unsaved
// false. Needs a folder and no prefab open. False with why on any failure, the
// level put back as it was.
[[nodiscard]] bool voe_editor_project_prefab_open(voe_editor_project *project,
						  const char *path,
						  voe_editor_notice *why);

// The set-aside level read back through scene_set, its unsaved flag restored,
// its arena destroyed and prefab cleared. Needs a prefab open. False with why
// when the level's text will not read, the prefab still counted open.
[[nodiscard]] bool voe_editor_project_prefab_back(voe_editor_project *project,
						  voe_editor_notice *why);

// project's folder's own name, or NULL when it is untitled.
const char *voe_editor_project_name(const voe_editor_project *project);

void voe_editor_project_destroy(voe_editor_project *project);
