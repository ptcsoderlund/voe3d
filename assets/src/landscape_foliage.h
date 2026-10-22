// The landscape file's foliage, internal to this folder: `landscape.c` reads
// the shape, layers and heights and hands `[Foliage]` and `[Copies]` to
// `landscape_foliage.c`, which prints with `landscape.c`'s printer and checks
// and writes a kind's quotes with its two path helpers.
//
// [Foliage] IS PRINTED BEFORE [Heights] and [Copies] after [Paint]: a kind's
// quote check counts line breaks from the start of the text, so the kinds are
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

// `[Foliage]`'s kinds and `[Copies]`' copies of `text`, `size` bytes, into
// `into`: kinds from kind0= to the first missing key, each with its optional
// `size<k>` or 0, the copies pushed on
// `arena`, none and NULL with no `[Copies]`. False and a line on stderr for
// what voe_assets_landscape_read refuses of them; `into` may be partly written.
[[nodiscard]] bool
voe_assets_landscape_foliage_read(const voe_assets_sectioned *doc,
				  const char *text, size_t size,
				  voe_base_arena *arena,
				  voe_assets_landscape *into,
				  voe_base_error *error);

// `[Foliage]` and a blank line after it when there are kinds, a kind's
// `size<k>` after its path unless 0 or 1000; nothing otherwise.
void voe_assets_landscape_kinds_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape);

// `[Copies]` with a blank line before it, one line a copy, when there are
// copies; nothing otherwise.
void voe_assets_landscape_copies_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape);
