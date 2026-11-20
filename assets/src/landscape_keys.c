// The landscape file's key table behind voe_assets_landscape_keys: one
// voe_assets_documented_key per key landscape.h's header lists, in its order,
// [Landscape] to [Copies], for the reference text written from it (0511).
//
// A RANGE IS THE HEADER'S CONSTANT, in the file's units: metres for `size`,
// thousandths for a kind's size, centimetres for a blend height, tenths of
// copies per 100 m² for a grow. `refused` is what landscape.c's strict read
// does with a value past it; a row with no single range (heights, copies) has
// none and is not marked. The defaults are spelled as the file spells them and
// match landscape.h's DEFAULT constants by hand: a string cannot be checked
// against a number at compile time.
#include <assets/landscape.h>
#include <base/assert.h>

static const voe_assets_documented_key documented_keys[] = {
	{ "Landscape", "size", "number", "", VOE_ASSETS_LANDSCAPE_SIZE_MIN,
	  VOE_ASSETS_LANDSCAPE_SIZE_MAX, "m", true,
	  "The length of a side; the grid is centred on the origin." },
	{ "Landscape", "cells", "whole number, a multiple of 4", "", 4,
	  VOE_ASSETS_LANDSCAPE_CELLS_MAX, "", true,
	  "Cells per side; the grid has cells + 1 rows of cells + 1 heights." },
	{ "Layers", "layer<N>", "quoted path", "", 0, 0, "", false,
	  "Layer N's .material, from layer0 to the first missing key, at most "
	  "8; layer 0 is the ground." },
	{ "Layers", "grow<L>_<K>", "whole number", "", 1,
	  VOE_ASSETS_LANDSCAPE_GROW_MAX, "tenths of copies per 100 m²", true,
	  "How densely layer L grows kind K, both in the file; absent grows "
	  "none." },
	{ "Foliage", "kind<K>", "quoted path", "", 0, 0, "", false,
	  "Kind K's .glb, from kind0 to the first missing key, at most 16." },
	{ "Foliage", "size<K>", "whole number", "1000",
	  VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN, VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX,
	  "thousandths", true, "Kind K's base size; 1000 is 1×." },
	{ "Foliage", "vary<K>", "whole number", "20", 0,
	  VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX, "%", true,
	  "How far kind K's copies' sizes spread either way." },
	{ "Foliage", "turn<K>", "whole number", "180", 0,
	  VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX, "°", true,
	  "How far kind K's copies' turns spread either way." },
	{ "Foliage", "tilt<K>", "whole number", "0", 0,
	  VOE_ASSETS_LANDSCAPE_TILT_MAX, "%", true,
	  "How far kind K's copies lean with the slope." },
	{ "Foliage", "shadow<K>", "0 or 1", "1", 0, 1, "", true,
	  "Whether kind K's copies cast a shadow." },
	{ "Foliage", "blend<K>", "0 or 1", "0", 0, 1, "", true,
	  "Whether kind K's copies blend with the ground they stand on." },
	{ "Foliage", "blendheight<K>", "whole number", "50",
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN,
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX, "cm", true,
	  "How high a blending kind K's copies blend with the ground." },
	{ "Heights", "row<R>", "cells + 1 whole numbers", "", 0, 0, "mm", false,
	  "Row R's heights up from the origin, at z = -size/2 + R*size/cells, "
	  "value C at x = -size/2 + C*size/cells; every row is required." },
	{ "Paint", "paint<K>_<R>", "cells + 1 whole numbers, v*n for n of v",
	  "", 0, 255, "", true,
	  "Layer K's weights along row R, K from 1; layer 0 is 255 minus the "
	  "rest, and every row of every layer from 1 is required." },
	{ "Copies", "c<I>", "six whole numbers: kind x z turn scale lean", "", 0,
	  0, "", false,
	  "Copy I, in order from c0: its kind, x and z in mm, turn in 1/65536 "
	  "of a turn, scale in thousandths from 1, lean 0 to 255." },
};

const voe_assets_documented_key *voe_assets_landscape_keys(uint32_t *count)
{
	VOE_BASE_ASSERT(count != NULL, "counting landscape keys into nothing");
	*count = (uint32_t)(sizeof(documented_keys) / sizeof(documented_keys[0]));
	return documented_keys;
}
