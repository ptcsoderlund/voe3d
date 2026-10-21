// A read `.glb` made ready for the model store on any thread: the first half
// of a load (0448 point 3), the put (voe_3d_models_put, 3d/models.h) being the
// second, on the one thread that owns the device.
//
//     voe_assets_model model;
//     const voe_3d_model_prepared *prepared;
//
//     if (!voe_assets_model_read_glb_encoded(glb, size, arena, &model, &error))
//             ... // said once by the reader
//     prepared = voe_3d_model_prepare(arena, &model, glb, size, levels,
//                                     levels_size, &error);
//     ... // each picture decoded into model.images (assets/model.h)
//     if (prepared != NULL)
//             loaded = voe_3d_models_put(models, device, path, stamp, prepared,
//                                        &error);
//     voe_base_arena_destroy(arena);   // after the put, never before
//
// THE SPLIT IS THE DEVICE AND THE STORE. The prepare bakes level 0, the file's
// own levels or the made ones its levels file holds, checked against the bake
// as the store's load checks them, the CPU shape with its welded edges and the
// bounding sphere; it touches no device, no store and nothing shared but the
// report's subjects, which are each thread's own, so several files prepare side
// by side, each on an arena of its own. The put uploads and keeps.
//
// THE MODEL IS KEPT BY POINTER, not copied, so its pictures may be decoded
// after the prepare, on other threads, into `model->images`; the put asserts
// they are by then. The prepare reads none of them.
//
// A STALE OR WRONG LEVELS FILE IS NO FAILURE: the put then makes level 0 alone,
// as the store's load does.
//
// A PICTURE IS DECODED HERE TOO, a model's or a material's map: by its path's
// extension, on any thread with an arena of its own, for the put or for
// voe_3d_models_put_material (3d/models.h).
//
// CONSTRAINTS: everything is pushed into `arena`, which must outlive the put;
// the shape is copied out of it by the put. The decode looks at the extension
// alone, not the file's magic bytes, so a PNG named `.jpg` fails MALFORMED.
#pragma once

#include <assets/image.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct voe_3d_model_prepared voe_3d_model_prepared;

// `model`, read from `glb`, baked on `arena` with its levels: its own when its
// nodes name them, else the made ones of `levels`, a levels file read against
// `glb`, NULL for none. NULL with `error` set, said once by the bake under the
// caller's subject (0445): UNSUPPORTED for more than VOE_3D_MODEL_PARTS
// materials or too deep a tree, REFUSED for more than a 32-bit count holds.
[[nodiscard]] const voe_3d_model_prepared *
voe_3d_model_prepare(voe_base_arena *arena, const voe_assets_model *model,
		     const uint8_t *glb, size_t glb_size, const uint8_t *levels,
		     size_t levels_size, voe_base_error *error);

// Whether `path` ends `.png`, `.jpg` or `.jpeg`, in any case.
bool voe_3d_model_picture_is(const char *path);

// Decodes `bytes` as the picture `path`'s extension says into `arena`. False,
// with `error` MALFORMED or UNSUPPORTED as the decoder says. Asserts `path` is
// a picture's.
[[nodiscard]] bool voe_3d_model_picture_decode(const char *path,
					       const uint8_t *bytes, size_t size,
					       voe_base_arena *arena,
					       voe_assets_image *image,
					       voe_base_error *error);
