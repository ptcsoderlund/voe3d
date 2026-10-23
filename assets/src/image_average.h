// The area-average shrink to an exact width and height, internal to assets:
// implemented in src/image_fit.c, called by voe_assets_image_fit with the size
// it computed and by voe_assets_image_mips for each level from the one above.
//
// ONE SHRINK, TWO CALLERS, SO A MIP AND A FITTED PICTURE CANNOT DISAGREE. Each
// kind's average (0466 point 3) lives once; a second copy for the mips would be
// a second place for colour to darken or a normal to shorten.
//
// The caller pushes `out->pixels`, out->width * out->height * 4 bytes, and
// names the size; neither side may be larger than the source's (nothing is
// enlarged) or zero. The working memory is one row of floats in `arena`,
// rewound before the return. Row zero stays the top.
#pragma once

#include <assets/image.h>

// `out`'s pixels made from `source`'s by the area average of `kind`.
void voe_assets_image_average(voe_base_arena *arena,
			      const voe_assets_image *source,
			      const voe_assets_image *out,
			      voe_assets_image_kind kind);
