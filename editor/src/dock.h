// Where the editor's panels are, as a tree of data (ADR-0142). A node either
// divides the space it was given in two or names the one panel that fills it,
// and the walk turns that tree into `ui` rows, columns and panels — so where a
// panel sits is a number in an array, and not the order of the calls in a
// function.
//
// AND THAT IS WORTH A TREE BEFORE ANYTHING CAN BE DRAGGED, WHICH IS THE THING A
// READER ARRIVING TODAY WILL DOUBT. Nothing here is rearrangeable: no splitter,
// no tab bar, no closing a panel, nothing saved. An interface laid out by call
// order looks identical on screen and costs less to write. The difference is
// what it takes to CHANGE it — with call order, a splitter has to rewrite the
// function that draws the frame, which is the function every panel's contents
// are also in; with a tree, a splitter writes one `fraction` and this file is
// untouched. The tree is here first so that the thing that comes next is an
// edit to one number rather than a rewrite of everything around it.
//
// THE FRACTION IS THE NUMBER A SPLITTER WILL LATER WRITE BACK. Nothing writes
// one today: `voe_editor_dock_default` builds the tree and no code path changes
// it after that. It is the whole of what a drag would have to touch.
//
// NO FUNCTION POINTER LIVES IN THIS FOLDER. A leaf names a panel from the
// enumeration below and `voe_editor_panel_draw` is one function with a `switch`
// over it, the way `app` keeps callbacks out of the frame loop (ADR-0135). A
// table of draw functions would be the shorter spelling and it would make the
// set of panels something a reader has to assemble from call sites.
#pragma once

#include "scene.h"
#include "view.h"

#include <math/float2.h>
#include <ui/layout.h>
#include <ui/widgets.h>

#include <stdint.h>

// The panels there are. A panel is one of these and nothing else: it is not a
// string, it is not registered anywhere, and there is no table to add a row to.
// A new panel is a value here, a case in `voe_editor_panel_draw`, and a leaf
// that names it.
typedef enum {
	VOE_EDITOR_PANEL_SCENE,
	VOE_EDITOR_PANEL_INSPECTOR,
	VOE_EDITOR_PANEL_SCENE_VIEW,
	VOE_EDITOR_PANEL_COUNT
} voe_editor_panel;

// Which way a split divides what it was given. A ROW puts its two children side
// by side and a COLUMN stacks them, which is what the two words mean in
// `ui/layout.h` and is deliberately not a second vocabulary.
typedef enum {
	VOE_EDITOR_DOCK_ROW,
	VOE_EDITOR_DOCK_COLUMN
} voe_editor_dock_axis;

typedef enum {
	VOE_EDITOR_DOCK_SPLIT,
	VOE_EDITOR_DOCK_LEAF
} voe_editor_dock_kind;

// How many nodes one tree may hold, and how deep the walk may go before it is
// certain something is wrong. A dock tree is a handful of panels: sixteen is
// already more splits than a person can point at, and the depth is asserted
// rather than returned because a tree deeper than this is either a cycle in the
// indices or a tree nobody built on purpose — the program's own bad value, which
// is rule 13's assert side.
#define VOE_EDITOR_DOCK_NODES 16
#define VOE_EDITOR_DOCK_DEPTH 16

// One node. A SPLIT reads `axis`, `fraction`, `first` and `second`; a LEAF reads
// `panel`, and `view` as well when that panel is SCENE_VIEW — which of the
// editor's views it shows, an index into voe_editor_views and not a view itself,
// so a tree still holds nothing but numbers. The two are one struct rather than a union because a dock tree is
// sixteen of these and telling a reader which fields are live is what `kind` is
// for.
//
// `first` AND `second` ARE INDICES INTO THE TREE'S OWN ARRAY AND NOT POINTERS.
// A tree is therefore a plain value that can be copied, compared and one day
// written to a file, and a bad index is caught by the walk rather than by a
// segmentation fault.
//
// `fraction` is how much of the split's length the FIRST child gets, between
// nought and one exclusive. Two children and one number: there is no list of
// weights, because a splitter drags one boundary at a time.
typedef struct {
	voe_editor_dock_kind kind;
	voe_editor_dock_axis axis;
	double fraction;
	uint32_t first;
	uint32_t second;
	voe_editor_panel panel;
	uint32_t view;
} voe_editor_dock_node;

