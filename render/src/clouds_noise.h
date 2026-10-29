// The cloud field's two noise textures, baked on the CPU (ADR-0492 point 4).
// Pure CPU, no Vulkan: the device makes both once at open and uploads them; the
// tests hold them to their claims.
//
//     uint8_t weather[VOE_RENDER_CLOUDS_WEATHER_SIZE *
//                     VOE_RENDER_CLOUDS_WEATHER_SIZE];
//     voe_render_clouds_weather(weather);
//     voe_render_clouds_detail(detail); // 64 × 64 × 64 bytes
//
// THE WEATHER is the clouds' shapes over one VOE_RENDER_CLOUDS_PERIOD, 256 ×
// 256 bytes row by row, 32 m a texel: Perlin fBm whose lattice periods divide
// the size, so it tiles.
//
// THE DETAIL erodes those shapes: 64 × 64 × 64 bytes, x fastest then y then z,
// repeating every 256 m (4 m a texel): inverted Worley fBm, its cells wrapped,
// so it tiles. 256 m divides the period, so an origin wrapped back to 0 shows
// no seam in either field.
//
// THE SAME BYTES EVERYWHERE. No state, no randomness beyond a fixed hash, and
// integer arithmetic only, so every run and every machine gets the same bytes.
// The weather is stretched over the whole byte, darkest 0 and brightest 255;
// the detail is its sum over the most it could be, 255.
//
// WHY BAKED. Computed per sample, the noise would be tens of evaluations a
// march step at every pixel; baked, a sample is one texture read, and the
// march, the shadows and the reflections read the one field, so all three are
// the same clouds. The twin reads are in `shaders/clouds.slangh`.
#pragma once

#include <render/device.h>

#include <stdint.h>

#define VOE_RENDER_CLOUDS_WEATHER_SIZE 256
#define VOE_RENDER_CLOUDS_DETAIL_SIZE 64

// Fills `texels`, VOE_RENDER_CLOUDS_WEATHER_SIZE² bytes, with the weather map.
void voe_render_clouds_weather(uint8_t *texels);

// Fills `texels`, VOE_RENDER_CLOUDS_DETAIL_SIZE³ bytes, with the detail volume.
void voe_render_clouds_detail(uint8_t *texels);
