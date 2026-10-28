// The dock layout a person left, remembered in `<settings>/voe3d/editor_settings`
// beside settings.h's keys (0491 point 7): the tree as one line per node and
// each moving panel's floating box as one line, read back at start.
//
//     if (voe_editor_settings_layout_read(&tree, floats) &&
//         voe_editor_dock_check(&tree, floats))   // else the default tree
//
// THE LINES: `dock_root <i>`; `dock_node <i> split <row|column>
// <fraction|first|second> <length> <fraction> <first> <second>`, the length
// `%.3f` mm and the fraction `%.6f`; `dock_node <i> leaf <panel>`, a view
// `dock_node <i> leaf view <0|1>`; `dock_float <panel> <0|1> <x> <y> <wide>
// <high>`, each `%.3f` mm from the dock area's top left. The panel words are
// `scene assets inspector view frame project errors landscape`. One flat line
// per node, so reading never nests (rule 14).
//
// A FIRST START IS NOT A FAILURE, as settings.h says. A missing file or no
// layout lines reads as no tree, and a float line that does not parse, or
// names no moving panel, leaves that panel's entry as the caller set it;
// nothing is reported. The reader does not check the tree:
// voe_editor_dock_check does. A write that fails is reported at the site.
//
// Its own file because scene.h includes settings.h, so settings.h cannot
// include dock.h. It shares settings.h's line read and keep-other-keys write.
#pragma once

#include "dock.h"

// Empties `tree`, then reads the `dock_root` and `dock_node` lines into it
// and each `dock_float` line that parses over that panel's entry in `floats`.
// True only when a root line was read and every node line parsed, a node's
// index under VOE_EDITOR_DOCK_NODES; the tree is not checked.
bool voe_editor_settings_layout_read(
	voe_editor_dock_tree *tree,
	voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT]);

// Re-reads the file, keeps every other key's line in its order, then writes
// the root, the nodes and one float line per moving panel. False on any
// failure, reported through base/report.h at the site.
[[nodiscard]] bool voe_editor_settings_layout_write(
	const voe_editor_dock_tree *tree,
	const voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT]);