// A whole tree: its nodes, how many of them are live, and which one is the root.
typedef struct {
	voe_editor_dock_node nodes[VOE_EDITOR_DOCK_NODES];
	uint32_t count;
	uint32_t root;
} voe_editor_dock_tree;

// A surface with a tree on it: how big it is in the surface's own millimetres,
// and the pointer in those same millimetres. Both are the caller's to fill in —
// nothing in this folder asks a window anything (ADR-0141 point 4).
//
// ROOT ZERO IS THE WINDOW, AND A DETACHED PANEL WOULD BE A FURTHER ROOT. The
// editor holds an array of them and the loop walks every one, so the shape is
// already here.
//
// THAT DOES NOT MAKE A SECOND OS WINDOW CHEAP, AND SAYING SO IS THE POINT OF
// THIS PARAGRAPH. `platform` opens one window, `render` opens one device onto
// one surface and `app` brackets one frame for it; a second root that is its own
// window is a card in each of those three folders before it is a line here. What
// the array buys today is that the walk takes a root rather than reading a
// global, which is the part that would otherwise have to be undone.
typedef struct {
	voe_editor_dock_tree tree;
	voe_math_float2 size;
	voe_ui_pointer pointer;
} voe_editor_dock_root;

// The tree the editor opens on: three columns — `Scene` a fifth of the width on
// the left, the two scene views stacked half and half in the middle three
// fifths, `Inspector` the last fifth on the right. Which view is on top is one
// number on one leaf, which is what having a tree at all is for.
voe_editor_dock_tree voe_editor_dock_default(void);

// Whether a leaf in `tree` shows view `view`. The loop asks before it draws a
// view, because a view nobody shows is not drawn — see view.h.
bool voe_editor_dock_shows_view(const voe_editor_dock_tree *tree, uint32_t view);

// Emits `root`'s tree into `ui` as this frame's whole interface: a split becomes
// a row or a column whose two children are given fixed sizes in millimetres, and
// a leaf becomes a panel with `voe_editor_panel_draw`'s contents in it.
//
// It is the whole of one `ui` frame and it opens the frame's root container, so
// it is called between voe_ui_frame_begin and voe_ui_frame_end and once per
// root — see interface.h on why a root is a frame.
//
// `scene` is what the panels read and write: the list is its identity table and
// the selection is its own (scene.h). `views` is where a scene view's panel
// finds its picture and records where the picture sat, and passes through on the
// same terms as `scene`. IT PASSES STRAIGHT THROUGH AND THE TREE
// NEVER LOOKS AT IT — no node holds one, no split reads one, and a tree built
// for one scene lays out identically for another. The parameter is here because
// voe_editor_panel_draw is called from inside the walk and a panel's contents
// are ordinary C (ADR-0142 point 2); handing it in is the alternative to this
// folder reaching for a global.
void voe_editor_dock_walk(const voe_editor_dock_root *root, voe_ui_context *ui,
			  voe_editor_scene *scene, voe_editor_views *views);

// What is on one panel. One function, one `switch`, no table and no function
// pointer; a panel's contents are ordinary `ui` calls and the tree above owns
// only the geometry (ADR-0142 point 2). Called from inside the leaf's panel, so
// everything it emits is a child of that panel.
//
// `view` is the leaf's own and is read only for a SCENE_VIEW panel.
void voe_editor_panel_draw(voe_ui_context *ui, voe_editor_panel panel,
			   uint32_t view, voe_editor_scene *scene,
			   voe_editor_views *views);
