// The landscape file's foliage, internal to this folder: `landscape.c` reads
// the shape, layers and heights and hands `[Foliage]`, `[Layers]`' grow keys
// and `[Copies]` to `landscape_foliage.c`, which prints with `landscape.c`'s
// printer and checks and writes a glb's quotes with its two path helpers.
//
// [Foliage] IS PRINTED BEFORE [Heights] and [Copies] after [Paint]: a glb's
// quote check counts line breaks from the start of the text, so the groups are
// kept near it, and a copy is the last thing a file holds.
#pragma once

#include "landscape_paint.h"

#include <stddef.h>

// One escaped path: every byte doubled at worst, two quotes and a NUL.
#define VOE_ASSETS_LANDSCAPE_QUOTED_PATH (2 * VOE_ASSETS_MATERIAL_PATH + 3)

// Whether key `name` of `section` was written with its value in quotes. In
// `landscape.c`.
bool voe_assets_landscape_written_quoted(const voe_assets_sectioned *doc,
					 uint32_t section, const char *name,
					 const char *text, size_t size);

// `path`, NUL-terminated within VOE_ASSETS_MATERIAL_PATH bytes and with no line
// break, between quotes with `"` and `\` escaped, into `quoted` of
// VOE_ASSETS_LANDSCAPE_QUOTED_PATH bytes. In `landscape.c`.
void voe_assets_landscape_quote(const char *path, char *quoted);

// `[Foliage]`'s groups, `[Layers]`' grows and `[Copies]`' copies of `text`,
// `size` bytes, into `into`, whose layers are read already: groups in either
// spelling, each look key missing the default's; then each `grow<l>_<g>`, now
// that both counts are known; the copies pushed on `arena`, none and NULL with
// no `[Copies]`. False and a line on stderr for what voe_assets_landscape_read
// refuses of them, and when `strict` for a `[Foliage]` key not a path, look key
// or seed of a glb or group in the file, in its spelling; `into` may be partly
// written.
[[nodiscard]] bool
voe_assets_landscape_foliage_read(const voe_assets_sectioned *doc,
				  const char *text, size_t size, bool strict,
				  voe_base_arena *arena,
				  voe_assets_landscape *into,
				  voe_base_error *error);

// `[Layers]`' `grow<l>_<g>` lines of every layer and group below their counts,
// each only above 0. `landscape.c` calls it after the layer paths.
void voe_assets_landscape_grows_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape);

// `[Foliage]` and a blank line after it when there are groups: each glb's
// `glb<g>_<j>` and its look keys after it (`size`, `vary`, `turn`, `tilt`,
// `shadow`, `blend`, `blendheight`, `share`), then the group's `seed<g>`, each
// only off the default; nothing otherwise.
void voe_assets_landscape_groups_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape);

// `[Copies]` with a blank line before it, one line a copy, when there are
// copies; nothing otherwise.
void voe_assets_landscape_copies_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape);
