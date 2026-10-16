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
//     [Levels]
//     count=6
//     share1=50
//     ...
//     share5=3
//     swap1=1
//     ...
//     swap5=1
//
// `material<N>` is glTF material N of the file, N written with no leading zero
// and below VOE_ASSETS_PARTS; the value is a project-relative `.material` path,
// "" for none, when the part draws with the glTF material itself.
//
// `[Levels]` (ADR 0430, 0436) is optional; each key absent reads as its
// default. `count` is the levels in all, level 0 included, 1 to 6 (0433).
// An old `strength` key is read and ignored whatever its value, and never
// written: the edges and collapse limit it set are now fixed in `3d` (0437).
// `share<N>` is level N's triangles in percent of level 0's, in (0, 100), each
// below the one before; `swap<N>` multiplies the distance level N's error
// allows, 0.1 to 10, not level N-1's: a level landing at or nearer than the
// one before swaps in with it and hides it (`3d/model_levels.h`, THE LEVELS).
// N is 1 to 5, counted from zero as level indices are
// (0434). The old word
// `swap="near|normal|far|farther"` still reads, as 0.5, 1, 2 or 4 for every
// `swap<N>` absent, but is never written.
//
// KEYS ARE INDICES, NOT NAMES. Sectioned text refuses a repeated key, and glTF
// names repeat and hold any byte; an index is unique and always a legal key. A
// re-export that reorders the materials is set right by importing again.
//
// NOTHING HERE OPENS A FILE; `platform` owns files, and the caller hands the
// bytes in and writes the text out. The game never reads this (0236): the cook
// writes it into a table. The one exception is `[Levels]`: the loader reads it
// when it makes a levels file (0430 point 5).
#pragma once

#include <assets/material.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// The most glTF materials one parts file names.
#define VOE_ASSETS_PARTS 32

// The most levels a model has in all, level 0 included; equal to `3d`'s
// VOE_3D_MODEL_LEVELS.
#define VOE_ASSETS_LEVELS 6

// The least and the most a level's swap multiplier may be.
#define VOE_ASSETS_LEVELS_SWAP_LEAST 0.1f
#define VOE_ASSETS_LEVELS_SWAP_MOST 10.0f

typedef struct {
	// Levels in all, level 0 included: 1 to VOE_ASSETS_LEVELS.
	uint32_t count;
	// Each level's triangles in percent of level 0's, by level index;
	// index 0 is always 100.
	float shares[VOE_ASSETS_LEVELS];
	// Each level's multiplier on the distance its error allows, by level
	// index; index 0 is always 1.
	float swaps[VOE_ASSETS_LEVELS];
} voe_assets_levels;

// What a `[Levels]` section, or a key of it, absent reads as.
#define VOE_ASSETS_LEVELS_DEFAULT                                   \
	((voe_assets_levels){ .count = 6,                           \
			      .shares = { 100, 50, 25, 12, 6, 3 },      \
			      .swaps = { 1, 1, 1, 1, 1, 1 } })

typedef struct {
	// One past the highest key read; entries at and above it are unused.
	uint32_t count;
	// Entry N for glTF material N: project-relative, NUL-terminated, ""
	// for none.
	char materials[VOE_ASSETS_PARTS][VOE_ASSETS_MATERIAL_PATH];
	// The model's levels, VOE_ASSETS_LEVELS_DEFAULT when the text has none.
	voe_assets_levels levels;
} voe_assets_parts;

// Reads `size` bytes of text, which need not be NUL-terminated; a key not in
// the text reads as "". MALFORMED for text the sectioned reader refuses, no
// `[Parts]` section, a key that is not `material<N>` with N below
// VOE_ASSETS_PARTS, a path of VOE_ASSETS_MATERIAL_PATH bytes or more, a
// `count` outside 1 to VOE_ASSETS_LEVELS, a `swap` not one of the four words,
// a `share<N>` or `swap<N>` that is not one number or is out of its range,
// shares not each below the one before, any other key in `[Levels]` but
// `strength`, or any section but those two. A line on stderr names what.
// `out` untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_parts_read(const char *text, size_t size,
					 voe_assets_parts *out,
					 voe_base_error *error);

// The text voe_assets_parts_read takes back exactly, one key per entry below
// `count`, paths quoted, then always `[Levels]` after `[Parts]`;
// NUL-terminated, `size` not counting the NUL, on `arena`. Cannot fail; asserts
// `count` is at most VOE_ASSETS_PARTS, each path is NUL-terminated in its room
// and holds no line break, the levels' count is 1 to VOE_ASSETS_LEVELS, and
// shares and swaps 1 to 5 in range, the shares in order. Numbers are written `%g`: six digits, enough for the tenths the panel
// sets.
voe_assets_material_text voe_assets_parts_write(const voe_assets_parts *parts,
						voe_base_arena *arena);
