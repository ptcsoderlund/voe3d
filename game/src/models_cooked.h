// A `.glb`'s cooked copy on disk (0470 points 1, 4 and 5): its path, its key,
// a copy taken when it is usable, and a copy made and written; with the
// folders the copies and the levels files need. Internal to `game`: the batch
// (models_batch.c) takes and writes copies, models.c picks the mode.
//
//     voe_3d_model_cooked_key key =
//             voe_game_models_cooked_key(folder, path, stamp, arena);
//     prepared = voe_game_models_cooked_take(folder, path, key, true, arena);
//     if (prepared == NULL)
//             ... // parse, LODs, decode and mip by kind, bake, then
//             voe_game_models_cooked_save(folder, path, prepared, &model,
//                                         key, device, arena);
//
// THE COPY is `<folder>/Cache/models/<path>.model`. Its key is the platform
// stamps of the `.glb` and of the `.parts` beside it, 0 when there is none. It
// is fresh when 3d reads it and its key is that one.
//
// WHO COOKS, each caller a mode:
//
//     caller                     copy it takes             writes a copy
//     update, prepare            a fresh one               missing or stale
//     prepare's remake           none                      always
//     watch, reload              a fresh one               never
//     update_cooked (the game)   any that reads, no key    missing
//
// The game trusts its copies: files copied into a build get new stamps, so
// it never compares the key (0470 point 5).
//
// A MAP'S COPY (0470 point 2), for the maps a material names, is
// `<folder>/Cache/pictures/<path>.<colour|normal|data>` by the kind its slot
// mips it as, keyed by the map's stamp: the materials call cooks as update,
// the one-material load as watch, materials_cooked (the game) as
// update_cooked (game/src/models_materials.c).
//
// COPIES ARE COMPRESSED (0477 points 1, 3 and 8): a model's pictures and a
// map's chain are BC7 blocks, BC5 for a normal map, compressed on the GPU
// (0486) on the caller's device and thread, so every caller that cooks one
// writes and uploads blocks; a copy of the older RGBA8 version is stale and
// made again.
//
// Constraints: a take reads the whole copy onto the caller's arena, the
// prepare's arrays or the map's pixels pointing into it, so the arena must
// outlive the put. A save is handed each chain and pushes its blocks and the
// copy, a quarter of the chain each.
// Neither says a line for a copy that is missing or broken; a failed write
// is one line on stderr.
#pragma once

#include <3d/model_cooked.h>
#include <3d/model_prepare.h>

#include <assets/image.h>

#include <base/arena.h>
#include <base/error.h>

#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// A batch's cooking mode: the rows of the header's table.
enum voe_game_models_cooking {
	VOE_GAME_MODELS_COOK_STALE,
	VOE_GAME_MODELS_COOK_REMAKE,
	VOE_GAME_MODELS_COOK_NEVER,
	VOE_GAME_MODELS_COOK_MISSING,
};

// Whether `path` ends `.glb`, compared without case.
bool voe_game_models_is_glb(const char *path);

// `folder`'s `.parts` beside `path`: its `.glb` ending swapped, or appended to
// a path not ending `.glb`. On `arena`.
const char *voe_game_models_parts_file(const char *folder, const char *path,
				       voe_base_arena *arena);

// Makes `<folder>/Cache`, `cache` (one folder under it) and each folder of the
// `/`-separated `path` under `cache`, every one only when absent; one another
// thread made between the look and the make counts as made. False, `error`
// set, when one could not be made.
[[nodiscard]] bool voe_game_models_folders_make(const char *folder,
						const char *cache,
						const char *path,
						voe_base_arena *scratch,
						voe_base_error *error);

// `<folder>/Cache/models/<path>.model`, on `arena`.
const char *voe_game_models_cooked_file(const char *folder, const char *path,
					voe_base_arena *arena);

// The key of `folder`'s `path` at the stamp `glb`, with its `.parts`' stamp,
// 0 for none. Scratch rewound.
voe_3d_model_cooked_key voe_game_models_cooked_key(const char *folder,
						   const char *path,
						   uint64_t glb,
						   voe_base_arena *scratch);

// The copy read whole onto `arena` and read as a prepare, its key `key` when
// `compare`. NULL, `arena` rewound, when it is missing, will not read, is
// broken or its key differs.
const voe_3d_model_prepared *
voe_game_models_cooked_take(const char *folder, const char *path,
			    voe_3d_model_cooked_key key, bool compare,
			    voe_base_arena *arena);

// `chain`, a full chain of `kind`, compressed by `kind` (0477) on `device` and
// waited for, the blocks on `arena`. A compression that will not start is
// render's one line and `chain` handed back as it is (0477 point 8).
voe_assets_image voe_game_models_compress(voe_render_device *device,
					  const voe_assets_image *chain,
					  voe_assets_image_kind kind,
					  voe_base_arena *arena);

// `prepared`, baked from `model` (whose pictures 3d keeps opaque), each of
// those pictures, already mipped by its kind (voe_3d_model_cooked_kind),
// compressed on `device`, made
// a copy keyed `key` on `arena` and written atomically with its folders made;
// a failed write is a line on stderr under the caller's subject. A picture
// that would not compress leaves no copy written: 3d writes blocks alone,
// and the next cook tries again.
void voe_game_models_cooked_save(const char *folder, const char *path,
				 const voe_3d_model_prepared *prepared,
				 const voe_assets_model *model,
				 voe_3d_model_cooked_key key,
				 voe_render_device *device,
				 voe_base_arena *arena);

// `<folder>/Cache/pictures/<path>.<colour|normal|data>` by `kind`, on `arena`.
const char *voe_game_models_picture_file(const char *folder, const char *path,
					 voe_assets_image_kind kind,
					 voe_base_arena *arena);

// The map `path`'s copy of `kind` read whole onto `arena` into `image`, its
// pixels pointing into it, its key `stamp` when `compare`. False, `image`
// untouched and `arena` rewound, when it is missing, will not read, is broken,
// of another kind or its key differs.
[[nodiscard]] bool voe_game_models_picture_take(const char *folder,
						const char *path,
						voe_assets_image_kind kind,
						uint64_t stamp, bool compare,
						voe_base_arena *arena,
						voe_assets_image *image);

// `image`, one level, given its full chain of `kind` on `arena`, compressed
// by voe_game_models_compress on `device` and returned, for the caller to
// upload; that written as the map's copy by voe_game_models_picture_write.
voe_assets_image voe_game_models_picture_save(const char *folder,
					      const char *path,
					      const voe_assets_image *image,
					      voe_assets_image_kind kind,
					      uint64_t stamp,
					      voe_render_device *device,
					      voe_base_arena *arena);

// `chain`, as given, made the map `path`'s copy of `kind` keyed `stamp` on
// `arena` and written atomically with its folders made; a failed write is a
// line on stderr under the caller's subject.
void voe_game_models_picture_write(const char *folder, const char *path,
				   const voe_assets_image *chain,
				   voe_assets_image_kind kind, uint64_t stamp,
				   voe_base_arena *arena);
