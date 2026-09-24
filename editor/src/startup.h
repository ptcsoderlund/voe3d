// Which project the editor opens on, and the one line saying whether field
// descriptions reached the binary. main.c calls both once, before the device.
//
// WHICH PROJECT IT OPENS ON IS ARGUED HERE (project.h). The folder on the
// command line (options.h) is opened, and one that cannot be opened prints
// `voe_editor: <why>` on stderr and stops this program before it starts. With
// no folder, last_project.h's remembered path is tried the same way, but its
// failure is not fatal: an untitled cube and light are opened instead and the
// reason goes into session.notice and onto stderr, as at a first start. What is
// open is written back as the last project, unless this is a capture.
#pragma once

#include "options.h"
#include "session.h"

// Fills session->project from options: argued, remembered or untitled, and
// writes it back as the last project unless options->capture is set. False
// only when the argued folder would not open, already said on stderr, and
// session->project is then NULL.
[[nodiscard]] bool voe_editor_startup_project(const voe_editor_options *options,
					      voe_editor_session *session);

// Prints whether field descriptions are compiled in, on stdout.
void voe_editor_startup_say_descriptions(void);
