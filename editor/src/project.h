// The project being worked on: its own arena, the world in it, the kept
// sections it was read with, its absolute folder (NULL when untitled) and
// whether it has unsaved changes (ADR-0164).
//
// THE PROJECT OWNS ITS ARENA, AND NOTHING OUTLIVES IT THAT ISN'T IN IT. The
// world, its entities, the kept sections' text and the folder path all come
// out of the one arena voe_editor_project_new_untitled or
// voe_editor_project_new_opened makes; voe_editor_project_destroy is the only
// way anything in it goes back. This is what makes New and Open "build a
// whole new project, then discard the old one" rather than an edit to the
// world in place — the struct itself even lives inside its own arena.
//
// A SCENE READ OWNS AN ARENA OF ITS OWN. The kept sections and the read's own
// working memory go in scene_arena and nothing else does, because
// voe_editor_project_scene_set reads a scene into the same world again and
// again and a re-read's kept sections cannot be pushed on top of the last
// read's for ever. The world's tables were allocated out of the project's
// arena before the first read, so clearing this one is always right.
//
// THE WORLD'S REGISTRATIONS ARE THIS FILE'S, NOT main.c'S. Every project's
// world holds the same eight component types with the same room — transform,
// identity, light, camera, shape, mesh, material, panel — whether it is the
// untitled scene or one read off disk, so there is exactly one place that
// decides the capacities and calls the eight _register functions.
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
#pragma once

#include "notice.h"

#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <game/world.h>

#include <stdbool.h>
#include <stddef.h>

// The room a project's world has for drawn entities is
// VOE_GAME_WORLD_MAX_DRAWN, which lives in game/world.h beside the world every
// project and the game build (0237); a device that draws a project's shapes
// sizes its voe_render_capacities from it rather than a second copy.

typedef struct {
	voe_base_arena *arena;
	// What a scene read owns: the kept sections below and the read's own
	// working memory, cleared by every re-read — see the header.
	voe_base_arena *scene_arena;
	voe_ecs_world *world;
	// The sections authoring/scene_read.h read and did not understand,
	// read back into voe_authoring_scene_write on every save so a file
	// this program does not fully know survives a trip through it.
	voe_authoring_kept kept;
	// The project's absolute folder, in arena, or NULL for the untitled
	// project: nothing has been saved yet to name one.
	const char *folder;
	bool unsaved;
} voe_editor_project;

// The untitled scene: one cube, the light that shows it and the scene's one
// camera, in a fresh world, with no folder. Cannot fail: nothing here uploads
// to a device or opens a file, and the room the three entities need is this
// file's own to size.
voe_editor_project *voe_editor_project_new_untitled(void);

// Opens the project at folder. NULL on failure, with why naming the file the
// failing step was on and what was wrong with it — see the header above. A
// scene with no camera (written before 0218) is given one, with an id above
// every id it holds, and the project comes back marked unsaved.
[[nodiscard]] voe_editor_project *voe_editor_project_new_opened(const char *folder,
								 voe_editor_notice *why);

// Saves project. folder is NULL for an opened project (folder is already set)
// and required for an untitled one. False on failure, with why saying why and
// nothing written; true clears project->unsaved and, for an untitled project,
// sets project->folder to folder.
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
// that same world. True with the world holding exactly what the text says;
// false with why filled from the report.
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

// project's folder's own name, or NULL when it is untitled.
const char *voe_editor_project_name(const voe_editor_project *project);

void voe_editor_project_destroy(voe_editor_project *project);
