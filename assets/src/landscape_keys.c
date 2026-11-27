// The landscape file's key table behind voe_assets_landscape_keys: one
// voe_assets_documented_key per key landscape.h's header lists, in its order,
// [Landscape] to [Copies], for the reference text written from it (0511).
//
// A RANGE IS THE HEADER'S CONSTANT, in the file's units: metres for `size`,
// thousandths for a glb's size, centimetres for a blend height, tenths of
// copies per 100 m² for a grow, a group's seed 0 to GROUPS - 1. `refused` is
// what landscape.c's strict read does with a value past it; a row with no
// single range (heights, copies) has none and is not marked. The defaults are
// spelled as the file spells them and match landscape.h's DEFAULT constants by
// hand: a string cannot be checked against a number at compile time. A seed's
// default is `G`, the group's own index.
//
// THE OLD SPELLING'S ROWS FOLLOW THE GROUPS' (0521 point 1): `kind<K>` and its
// `<key><K>` look keys are read only, each kind as a one-glb group, and never
// written.
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
	{ "Layers", "grow<L>_<G>", "whole number", "", 1,
	  VOE_ASSETS_LANDSCAPE_GROW_MAX, "tenths of copies per 100 m²", true,
	  "How densely layer L grows group G, both in the file; absent grows "
	  "none." },
	{ "Foliage", "glb<G>_<J>", "quoted path", "", 0, 0, "", false,
	  "Glb J of group G, from glb<G>_0 to the first missing key, at most 8 "
	  "and each path once; groups from glb0_0 to the first missing, at most "
	  "16." },
	{ "Foliage", "size<G>_<J>", "whole number", "1000",
	  VOE_ASSETS_LANDSCAPE_GLB_SIZE_MIN, VOE_ASSETS_LANDSCAPE_GLB_SIZE_MAX,
	  "thousandths", true, "Glb J of group G's base size; 1000 is 1×." },
	{ "Foliage", "vary<G>_<J>", "whole number", "20", 0,
	  VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX, "%", true,
	  "How far glb J of group G's copies' sizes spread either way." },
	{ "Foliage", "turn<G>_<J>", "whole number", "180", 0,
	  VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX, "°", true,
	  "How far glb J of group G's copies' turns spread either way." },
	{ "Foliage", "tilt<G>_<J>", "whole number", "0", 0,
	  VOE_ASSETS_LANDSCAPE_TILT_MAX, "%", true,
	  "How far glb J of group G's copies lean with the slope." },
	{ "Foliage", "shadow<G>_<J>", "0 or 1", "1", 0, 1, "", true,
	  "Whether glb J of group G's copies cast a shadow." },
	{ "Foliage", "blend<G>_<J>", "0 or 1", "0", 0, 1, "", true,
	  "Whether glb J of group G's copies blend with the ground they stand "
	  "on." },
	{ "Foliage", "blendheight<G>_<J>", "whole number", "50",
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN,
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX, "cm", true,
	  "How high glb J of group G's blending copies blend with the ground." },
	{ "Foliage", "share<G>_<J>", "whole number", "1", 0,
	  VOE_ASSETS_LANDSCAPE_SHARE_MAX, "", true,
	  "Glb J's share of group G's copies against its other glbs'; a group "
	  "whose shares are all 0 picks as if all were 1." },
	{ "Foliage", "seed<G>", "whole number", "G", 0,
	  VOE_ASSETS_LANDSCAPE_GROUPS - 1, "", true,
	  "The seed of group G's draws, which it keeps when a group before it is "
	  "removed; absent is G." },
	{ "Foliage", "kind<K>", "quoted path", "", 0, 0, "", false,
	  "Read only, as a one-glb group K: the old spelling, never in a file "
	  "with a glb key." },
	{ "Foliage", "size<K>", "whole number", "1000",
	  VOE_ASSETS_LANDSCAPE_GLB_SIZE_MIN, VOE_ASSETS_LANDSCAPE_GLB_SIZE_MAX,
	  "thousandths", true, "Read only, as a one-glb group: size<K>_0." },
	{ "Foliage", "vary<K>", "whole number", "20", 0,
	  VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX, "%", true,
	  "Read only, as a one-glb group: vary<K>_0." },
	{ "Foliage", "turn<K>", "whole number", "180", 0,
	  VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX, "°", true,
	  "Read only, as a one-glb group: turn<K>_0." },
	{ "Foliage", "tilt<K>", "whole number", "0", 0,
	  VOE_ASSETS_LANDSCAPE_TILT_MAX, "%", true,
	  "Read only, as a one-glb group: tilt<K>_0." },
	{ "Foliage", "shadow<K>", "0 or 1", "1", 0, 1, "", true,
	  "Read only, as a one-glb group: shadow<K>_0." },
	{ "Foliage", "blend<K>", "0 or 1", "0", 0, 1, "", true,
	  "Read only, as a one-glb group: blend<K>_0." },
	{ "Foliage", "blendheight<K>", "whole number", "50",
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN,
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX, "cm", true,
	  "Read only, as a one-glb group: blendheight<K>_0." },
	{ "Heights", "row<R>", "cells + 1 whole numbers", "", 0, 0, "mm", false,
	  "Row R's heights up from the origin, at z = -size/2 + R*size/cells, "
	  "value C at x = -size/2 + C*size/cells; every row is required." },
	{ "Paint", "paint<K>_<R>", "cells + 1 whole numbers, v*n for n of v",
	  "", 0, 255, "", true,
	  "Layer K's weights along row R, K from 1; layer 0 is 255 minus the "
	  "rest, and every row of every layer from 1 is required." },
	{ "Copies", "c<I>", "six whole numbers: group x z turn scale lean", "", 0,
	  0, "", false,
	  "Copy I, in order from c0: its group, x and z in mm, turn in 1/65536 "
	  "of a turn, scale in thousandths from 1, lean 0 to 255." },
};

const voe_assets_documented_key *voe_assets_landscape_keys(uint32_t *count)
{
	VOE_BASE_ASSERT(count != NULL, "counting landscape keys into nothing");
	*count = (uint32_t)(sizeof(documented_keys) / sizeof(documented_keys[0]));
	return documented_keys;
}
