// A PNG file read into a colour texture with its size: the splash a program
// shows while it starts and loads (decision 0356).
//
//     voe_app_picture splash;
//     if (voe_app_picture_read(device, "splashscreen.png", scratch, &splash,
//                              &error))
//             ... draw splash.texture at splash.width by splash.height ...
//     voe_render_texture_destroy(device, splash.texture);
//
// THREE STEPS. The file is read whole (platform/file.h), decoded to RGBA8
// (assets/image.h's voe_assets_png_decode) and uploaded as
// VOE_RENDER_TEXTURE_COLOUR with clamped sampling (VOE_RENDER_SAMPLING_SHARP
// in render/device.h's voe_render_texture_create), so a coordinate a hair past
// an edge reads the edge and not the far side.
//
// FALSE, WITH `error` SET AND `out` UNTOUCHED, for a missing file, bytes that
// are not a PNG, or an upload the card refuses; the step that failed has
// already said why on stderr where it can.
//
// WHY IT LIVES IN `app`: `game` may not name `assets` (0356), and a picture
// read into a texture is a frame loop's need both programs share.
//
// Constraints: the file and the decoded pixels are pushed into `scratch`,
// which the caller rewinds; a large picture needs about twice its RGBA size
// there (the file plus the pixels). A startup operation: the upload waits for
// the GPU to go idle. The texture is the caller's, given back with
// voe_render_texture_destroy before the device closes.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <stdint.h>

typedef struct {
	voe_render_texture texture;
	uint32_t width;
	uint32_t height;
} voe_app_picture;

// Reads the PNG at path into a texture on device. False with error set, out
// untouched, when the file is missing, is not a PNG, or the upload fails.
[[nodiscard]] bool voe_app_picture_read(voe_render_device *device,
					const char *path,
					voe_base_arena *scratch,
					voe_app_picture *out,
					voe_base_error *error);
