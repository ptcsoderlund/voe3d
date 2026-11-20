// A tree species file (`<name>.tree`, 0515 point 1): the materials and
// settings a tree is grown from, in sectioned text, read into a struct and
// written back.
//
//     [Tree]
//     bark="Assets/pine_bark.material"
//     leaf="Assets/pine_needles.material"
//     height=12
//     trunk=0.4
//     crown=0.3
//     branching=4
//     reach=0.35
//     angle=60
//     droop=0.2
//     crooked=0.2
//     leaf_size=0.3
//     leaf_density=12
//     variations=4
//
// THIS FILE IS ALL A SPECIES KEEPS ON DISK (0510): every variation is grown
// again from it when a project opens, and no mesh is saved beside it.
//
// `bark` and `leaf` are quoted project-relative `.material` paths, "" for none.
// `height` is metres; `trunk` the base diameter in metres; `crown` the share
// of the height where branches start; `branching` branches per metre of their
// parent; `reach` a child's length as a share of its parent's; `angle` degrees
// from the parent; `droop` how far branches bend down (negative up) and
// `crooked` how far they wander, both without unit; `leaf_size` metres a leaf
// card; `leaf_density` cards per twig; `variations` how many trees are grown.
// The ranges and defaults are the constants below, which the Inspector and the
// key table share.
//
// A MISSING KEY READS ITS DEFAULT, A VALUE OUT OF RANGE IS CLAMPED AND AN
// UNKNOWN KEY IS PASSED OVER, as a material file's are, so a project still
// opens with keys another editor version wrote. A FILE TAKEN IN FROM DISK IS
// READ STRICTLY (0509 point 3): voe_assets_tree_read_strict refuses an unknown
// section or key and a value out of range, naming it.
//
// VARIATION v OF A SPECIES IS THE PATH `<path>#<v>`, counting from 0 (0515
// point 3): `Assets/pine.tree#2`. It names a grown entry, never a file: `#` is
// never part of a file name the editor makes, so it never meets a real one.
//
// NOTHING HERE OPENS A FILE; `platform` owns files, and the caller hands the
// bytes in and writes the text out.
#pragma once

#include <assets/material.h>
#include <assets/sectioned.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// The room of a bark or leaf path, its NUL included.
#define VOE_ASSETS_TREE_PATH VOE_ASSETS_MATERIAL_PATH

// Each key's least, greatest and default (0515 point 1).
#define VOE_ASSETS_TREE_HEIGHT_MIN 1.0f
#define VOE_ASSETS_TREE_HEIGHT_MAX 60.0f
#define VOE_ASSETS_TREE_HEIGHT_DEFAULT 12.0f
#define VOE_ASSETS_TREE_TRUNK_MIN 0.05f
#define VOE_ASSETS_TREE_TRUNK_MAX 3.0f
#define VOE_ASSETS_TREE_TRUNK_DEFAULT 0.4f
#define VOE_ASSETS_TREE_CROWN_MIN 0.0f
#define VOE_ASSETS_TREE_CROWN_MAX 0.9f
#define VOE_ASSETS_TREE_CROWN_DEFAULT 0.3f
#define VOE_ASSETS_TREE_BRANCHING_MIN 0.5f
#define VOE_ASSETS_TREE_BRANCHING_MAX 20.0f
#define VOE_ASSETS_TREE_BRANCHING_DEFAULT 4.0f
#define VOE_ASSETS_TREE_REACH_MIN 0.05f
#define VOE_ASSETS_TREE_REACH_MAX 1.0f
#define VOE_ASSETS_TREE_REACH_DEFAULT 0.35f
#define VOE_ASSETS_TREE_ANGLE_MIN 5.0f
#define VOE_ASSETS_TREE_ANGLE_MAX 120.0f
#define VOE_ASSETS_TREE_ANGLE_DEFAULT 60.0f
#define VOE_ASSETS_TREE_DROOP_MIN -1.0f
#define VOE_ASSETS_TREE_DROOP_MAX 1.0f
#define VOE_ASSETS_TREE_DROOP_DEFAULT 0.2f
#define VOE_ASSETS_TREE_CROOKED_MIN 0.0f
#define VOE_ASSETS_TREE_CROOKED_MAX 1.0f
#define VOE_ASSETS_TREE_CROOKED_DEFAULT 0.2f
#define VOE_ASSETS_TREE_LEAF_SIZE_MIN 0.02f
#define VOE_ASSETS_TREE_LEAF_SIZE_MAX 2.0f
#define VOE_ASSETS_TREE_LEAF_SIZE_DEFAULT 0.3f
#define VOE_ASSETS_TREE_LEAF_DENSITY_MIN 0.0f
#define VOE_ASSETS_TREE_LEAF_DENSITY_MAX 64.0f
#define VOE_ASSETS_TREE_LEAF_DENSITY_DEFAULT 12.0f
#define VOE_ASSETS_TREE_VARIATIONS_MIN 1u
#define VOE_ASSETS_TREE_VARIATIONS_MAX 16u
#define VOE_ASSETS_TREE_VARIATIONS_DEFAULT 4u

