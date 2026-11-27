// A landscape file (`<name>.landscape`, ADR 0379 point 1, 0402 points 1-2, 0442
// point 1, 0476 points 1-2, 0521 point 1): a square grid of heights, up to
// eight painted material layers and the foliage on it, in sectioned text.
//
//     [Landscape]
//     size=256
//     cells=4
//
//     [Layers]
//     layer0="Assets/grass.material"
//     grow0_0=40
//
//     [Foliage]
//     glb0_0="Assets/pine.glb"
//     size0_0=2000
//     glb0_1="Assets/spruce.glb"
//     share0_1=3
//
//     [Heights]
//     row0=0 0 0 0 0
//
//     [Paint]
//     paint1_0=0 120 255 120 0
//
//     [Copies]
//     c0=0 -1500 32000 16384 1000 0
//
// `size` is metres a side and `cells` the cells per side: cells + 1 rows of
// cells + 1 heights, row r at z = -size/2 + r*size/cells and value c at x =
// -size/2 + c*size/cells about the origin, a height metres up from it. Every
// grid is row-major. ONE FILE, NO PICTURE BESIDE IT (0379 point 1): diffable;
// 512 cells is 2-3 MB. AT MOST 2048 CELLS (0396 point 1): 2049² texels fit
// Vulkan's 4096 a side. HEIGHTS ARE WHOLE MILLIMETRES, read back as written.
//
// LAYERS ARE QUOTED .material PATHS from layer0= to the first missing key. PAINT
// IS A WEIGHT 0-255 PER LAYER AT EVERY POINT, summing to 255: `paint<k>_<r>` is
// layer k's row r from k = 1, LAYER 0 NEVER WRITTEN but 255 minus the rest, and
// `v*n` is n copies of v. No [Paint] and NULL paint are all layer 0. A
// RESAMPLE IS BILINEAR, layers 1-7 rounded and layer 0 given the rest.
//
// A GROUP HOLDS QUOTED .glb PATHS (0520): `glb<g>_<j>`, g to the first group
// with no `glb<g>_0` and j to the first missing, at most 16 groups of 8, a path
// once in its group. A GLB'S LOOK is `size` (thousandths), `vary` (size ±%),
// `turn` (±°), `tilt` (% with slope), `shadow=0`, `blend=1`, `blendheight`
// (cm) and `share` (0-10, how often it is picked against its group's others),
// each `<g>_<j>`, written only off the default, missing read as it. `seed<g>`
// (0-15, g when missing, 0517) is the group's. LAYER l GROWS GROUP g at
// `grow<l>_<g>` tenths of copies per 100 m², THE LAYER ROW'S CHOICE (0476).
// COPY i IS `c<i>=group x z turn scale lean`, standing on the ground.
//
// THE OLD SPELLING READS AS ONE-GLB GROUPS: `kind<k>` with `<key><k>` look keys
// and `seed<k>` is group k of one glb, so every saved project opens (0520). Both
// spellings in one file are malformed; the writer writes groups only.
//
// A FILE TAKEN IN FROM DISK IS READ STRICTLY (0509 point 3), refusing an unknown
// section or key too. THE GAME NEVER READS THIS (0236): the editor does; the
// cook makes a table. `platform` owns files. Results go on the arena handed in.
#pragma once

