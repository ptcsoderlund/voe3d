// One picture handed to `render` as one SMOOTH texture, whatever shape it came
// in: the call a model's pictures (model_upload.c) and a material's maps
// (models_material.c) both go through, so the two never upload two ways.
// Internal to 3d.
//
//     voe_render_texture id;
//
//     if (!voe_3d_picture_upload(device, &image, VOE_RENDER_TEXTURE_COLOUR,
//                                &id, &error))
//             ... // nothing was made; `error` says why
//
// THREE SHAPES, ONE CALL (0477 point 1). A BC7 or BC5 picture goes up as its
// blocks, as they are, its format naming the blocks; an RGBA8 picture carrying
// its levels goes up with them (0470 point 3); one of a single level goes up
// SMOOTH and the GPU mips it.
//
// Constraints: a picture of width or height 0, or of no pixels, is the
// caller's bug and asserts. It fails as `render`'s create does: REFUSED with
// one line when the card has no room, or no BC support for blocks. Like every
// texture create, it waits for the card, so it runs between frames on the
// device's thread.
#pragma once

#include <assets/image.h>
#include <base/error.h>
#include <render/device.h>

#include <stdbool.h>

// `image` uploaded as `kind` into `out`, in whichever of the three shapes it
// is; false, nothing made and `error` set, when `render` refuses.
[[nodiscard]] bool voe_3d_picture_upload(voe_render_device *device,
					 const voe_assets_image *image,
					 voe_render_texture_kind kind,
					 voe_render_texture *out,
					 voe_base_error *error);