// One species' values, each in its range once read.
typedef struct {
	// Project-relative `.material` paths, NUL-terminated, "" for none.
	char bark[VOE_ASSETS_TREE_PATH];
	char leaf[VOE_ASSETS_TREE_PATH];
	// Metres.
	float height;
	// Base diameter, metres.
	float trunk;
	// Share of the height where branches start.
	float crown;
	// Branches per metre of parent.
	float branching;
	// A child's length as a share of its parent's.
	float reach;
	// Degrees from the parent.
	float angle;
	float droop;
	float crooked;
	// Metres.
	float leaf_size;
	// Cards per twig.
	float leaf_density;
	uint32_t variations;
} voe_assets_tree;

typedef struct {
	const char *text;
	size_t size;
} voe_assets_tree_text;

// What a missing key reads as, and what Create writes: no materials and each
// key's DEFAULT.
voe_assets_tree voe_assets_tree_default(void);

// Reads `size` bytes of text, which need not be NUL-terminated. A missing key
// reads its default, a value out of range is clamped, an unknown key or section
// is passed over. MALFORMED for text the sectioned reader refuses, no `[Tree]`
// section, a number that does not parse, a `variations` not a whole number, or
// a path of VOE_ASSETS_TREE_PATH bytes or more. A line on stderr names what.
// `out` untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_tree_read(const char *text, size_t size,
					voe_assets_tree *out,
					voe_base_error *error);

// As voe_assets_tree_read, and MALFORMED too for a section other than [Tree],
// a key the reader does not take, and a value out of range, which is refused
// and not clamped. The line on stderr names the line or key and why.
[[nodiscard]] bool voe_assets_tree_read_strict(const char *text, size_t size,
					       voe_assets_tree *out,
					       voe_base_error *error);

// The keys the readers take, one row each in the struct's order, ranges from
// the constants above; `count` gets the rows. Static, never freed.
const voe_assets_documented_key *voe_assets_tree_keys(uint32_t *count);

// The text voe_assets_tree_read takes back exactly, NUL-terminated, `size` not
// counting the NUL, on `arena`. Floats are printed to nine digits, which a
// float round-trips through, and paths always quoted. Cannot fail; asserts each
// path is NUL-terminated in its room and holds no line break.
voe_assets_tree_text voe_assets_tree_write(const voe_assets_tree *tree,
					   voe_base_arena *arena);

// Whether `path` names a species or one of its variations: it ends `.tree` in
// any case, or `.tree#` and one or more digits.
bool voe_assets_tree_path_is(const char *path);

// `<path>#<v>` into `out` of `room` bytes, NUL-terminated. False, and `out`
// untouched, when it will not fit.
[[nodiscard]] bool voe_assets_tree_variation_path(char *out, size_t room,
						  const char *path, uint32_t v);
