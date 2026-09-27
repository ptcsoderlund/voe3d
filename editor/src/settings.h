// The sizes a person gave the editor's panels, remembered in
// `<settings>/voe3d/editor_settings`: the Scene list's and the Inspector's
// widths, the Assets panel's height (ADR-0279) and the top bar's height, in
// the surface's millimetres, and the views' share.
//
// ONE FILE FOR THE PERSON, NOT THE PROJECT (ADR-0220). The layout is how a
// person likes their editor, so every project opens with the panels as they
// were last left, and nothing of it is written into a project.
//
// ONE `<key> <number>` LINE PER SIZE (ADR-0226): `scene_wide`,
// `inspector_wide`, `assets_tall`, `topbar_high` and `view_share`, each
// `%.3f`. The first four are millimetres; `view_share` is a share, the top
// view's part of the middle, not millimetres (ADR-0229). The file is named for
// the editor's settings and later settings join it, so a write keeps every
// line with another key, in its order, and only replaces these five.
//
// A FIRST START IS NOT A FAILURE. A missing settings folder or file, a line
// that does not parse or a number out of range leaves that size as the caller
// set it and reports nothing: the defaults are a correct answer. A write that
// fails is reported at the site. Each call keeps its own scratch arena.
#pragma once

// Millimetres, but `view_share`. `topbar_high` nought means the bar fits its
// content; `view_share` is nought to one exclusive.
typedef struct {
	float scene_wide;
	float inspector_wide;
	float topbar_high;
	double view_share;
	float assets_tall;
} voe_editor_settings;

// Overwrites each field the file has a parsing, in-range line for:
// `scene_wide`, `inspector_wide` and `assets_tall` above nought, `topbar_high`
// nought or more, all at most 1000 mm, and `view_share` finite and strictly
// between nought and one. Every other field keeps what the caller put in.
void voe_editor_settings_read(voe_editor_settings *settings);

// Re-reads the file, keeps every line with another key in its order, and
// writes the five after them, making `<settings>` and
// `<settings>/voe3d` first if either is missing. False on any failure,
// reported through base/report.h at the site.
[[nodiscard]] bool voe_editor_settings_write(const voe_editor_settings *settings);
