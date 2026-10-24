// One 4×4 block of a GPU-compressed texture, both ways, internal to assets:
// BC7 implemented in src/image_bc7.c, BC5 in src/image_bc5.c. The encode is
// what the cook and the
// editor's queue run on the CPU (0477 point 2); the decode is what a card
// does with the same 16 bytes, so a test can hold the encoder to what draws.
//
// Texels are RGBA8, 64 bytes, row by row, the top row first. A block is the
// 16 bytes in the order the GPU reads them.
//
// MODE 6 ONLY. Of BC7's eight modes the encoder writes mode 6 alone: one pair
// of RGBA endpoints along the block's main axis, 16 levels between them, alpha
// interpolated with the colour. It is the smallest encoder that looks the same
// at a glance (0477 point 2); the partitioned modes come only if the sponsor
// sees blocks, behind this same call. The decode reads mode 6 alone, the one
// mode written, and asserts it.
//
// COLOUR IS ENCODED AS THE BYTES ARE STORED. Nothing here converts: an sRGB
// map's blocks are sRGB, read as sRGB by the format the upload names, and a
// data map's are numbers. The error the encoder minimises is in those bytes.
//
// BC5 is two BC4 blocks, red then green: a normal map's x and y, each with
// eight levels of its own (0477 point 2). Blue and alpha are not kept.
#pragma once

#include <stdint.h>

// `rgba`, sixteen texels, as one BC7 mode 6 block. No allocation; the same
// texels always give the same bytes.
void voe_assets_bc7_encode(const uint8_t rgba[64], uint8_t block[16]);

// A mode 6 `block` as the sixteen texels a card decodes it to.
void voe_assets_bc7_decode(const uint8_t block[16], uint8_t rgba[64]);

// `rgba`'s red and green, sixteen texels, as one BC5 block. No allocation;
// the same texels always give the same bytes.
void voe_assets_bc5_encode(const uint8_t rgba[64], uint8_t block[16]);

// A BC5 `block` as a card decodes it: red and green, blue 0, alpha 255.
void voe_assets_bc5_decode(const uint8_t block[16], uint8_t rgba[64]);
