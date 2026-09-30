// A picture in the model store (ADR-0298 point 5): the one shared quad, the
// decode of a `.png`, `.jpg` or `.jpeg` path's bytes, the built-in soft dot's
// pixels, and the upload of a picture as one COLOUR texture and two BLENDED
// materials. Internal to 3d: only models.c calls it.
//
//     if (voe_3d_model_picture_is(path) &&
//         voe_3d_model_picture_decode(path, bytes, size, scratch, &image, &e) &&
//         voe_3d_model_picture_upload(device, scratch, &image, &upload, &e))
//             ... // upload.materials[0] lit, [1] glow; parts on the quad
//
// THE UPLOAD FILLS A voe_3d_model_upload, the answer model_upload.h gives a
// `.glb`, so the store holds, gives back and frees a picture's texture and
// shading records exactly as it does a model's. Its lists are kept true as it
// goes, so after a failure they are what was made before it.
//
// THE QUAD IS THE STORE'S AND NOT A PICTURE'S: one geometry every picture's
// parts point at, created on the store's first picture or dot and destroyed at
// its clear. A picture entry never destroys it.
//
// CONSTRAINTS: the extension is the whole of the dispatch; a file's own magic
// bytes are not looked at, so a PNG named `.jpg` fails as MALFORMED.
#pragma once

#include "model_upload.h"

#include <assets/image.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The soft dot's side, in pixels.
#define VOE_3D_MODEL_PICTURE_DOT_SIDE 32

// Whether `path` ends `.png`, `.jpg` or `.jpeg`, in any case.
bool voe_3d_model_picture_is(const char *path);

// Decodes `bytes` as the picture `path`'s extension says into `arena`. False,
// with `error` MALFORMED or UNSUPPORTED as the decoder says.
[[nodiscard]] bool voe_3d_model_picture_decode(const char *path,
					       const uint8_t *bytes, size_t size,
					       voe_base_arena *arena,
					       voe_assets_image *image,
					       voe_base_error *error);

// The soft dot's pixels in `arena`: white, alpha 1 at the centre to 0 at the
// edge.
voe_assets_image voe_3d_model_picture_dot(voe_base_arena *arena);

// Uploads the quad: one metre a side in XY, centred, facing +Z. False, with
// `error` set, when the device has no room.
[[nodiscard]] bool voe_3d_model_picture_quad(voe_render_device *device,
					     voe_render_geometry *out,
					     voe_base_error *error);

// Uploads `image` as one COLOUR texture and its two materials, both BLENDED on
// it: materials[0] lit, materials[1] unlit. Arrays in `arena`. False when
// `render` has no room, with `out` listing what was made.
[[nodiscard]] bool voe_3d_model_picture_upload(voe_render_device *device,
					       voe_base_arena *arena,
					       const voe_assets_image *image,
					       voe_3d_model_upload *out,
					       voe_base_error *error);
