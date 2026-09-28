// Where the editor's panels are, as a tree of data (ADR-0142). A node either
// divides the space it was given in two or names the one panel that fills it,
// and the walk turns that tree into `ui` rows, columns and panels — so where a
// panel sits is a number in an array, and not the order of the calls in a
// function.
//
// AND THAT IS WORTH A TREE BEFORE ANYTHING CAN BE DRAGGED, WHICH IS THE THING A
// READER ARRIVING TODAY WILL DOUBT. Nothing here is rearrangeable: no tab bar,
// no moving or closing a panel; only a border's place. An interface laid out by call
// order looks identical on screen and costs less to write. The difference is
// what it takes to CHANGE it — with call order, a splitter has to rewrite the
// function that draws the frame, which is the function every panel's contents
// are also in; with a tree, a splitter writes one `fraction` and this file is
// untouched. The tree is here first so that the thing that comes next is an
// edit to one number rather than a rewrite of everything around it.
//
// A HELD LENGTH IS WHAT A DRAG WRITES, THROUGH resize.h, AND THE TREE IS STILL
// ONLY NUMBERS. A split may hold one child at a length in millimetres (0226);
// `voe_editor_dock_panel_length_set` is the one write, and
// `voe_editor_dock_arrange` says where every node and seam is without a `ui`
// frame, so a border can be hit-tested before anything is drawn. The two scene
// views divide by their split's `fraction`, the views' share (0228, 0229): a
// share and not a length, so it holds as the window changes. Every seam is
// drawn in the border colour, and the lit one as two stripes (0231, 0232).
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
#include <ui/theme.h>
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
	VOE_EDITOR_PANEL_ASSETS,
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

// Which child of a split is held at `length` mm, the other taking the rest.
// FRACTION is zero, so a node that names none divides by `fraction` as before.
typedef enum {
	VOE_EDITOR_DOCK_HOLD_FRACTION,
	VOE_EDITOR_DOCK_HOLD_FIRST,
	VOE_EDITOR_DOCK_HOLD_SECOND
} voe_editor_dock_hold;

// The least a held side panel is laid out at: narrower than 30 mm a panel's
// headings and fields no longer fit a line (0226).
#define VOE_EDITOR_DOCK_PANEL_MIN 30.0f
// The least a scene view keeps along a split's axis, so a panel dragged wide
// never squeezes the picture past being usable; where the two cannot both
// hold, this wins (0226).
#define VOE_EDITOR_DOCK_VIEW_ROOM 40.0f
// The default tree's Scene list and Inspector: a fifth of a 16:9 surface
// (0226), kept in millimetres so a wider window gives the room to the views.
#define VOE_EDITOR_DOCK_SIDE_WIDE 48.0f
// The Assets panel's height under the Scene list (0277 point 6).
#define VOE_EDITOR_DOCK_ASSETS_TALL 70.0f

// One node. A SPLIT reads `axis`, `hold`, `first` and `second`, then `length`
// when it holds a child and `fraction` when it does not; a LEAF reads
// `panel`, and `view` as well when that panel is SCENE_VIEW — which of the
// editor's views it shows, an index into voe_editor_views and not a view itself,
// so a tree still holds nothing but numbers. The two are one struct rather
// than a union because a dock tree is sixteen of these and telling a reader
// which fields are live is what `kind` is for.
//
// `first` AND `second` ARE INDICES INTO THE TREE'S OWN ARRAY AND NOT POINTERS.
// A tree is therefore a plain value that can be copied, compared and one day
// written to a file, and a bad index is caught by the walk rather than by a
// segmentation fault.
//
// `fraction` is how much of the split's length the FIRST child gets, between
// nought and one exclusive. Two children and one number: there is no list of
// weights, because a splitter drags one boundary at a time. `length` is the held
// child's millimetres as a person set them; what is laid out is that clamped
// (voe_editor_dock_arrangement's `shown`), and `length` itself is kept as set.
typedef struct {
	voe_editor_dock_kind kind;
	voe_editor_dock_axis axis;
	double fraction;
	voe_editor_dock_hold hold;
	float length;
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
// the pointer in those same millimetres, and this frame's keyboard (task 14).
// All three are the caller's to fill in — nothing in this folder asks a
// window anything (ADR-0141 point 4). `keyboard` is handed to `ui` beside
// `pointer` (interface.c), which is what lets the browser's own name field
// (browser.h) be typed into without this folder or interface.c knowing a
// field exists. `lit` is the split whose seam is drawn lit this frame,
// UINT32_MAX or any index that is not a split for none; the caller's to fill,
// like `pointer` (resize.h's `reached`).
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
	voe_ui_keyboard keyboard;
	uint32_t lit;
} voe_editor_dock_root;

// The tree the editor opens on: three columns — `Scene` above `Assets`, held
// at ASSETS_TALL, together held at SIDE_WIDE on the left, `Inspector` held
// at SIDE_WIDE on the right, and the two scene views
// stacked taking the rest between them, the views' share starting at a half.
// Which view is on top is one number on one leaf, which is what having a tree
// at all is for.
voe_editor_dock_tree voe_editor_dock_default(void);