#include <assets/material.h>
#include <assets/sectioned.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// What Create writes.
#define VOE_ASSETS_LANDSCAPE_CELLS 512
// The most a file may hold.
#define VOE_ASSETS_LANDSCAPE_CELLS_MAX 2048
#define VOE_ASSETS_LANDSCAPE_SIZE_MIN 16.0f
#define VOE_ASSETS_LANDSCAPE_SIZE_MAX 8192.0f
#define VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT 256.0f
// The most material layers a landscape names, two RGBA8 planes' worth.
#define VOE_ASSETS_LANDSCAPE_LAYERS 8
// The most foliage groups, glbs a group and copies a landscape holds, and a glb
// path's room.
#define VOE_ASSETS_LANDSCAPE_GROUPS 16
#define VOE_ASSETS_LANDSCAPE_GROUP_GLBS 8
#define VOE_ASSETS_LANDSCAPE_COPIES 65536
#define VOE_ASSETS_LANDSCAPE_GLB_PATH 128
// A glb's base size in thousandths: the file's range and the editor's slider's.
#define VOE_ASSETS_LANDSCAPE_GLB_SIZE_MIN 100
#define VOE_ASSETS_LANDSCAPE_GLB_SIZE_MAX 10000
// A glb's three spreads, most and default: size ±%, turn ±°, tilt % with slope.
#define VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX 100
#define VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX 180
#define VOE_ASSETS_LANDSCAPE_TILT_MAX 100
#define VOE_ASSETS_LANDSCAPE_SIZE_VARY_DEFAULT 20
#define VOE_ASSETS_LANDSCAPE_TURN_VARY_DEFAULT 180
#define VOE_ASSETS_LANDSCAPE_TILT_DEFAULT 0
// How high a blending glb's copies blend with the ground, in centimetres.
#define VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN 5
#define VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX 1000
#define VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_DEFAULT 50
// How often a glb is picked against its group's others (0521 point 2).
#define VOE_ASSETS_LANDSCAPE_SHARE_MAX 10
#define VOE_ASSETS_LANDSCAPE_SHARE_DEFAULT 1
// The most tenths of copies per 100 m² a layer grows of a group: 50 copies.
#define VOE_ASSETS_LANDSCAPE_GROW_MAX 500

// How a glb's copies look (0476 point 1, 0520). Compare field by field: padding.
typedef struct {
	// Base size in thousandths, GLB_SIZE_MIN to GLB_SIZE_MAX, 0 meaning 1×;
	// read as a factor only through voe_assets_landscape_glb_size.
	uint16_t size;
	// Size ±%, 0 to SIZE_VARY_MAX.
	uint8_t size_vary;
	// Turn ±degrees, 0 to TURN_VARY_MAX.
	uint8_t turn_vary;
	// How far a copy leans with the slope, %, 0 to TILT_MAX.
	uint8_t tilt;
	// Casts no shadow.
	bool shadowless;
	// Its copies blend with the ground they stand on (0481 point 4).
	bool blends;
	// How high they blend in centimetres, BLEND_HEIGHT_MIN to
	// BLEND_HEIGHT_MAX, 0 meaning BLEND_HEIGHT_DEFAULT.
	uint16_t blend_height;
	// How often it is picked against its group's other glbs, 0 to
	// SHARE_MAX; a group whose shares are all 0 picks as if all were 1.
	uint8_t share;
} voe_assets_landscape_look;

// A foliage group (0520): what is painted and grown, holding its glbs and its
// seed and nothing else.
typedef struct {
	// Project-relative `.glb` paths, NUL-terminated, each once; the first
	// `glb_count` are the group's.
	char glbs[VOE_ASSETS_LANDSCAPE_GROUP_GLBS][VOE_ASSETS_LANDSCAPE_GLB_PATH];
	// Each glb's look.
	voe_assets_landscape_look looks[VOE_ASSETS_LANDSCAPE_GROUP_GLBS];
	uint32_t glb_count;
	// The seed of the group's draws plus one, 0 meaning the group's own index
	// (0517 point 4); read only through voe_assets_landscape_group_seed.
	uint8_t seed;
} voe_assets_landscape_group;

// One foliage copy, in the order a cooked `{x, z, turn, scale, group, lean}`
// fills. Compare field by field, never with memcmp: the struct has padding.
typedef struct {
	// Millimetres in the grid's own space, as the heights' x and z.
	int32_t x, z;
	// 1/65536 of a turn about +Y.
	uint16_t turn;
	// Thousandths, 1 to 65535.
	uint16_t scale;
	// Below the landscape's group_count.
	uint8_t group;
	// 0 stands straight up, 255 along the ground's normal.
	uint8_t lean;
} voe_assets_landscape_copy;

