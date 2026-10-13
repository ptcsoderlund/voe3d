// A parts file (`<stem>.parts`, beside `<stem>.glb`, ADR 0420 point 1): which
// `.material` each glTF material of a `.glb` wears (0404), shared by every
// placed copy of the model, in sectioned text read into a struct and written
// back.
//
//     [Parts]
//     material0="Assets/tree-bark.material"
//     material1=""
//     material2="Assets/tree-leaves.material"
//
// `material<N>` is glTF material N of the file, N written with no leading zero
// and below VOE_ASSETS_PARTS; the value is a project-relative `.material` path,
// "" for none, when the part draws with the glTF material itself.
//
// KEYS ARE INDICES, NOT NAMES. Sectioned text refuses a repeated key, and glTF
// names repeat and hold any byte; an index is unique and always a legal key. A
// re-export that reorders the materials is set right by importing again.
//
// NOTHING HERE OPENS A FILE; `platform` owns files, and the caller hands the
// bytes in and writes the text out. The game never reads this (0236): the cook
// writes it into a table.
#pragma once

#include <assets/material.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// The most glTF materials one parts file names.
#define VOE_ASSETS_PARTS 32

typedef struct {
	// One past the highest key read; entries at and above it are unused.
	uint32_t count;
	// Entry N for glTF material N: project-relative, NUL-terminated, ""
	// for none.
	char materials[VOE_ASSETS_PARTS][VOE_ASSETS_MATERIAL_PATH];
} voe_assets_parts;

// Reads `size` bytes of text, which need not be NUL-terminated; a key not in
// the text reads as "". MALFORMED for text the sectioned reader refuses, no
// `[Parts]` section, a key that is not `material<N>` with N below
// VOE_ASSETS_PARTS, or a path of VOE_ASSETS_MATERIAL_PATH bytes or more. A
// line on stderr names what. `out` untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_parts_read(const char *text, size_t size,
					 voe_assets_parts *out,
					 voe_base_error *error);

// The text voe_assets_parts_read takes back exactly, one key per entry below
// `count`, paths quoted, NUL-terminated, `size` not counting the NUL, on
// `arena`. Cannot fail; asserts `count` is at most VOE_ASSETS_PARTS and each
// path is NUL-terminated in its room and holds no line break.
voe_assets_material_text voe_assets_parts_write(const voe_assets_parts *parts,
						voe_base_arena *arena);
