// `voe_editor --check [<folder>]`: a project's files read strictly, with no
// window and no device (ADR-0511 point 4), so an agent that wrote a file can
// ask whether the editor would take it.
//
//     if (options.check)
//             return voe_editor_check(options.folder);    // the exit code
//
// WHAT IS READ is assets_walk.h's list: every `.scene`, `.prefab`,
// `.material`, `.landscape`, `.parts` and `.tree` under the folder and its top-level
// `project.voe3d`, each with the strict reader the editor takes that kind in
// with (ADR-0509 point 3). A prefab is also read into a fresh world and refused
// unless it is one tree prefabs.h's refusal lets through, as its save is.
//
// THE PROJECT'S OWN COMPONENTS ARE CHECKED ONLY WHEN ITS CODE IS BUILT. A
// section of a type nothing registered passes the scene check, so the world is
// game/world.h's plus the library in `Build/editor/` when one is there, loaded
// as code.h's load 0; a `Code/` with no library says so on one line.
//
// WHAT IT SAYS, on stdout: one `<path>: <why>` line per refused file, the path
// project-relative and the why the reader's first kept error (base/report.h),
// then `all is well: N files checked` or `K of N files refused`. Stderr carries
// the readers' own reports. 0 when all is well; 1 when a file is refused, the
// folder will not list, or it holds no `project.voe3d`.
//
// Constraints: every file is read whole, one at a time, into one arena rewound
// after each.
#pragma once

// Checks the project at folder, NULL the current folder, and returns the exit
// code.
int voe_editor_check(const char *folder);