typedef struct {
	// Metres a side.
	float size;
	// Cells per side, a multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
	uint32_t cells;
	// (cells + 1)² metres, row-major.
	float *heights;
	// Project-relative `.material` paths, NUL-terminated; the first
	// `layer_count` are the landscape's.
	char layers[VOE_ASSETS_LANDSCAPE_LAYERS][VOE_ASSETS_MATERIAL_PATH];
	uint32_t layer_count;
	// Two RGBA8 planes of (cells + 1)² texels each, row-major as the heights:
	// plane 0 holds layers 0-3 in r g b a, plane 1 layers 4-7. A point's eight
	// bytes sum to 255. NULL means every point all layer 0.
	uint8_t *paint;
	// The first `group_count` are the landscape's foliage groups.
	voe_assets_landscape_group groups[VOE_ASSETS_LANDSCAPE_GROUPS];
	uint32_t group_count;
	// grows[l][g] is layer l's density of group g in tenths of copies per
	// 100 m², 1 to GROW_MAX, 0 not grown. The layer row's, not its material's.
	uint16_t grows[VOE_ASSETS_LANDSCAPE_LAYERS][VOE_ASSETS_LANDSCAPE_GROUPS];
	// `copy_count` copies, NULL with none.
	voe_assets_landscape_copy *copies;
	uint32_t copy_count;
} voe_assets_landscape;

typedef struct {
	const char *text;
	size_t size;
} voe_assets_landscape_text;

// Every height 0, no layers, paint NULL, no groups, every group zeroed, every
// grow 0, copies NULL. Asserts size in range and cells a multiple of 4 from 4
// to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
voe_assets_landscape voe_assets_landscape_flat(float size, uint32_t cells,
					       voe_base_arena *arena);

// The look a missing key reads as: size 0 (1×), size ±20 %, turn ±180°, tilt
// 0 %, casting shadows, not blending, blend height 0 (50 cm), share 1.
voe_assets_landscape_look voe_assets_landscape_look_default(void);

// Glb `glb` of group `group`'s base size as a factor on its copies' scale: its
// look's size in thousandths, 1 for 0. Asserts group below
// VOE_ASSETS_LANDSCAPE_GROUPS and glb below VOE_ASSETS_LANDSCAPE_GROUP_GLBS.
float voe_assets_landscape_glb_size(const voe_assets_landscape *landscape,
				    uint32_t group, uint32_t glb);

// Group `group`'s seed, the key of every draw made for it (0517 point 4): its
// seed minus one, `group` for 0. Asserts group below
// VOE_ASSETS_LANDSCAPE_GROUPS.
uint32_t voe_assets_landscape_group_seed(const voe_assets_landscape *landscape,
					 uint32_t group);

// The seed a group added at `group_count` takes: `group_count` when no group
// below it has that seed, else the least seed none has. Asserts group_count
// below VOE_ASSETS_LANDSCAPE_GROUPS, so one is always free.
uint32_t voe_assets_landscape_seed_new(const voe_assets_landscape *landscape);

// Removes group `group` (0516, 0520): its glbs, looks, seed, every layer's
// grows column and its copies go; later groups move down one with their glbs,
// looks and growth, and later copies keep their order with their group lowered
// by one. Each later group keeps its seed, made explicit before the move, so
// its draws stay where they were. The freed last slot is zeroed and
// group_count lowered. Nothing allocates. Asserts group below group_count.
void voe_assets_landscape_group_remove(voe_assets_landscape *landscape,
				       uint32_t group);

// The glb of group `group` a copy at x and z in mm draws as (0521 point 3): a
// hash of the group's seed and the position, modulo the shares' total, walked
// over the glbs in order; all shares 0 pick as all 1. LIVE, NOTHING STORED: a
// share or glb edit reaches copies already painted. Asserts group below
// group_count holding a glb.
uint32_t voe_assets_landscape_glb_pick(const voe_assets_landscape *landscape,
				       uint32_t group, int32_t x_mm,
				       int32_t z_mm);

// A new group at group_count holding `path` with the default look and the seed
// voe_assets_landscape_seed_new gives. False, nothing changed, at
// VOE_ASSETS_LANDSCAPE_GROUPS. Asserts `path` not empty and shorter than
// VOE_ASSETS_LANDSCAPE_GLB_PATH, as the three below do.
bool voe_assets_landscape_group_add(voe_assets_landscape *landscape,
				    const char *path);

// Appends `path` to group `group` with the default look. False, nothing
// changed, at VOE_ASSETS_LANDSCAPE_GROUP_GLBS or when the group holds `path`
// (0521 point 4). Asserts group below group_count.
bool voe_assets_landscape_glb_add(voe_assets_landscape *landscape,
				  uint32_t group, const char *path);

// Replaces glb `glb`'s path, keeping its look. False, nothing changed, when
// another glb of the group holds `path`. Asserts glb below the group's count.
bool voe_assets_landscape_glb_set(voe_assets_landscape *landscape,
				  uint32_t group, uint32_t glb,
				  const char *path);

