// The sizes a person gave the editor's panels, which dock panels are open and
// how the gizmo snaps, remembered in `<settings>/voe3d/editor_settings`: the
// Scene list's and the Inspector's widths, the Assets panel's height
// (ADR-0279) and the top bar's height, in the surface's millimetres, the
// views' share, the four open flags (0363 point 5), and the snap toggle and
// its three steps (0482 point 5).
//
// ONE FILE FOR THE PERSON, NOT THE PROJECT (ADR-0220). The layout is how a
// person likes their editor, so every project opens with the panels as they
// were last left, and nothing of it is written into a project. The snap steps
// are the person's too, not the project's (ADR-0438).
//
// ONE `<key> <value>` LINE EACH (ADR-0226): `scene_wide`, `inspector_wide`,
// `assets_tall`, `topbar_high` and `view_share`, each `%.3f`, then
// `scene_open`, `assets_open`, `inspector_open` and `view_open`, each `0` or
// `1`. The first four are millimetres; `view_share` is a share, the top view's
// part of the middle, not millimetres (ADR-0229). Then `snap`, `0` or `1`, and
// `snap_move` in metres, `snap_rotate` in degrees and `snap_scale`, each
// `%.3f`. The file is named for the editor's settings and later settings join
// it, so each write keeps every line with another key, in its order, and only
// replaces its own: the panel write keeps the four snap lines, the snap write
// the nine panel lines, and both the dock layout's (settings_layout.h) and the
// `fold` lines, which are the folds' (folds.h).
//
// A FIRST START IS NOT A FAILURE. A missing settings folder or file, a line
// that does not parse, a number out of range or a flag that is not exactly
// `0` or `1` leaves that field as the caller set it, or at its default, and
// reports nothing: the defaults, every panel open and snap off, are a correct
// answer. A write that fails is reported at the site. Each call keeps its own
// scratch arena.
#pragma once

#include <stddef.h>

// Millimetres, but `view_share`. `topbar_high` nought means the bar fits its
// content; `view_share` is nought to one exclusive. The `_open` flags are the
// Scene list, Assets, the Inspector and the bottom view.
typedef struct {
	float scene_wide;
	float inspector_wide;
	float topbar_high;
	double view_share;
	float assets_tall;
	bool scene_open;
	bool assets_open;
	bool inspector_open;
	bool view_open;
} voe_editor_settings;

// Whether a gizmo drag snaps, and its steps: `move` in metres, `rotate` in
// degrees, `scale` a factor.
typedef struct {
	bool on;
	float move;
	float rotate;
	float scale;
} voe_editor_snap;

// The steps a first start has.
#define VOE_EDITOR_SNAP_MOVE 0.5f
#define VOE_EDITOR_SNAP_ROTATE 15.0f
#define VOE_EDITOR_SNAP_SCALE 0.1f

// Each step's least and most, both allowed. Doubles, so a step written `%.3f`
// at its bound reads back in range.
#define VOE_EDITOR_SNAP_MOVE_LEAST 0.001
#define VOE_EDITOR_SNAP_MOVE_MOST 1000.0
#define VOE_EDITOR_SNAP_ROTATE_LEAST 0.1
#define VOE_EDITOR_SNAP_ROTATE_MOST 360.0
#define VOE_EDITOR_SNAP_SCALE_LEAST 0.001
#define VOE_EDITOR_SNAP_SCALE_MOST 100.0

// Overwrites each field the file has a parsing, in-range line for:
// `scene_wide`, `inspector_wide` and `assets_tall` above nought, `topbar_high`
// nought or more, all at most 1000 mm, `view_share` finite and strictly
// between nought and one, and each `_open` flag exactly `0` or `1`. Every
// other field keeps what the caller put in.
void voe_editor_settings_read(voe_editor_settings *settings);

// Re-reads the file, keeps every line with another key in its order, and
// writes the nine after them, making `<settings>` and
// `<settings>/voe3d` first if either is missing. False on any failure,
// reported through base/report.h at the site.
[[nodiscard]] bool voe_editor_settings_write(const voe_editor_settings *settings);

// Snap off with the three default steps, and over them each field the file
// has a parsing, in-range line for: `snap` exactly `0` or `1`, each step
// between its least and most.
voe_editor_snap voe_editor_settings_snap_read(void);

// Re-reads the file, keeps every line with another key in its order, and
// writes `snap`, `snap_move`, `snap_rotate` and `snap_scale` after them, as
// voe_editor_settings_write does. False on any failure, reported at the site.
[[nodiscard]] bool voe_editor_settings_snap_write(const voe_editor_snap *snap);

// --- settings.c's, settings_layout.c's and folds.c's own, not for others ---
//
// Hands `take` each line of the file, its newline cut: `into` and the line.
// Nothing when there is no file.
void voe_editor_settings_lines_read(void (*take)(void *into, const char *line),
				    void *into);

// Re-reads the file, keeps every non-empty line `own_line` is false for, in
// its order, and writes `own`'s `used` bytes after them, making `<settings>`
// and `<settings>/voe3d` first. False on any failure, which the caller reports.
[[nodiscard]] bool voe_editor_settings_lines_write(
	bool (*own_line)(const char *line), const char *own, size_t used);
