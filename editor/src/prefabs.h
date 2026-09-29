// What the editor does with prefabs: every placed copy in a world expanded from
// its file (ADR-0283 point 4), and a tree made one.
//
// A PLACED COPY IS SAVED AS ITS ROOT ALONE, so the world a scene text is read
// into holds roots with a voe_scene_prefab row and nothing under them. Expanding
// reads `<folder>/<path>` for every root with a prefab row and no part row
// (scene/prefab_component.h), in ascending identity id, and hands it to
// voe_authoring_prefab_read: the root gains the prefab root's rows it lacks, the
// rest of the tree is made with ids above every id in the world, and every one
// gets a part row. The first id is one above the largest in the world, carried
// from one root to the next.
//
// IT IS A LOAD, rule 3's creation exception, as a scene read is: rows are added
// straight to the world, no system is asked and no intent submitted. So it runs
// only between a scene read or the world step and anything that reads the
// world, never while a system or a panel holds a row pointer.
//
// THE IDS ARE DETERMINISTIC for one text and one set of files: the same roots in
// the same order from the same first id. That is what lets undo, which reads a
// text back and expands it again, re-find a selected part by its id.
//
// A FILE THAT WILL NOT READ, OR THAT THE READER REFUSES, says
// `Could not read <path>: <category>` in the notice, and the root still gets
// its part row, so it is not tried again every frame. A root with no transform
// is refused the same way.
//
// A TREE DRAGGED INTO THE ASSETS PANEL BECOMES ONE (0283 point 6): its text is
// written to `Assets/<shown>/<name>.prefab`, then the root gets a prefab row
// and the whole tree part rows naming it, so the original is a placed copy.
// THE FILE IS WRITTEN BEFORE THE ROWS, so a failed write changes nothing, and
// undoing the step leaves the file.
//
// Constraints: an untitled project (folder NULL) expands nothing. Each root is
// found by a scan of the prefab table, so a world of n roots costs n² checks;
// a sorted copy of the table would lift it, and 32 identities make it moot.
#pragma once

#include "notice.h"
#include "project.h"

#include <base/arena.h>

#include <ecs/world.h>

// Expands every unexpanded placed copy in world from its file under folder. The
// scratch is rewound to where it was; a failure is said in why and the rest are
// still expanded.
void voe_editor_prefabs_expand(voe_ecs_world *world, const char *folder,
			       voe_base_arena *scratch, voe_editor_notice *why);

// True, with why, when the tree under root holds a camera, a light, a prefab
// row or a part row: what no prefab may hold.
[[nodiscard]] bool voe_editor_prefab_refused(const voe_ecs_world *world,
					     voe_ecs_entity root,
					     voe_editor_notice *why);

// Makes a prefab of root's tree in project's `Assets/<shown>/` (`shown` the
// Assets panel's folder relative to `Assets/`). Refused with why when the
// project is untitled, root has no identity, _refused says so or the file
// exists; else the file is written and the rows queued. False with why on any
// failure. The scratch is rewound to where it was.
[[nodiscard]] bool voe_editor_prefab_make(voe_editor_project *project,
					  voe_ecs_entity root,
					  const char *shown,
					  voe_base_arena *scratch,
					  voe_editor_notice *why);