// Removes glb `glb` of group `group`: later glbs move down one with their
// looks and the freed last slot is zeroed. The copies stay, their pick being
// live. Asserts the group holds more than one glb.
void voe_assets_landscape_glb_remove(voe_assets_landscape *landscape,
				     uint32_t group, uint32_t glb);

// `from` at the same size and `cells` cells a side, each height and each
// layer's weight bilinear from `from`'s at the same point of the square, layers
// 1-7 rounded and layer 0 given the rest; the same count is an exact copy. The
// layers and grows are carried, and paint NULL stays NULL. Groups and copies
// are carried unchanged, the copies copied onto `arena`. Asserts cells a
// multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
voe_assets_landscape
voe_assets_landscape_resample(const voe_assets_landscape *from, uint32_t cells,
			      voe_base_arena *arena);

// Reads `size` bytes of text, which need not be NUL-terminated. MALFORMED for
// text the sectioned reader refuses, a missing key or row, a row with too few
// or too many numbers or one not a whole number, a layer path not quoted or of
// VOE_ASSETS_MATERIAL_PATH bytes or more, a weight past 255, or a point whose
// layers 1-7 pass 255. MALFORMED too for both foliage spellings in one file, a
// glb path not quoted, of VOE_ASSETS_LANDSCAPE_GLB_PATH bytes or more or twice
// in one group, a glb's size not a whole number from GLB_SIZE_MIN to
// GLB_SIZE_MAX, a spread or share not a whole number up to its maximum, a
// `shadow` or `blend` other than 0 or 1, a `blendheight` not a whole number
// from BLEND_HEIGHT_MIN to BLEND_HEIGHT_MAX, a `seed<g>` not a whole number
// (one out of 0-15 is clamped), a `grow` key not `grow<l>_<g>` with l below
// layer_count and g below group_count or its density not 1 to GROW_MAX, and a
// copy key out of order or misnamed, of other than six whole numbers, with a
// group not below group_count, or a turn, scale or lean out of range or a
// scale of 0. UNSUPPORTED for a size out of range, cells not a multiple of 4
// from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX, or past
// VOE_ASSETS_LANDSCAPE_COPIES copies. Paint is allocated when there are layers
// and NULL with none; no [Copies] is no copies; a glb's missing look key is
// the default's, a missing grow 0. A line on stderr names what. `out`
// untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_landscape_read(const char *text, size_t size,
					     voe_base_arena *arena,
					     voe_assets_landscape *out,
					     voe_base_error *error);

// As voe_assets_landscape_read, and MALFORMED too for a section other than
// [Landscape], [Layers], [Foliage], [Heights], [Paint] and [Copies], and for a
// key no section takes: a layer, glb, look key, seed, row or paint row past the
// file's counts or misnamed, a `share` in the old spelling, and a `seed<g>`
// out of 0-15. The line on stderr names the line and the key.
[[nodiscard]] bool voe_assets_landscape_read_strict(const char *text,
						    size_t size,
						    voe_base_arena *arena,
						    voe_assets_landscape *out,
						    voe_base_error *error);

// The key table in `src/landscape_keys.c`: the same sections and keys this
// header lists, one row each in its order, ranges from the constants above,
// the old spelling's rows marked read only. `count` gets the rows. Static,
// never freed.
const voe_assets_documented_key *voe_assets_landscape_keys(uint32_t *count);

// The text voe_assets_landscape_read takes, NUL-terminated, `size` not
// counting the NUL. [Layers] only when there are any, each grow of a group
// below group_count after the paths when above 0; [Paint] only when paint is
// not NULL and there are two layers or more, runs of 3 or more as `v*n`;
// [Foliage] only when there are groups, in the group spelling only, a look key
// only off the default (size neither 0 nor 1000, `shadow=0` only when
// shadowless, `blend=1` only when blending, `blendheight` neither 0 nor 50,
// `share` not 1) and `seed<g>` only when the seed is not g; and [Copies] only
// when there are copies, one line a copy. Cannot fail; asserts each layer and
// glb path is NUL-terminated with no line break, each look and grow in range.
voe_assets_landscape_text
voe_assets_landscape_write(const voe_assets_landscape *landscape,
			   voe_base_arena *arena);
