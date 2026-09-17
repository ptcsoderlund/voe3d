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
// THE WORLD'S REGISTRATIONS ARE THIS FILE'S, NOT main.c'S. Every project's
// world holds the same seven component types with the same room — transform,
// identity, light, shape, mesh, material, panel — whether it is the untitled
// scene or one read off disk, so there is exactly one place that decides the
// capacities and calls the seven _register functions.
//
// voe_editor_project_new_opened READS project.voe3d, THEN THE SCENE IT NAMES.
// Each step clears base/report.h's kept error first, so a failure's notice is
// that step's own and not one left over from before it. A missing
// project.voe3d is worded "is not a project" rather than repeating whatever
// the operating system said about opening a file that is not there — that
// wording is what a person choosing a folder in Open needs to hear. Every
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

#include <base/arena.h>

#include <ecs/world.h>

#include <stdbool.h>

// The world's own room, and the room a project's scene may hold. Registered
// by every project's world, whether built untitled or read off disk; a device
// that draws a project's shapes sizes its own voe_render_capacities from
// VOE_EDITOR_PROJECT_MAX_DRAWN, so main.c reads it rather than keeping a
// second copy of the number.
#define VOE_EDITOR_PROJECT_MAX_DRAWN 64

typedef struct {
	voe_base_arena *arena;
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

// The untitled scene: one cube and the light that shows it, in a fresh world,
// with no folder. Cannot fail: nothing here uploads to a device or opens a
// file, and the room the two entities need is this file's own to size.
voe_editor_project *voe_editor_project_new_untitled(void);

// Opens the project at folder. NULL on failure, with why naming the file the
// failing step was on and what was wrong with it — see the header above.
[[nodiscard]] voe_editor_project *voe_editor_project_new_opened(const char *folder,
								 voe_editor_notice *why);

// Saves project. folder is NULL for an opened project (folder is already set)
// and required for an untitled one. False on failure, with why saying why and
// nothing written; true clears project->unsaved and, for an untitled project,
// sets project->folder to folder.
[[nodiscard]] bool voe_editor_project_save(voe_editor_project *project,
					   const char *folder,
					   voe_editor_notice *why);

// project's folder's own name, or NULL when it is untitled.
const char *voe_editor_project_name(const voe_editor_project *project);

void voe_editor_project_destroy(voe_editor_project *project);
