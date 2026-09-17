// Which project was open last, remembered per person on the machine and
// nowhere inside a project — `<settings>/voe3d/last_project`, one line, the
// project's absolute folder (ADR-0164). This is the whole of what makes
// starting the editor with no arguments reopen what it had.
//
// A FIRST START IS NOT A FAILURE. voe_editor_last_project_read hands back NULL
// when there is no settings folder or no file, and reports nothing: nobody
// asked "was there a last project" and needs telling there was not one. What
// happens when the path it names can no longer be opened is the caller's to
// decide and report — see project.h.
//
// voe_editor_last_project_write MAKES ITS OWN WAY THERE. `<settings>` may not
// exist yet (a first save, ever) and platform/folder.h's _create makes one
// level at a time, so this makes `<settings>` and then `<settings>/voe3d`
// before it writes the file. It keeps its own scratch arena and needs none
// from a caller.
#pragma once

#include <base/arena.h>

// The one line of `<settings>/voe3d/last_project`, without its trailing
// newline, pushed into arena. NULL when platform/folder.h has no settings
// folder for this machine, or when the file is not there.
const char *voe_editor_last_project_read(voe_base_arena *arena);

// Writes folder as the last project, making `<settings>` and
// `<settings>/voe3d` first if either is missing. False on a failure at any
// step, reported through base/report.h at the site.
[[nodiscard]] bool voe_editor_last_project_write(const char *folder);