// Whether a leaf in `tree` shows view `view`. The loop asks before it draws a
// view, because a view nobody shows is not drawn — see view.h.
bool voe_editor_dock_shows_view(const voe_editor_dock_tree *tree, uint32_t view);

// Where one node is, and for a split where its seam is. `rect` is the node's
// own rectangle; `seam` is the gap between a split's two children. For every
// split, `least` and `most` are the bounds a drag may write and `shown` the
// length laid out, `most` winning and never below nought: the held child's
// length, or for a FRACTION split the first child's — `fraction` of the divided
// length, between its need and the divided length less the second's need. All
// three are nought for a leaf.
typedef struct {
	voe_ui_rect rect;
	voe_ui_rect seam;
	float least;
	float most;
	float shown;
} voe_editor_dock_place;

// Every node's place, indexed as the tree's nodes are. A node the root does
// not reach is left zeroed.
typedef struct {
	voe_editor_dock_place nodes[VOE_EDITOR_DOCK_NODES];
} voe_editor_dock_arrangement;

// Lays `tree` out over `area` in millimetres, the same division the walk
// draws — the walk takes every child's size from this, so the two cannot
// disagree. A held length is clamped as 0226 says: no less than what the held
// child needs, never so much the other side has less than it needs, and where
// both cannot hold the other side wins.
void voe_editor_dock_arrange(const voe_editor_dock_tree *tree, voe_ui_rect area,
			     voe_editor_dock_arrangement *out);

// The length of the split whose held child is a leaf of `panel`, or a split
// whose first child is, and that length set. Nought, and nothing written, when no split holds one.
float voe_editor_dock_panel_length(const voe_editor_dock_tree *tree,
				   voe_editor_panel panel);
void voe_editor_dock_panel_length_set(voe_editor_dock_tree *tree,
				      voe_editor_panel panel, float length);

// The one write of split `node`'s edge, `length` being what `shown` is in
// `arrangement` (the tree's own, laid out this frame). It is clamped to the
// place's least..most; a held split stores it as `length`, a FRACTION split as
// its share of the divided length, kept strictly between nought and one.
void voe_editor_dock_split_set(voe_editor_dock_tree *tree, uint32_t node,
			       const voe_editor_dock_arrangement *arrangement,
			       float length);

// The views' share: the `fraction` of the split whose two children are both
// scene views, and that share set (nought < `share` < one). A half, and
// nothing written, when there is no such split.
double voe_editor_dock_view_share(const voe_editor_dock_tree *tree);
void voe_editor_dock_view_share_set(voe_editor_dock_tree *tree, double share);

// Emits `root`'s tree into `ui` as one row or column whose two children are
// given fixed sizes in millimetres, on down to a leaf's panel with
// `voe_editor_panel_draw`'s contents in it.
//
// `parent` IS THE AXIS OF WHATEVER CONTAINER THIS CALL IS MADE INSIDE — the
// same thing walk_node's own `parent` argument already is, for the tree's
// deeper nodes, and for the same reason: a `voe_ui_container.size.along` and
// `.across` on a child are read against its PARENT's flow, never its own
// (ui/layout.h), and this call's own row is a child exactly like every split
// below it once anything wraps it. `sizing_in()`, this file's one place that
// turns a plain voe_math_float2 into the pair read correctly either way, is
// what this call uses too. interface.c opens a column over the top bar and
// this tree, so it passes VOE_EDITOR_DOCK_COLUMN; a caller that opens no
// container of its own and hands this the frame's actual root passes
// VOE_EDITOR_DOCK_ROW, which reads exactly as it did before this parameter
// existed.
//
// It is called between voe_ui_frame_begin and voe_ui_frame_end and once per
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
//
// `palette` is the one in force, read for every seam's colours (0231, 0232)
// and handed to each panel, where only the Scene list reads it (0282).
void voe_editor_dock_walk(const voe_editor_dock_root *root,
			  voe_editor_dock_axis parent, voe_ui_context *ui,
			  const voe_ui_theme *palette, voe_editor_scene *scene,
			  voe_editor_views *views);

// What is on one panel. One function, one `switch`, no table and no function
// pointer; a panel's contents are ordinary `ui` calls and the tree above owns
// only the geometry (ADR-0142 point 2). Called from inside the leaf's panel, so
// everything it emits is a child of that panel.
//
// AND INSIDE THE SCROLL AREA THAT PANEL HOLDS, unless the panel is a scene view
// — see dock.c's header. So what is emitted here is clipped to the leaf and
// scrolls when there is more of it than fits, and a panel need not ask for
// either.
//
// `view` is the leaf's own and is read only for a SCENE_VIEW panel.
// `palette` is the walk's, read only by the Scene list for its drag's marks.
void voe_editor_panel_draw(voe_ui_context *ui, voe_editor_panel panel,
			   uint32_t view, const voe_ui_theme *palette,
			   voe_editor_scene *scene, voe_editor_views *views);
