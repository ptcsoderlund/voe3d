// A project's Docs: the three files in `<project>/Docs/` that tell a reader,
// an agent among them, how the project's files are written (0505, 0511 point 3).
//
//     voe_editor_notice why = { 0 };
//     if (!voe_editor_docs_write(project, scratch, &why))
//             ...                     // why names the file or folder
//
// THE EDITOR OWNS EXACTLY THREE FILES THERE:
//
//   - `voe3d-scene.md`, authoring/reference.h's scene text for the project's
//     world, its own components included when its code is loaded;
//   - `voe3d-files.md`, the material, parts, landscape, tree species and
//     project key tables' texts, joined in that order;
//   - `voe3d.md`, the index: which kind is documented where, that the editor
//     rewrites its own three and leaves every other file in Docs/ to the user,
//     that a file changed on disk is taken in (0509), the check command written
//     out whole with its output and exits, and where the engine's headers are.
//
// Each is read first and written only when its bytes differ, so an unchanged
// file keeps its stamp. Nothing else in Docs/ is read, written or removed, and
// no agent instruction file is made or named. An untitled project has no
// folder and writes nothing.
//
// The engine's headers are found under toolchain.h's engine folder, the source
// tree this editor was built from, which is the only place they exist.
//
// Constraints: the index's folder list is built by formatting it again per
// folder, quadratic in the engine's folder count (about twenty); a running
// length would lift it. Everything is pushed into `scratch` and rewound before
// the call returns.
#pragma once

#include "notice.h"
#include "project.h"

#include <base/arena.h>

#include <stdbool.h>

// Writes the three files into `<project->folder>/Docs/`, making Docs/ when it
// is missing; true with nothing written when project->folder is NULL. False
// with why naming the file or folder on a refused scene text, folder listing,
// read or write; the report is cleared first. Files written before a refusal
// stay written.
[[nodiscard]] bool voe_editor_docs_write(const voe_editor_project *project,
					 voe_base_arena *scratch,
					 voe_editor_notice *why);
