// The engine's two pipelines: a vertex out of the shared pool, placed by three matrices —
// two of them the camera's, shared by every draw in a frame, and one of them the
// object's, read out of a buffer by the object's own number — lit by the pass's
// directional lights and its point lights, and shaded by a record read
// the same way.
//
// ONE FILE, TWO PIPELINES. The solid one and the blended one are built from this
// same module (device.c); which of them a draw goes through is decided on the
// CPU by which draw call was made, and what this file does with alpha is decided
// by the shading record's own mode. The two have to agree and nothing checks it.
//
// THE ONLY PUSH CONSTANT IS THE OBJECT'S NUMBER, AND EVERYTHING ELSE IS A BUFFER
// READ. That is what a draw is told that the draw beside it is not; the matrix
// and the shading it names are in memory both stages can index. It is the shape
// an indirect draw needs — the number would come from the instance index instead
// and nothing else here would change — and it is why the CPU side has no push
// constant bigger than four bytes.
//
// THREE NUMBERS AND NONE OF THEM IS A HANDLE: object, shading, texture. The
// object number picks a record, the record names a shading index, the shading
// record names texture indices, and a texture index is a subscript into the
// array below. Every one of them is the same integer the CPU handed out
// (ADR-0018), so there is no table anywhere between the two sides of the bus.
//
// ONE FILE, SEVEN ENTRY POINTS, ONE SPIR-V MODULE. slangc compiles this whole file
// to draw.spv with all of them in it (the point-shadow pass's vertex stage, which writes the layer, the capture
// pass's two and the sky's two), and pipeline.c names them by the names
// spelled here — see -fvk-use-entrypoint-name in cmake/voe.cmake, which is what
// keeps those strings true.
//
// THE MATRICES ARE READ ROW-MAJOR AND THAT COSTS A FLAG. slangc is invoked with
// -matrix-layout-row-major, so float4x4 here has the layout voe_math_float4x4
// has in C and an upload is a straight copy of sixteen floats. slangc's own
// default is the other one and getting it wrong transposes every transform
// without failing to compile — which is why matrix_probe.slang exists and why
// render/tests/matrix.c refuses to pass without the flag.
//
// mul(M, v) IS M·v, AND COMPOSITION READS RIGHT TO LEFT. Vectors are columns, so
// the world matrix is applied first and appears innermost below. That is the
// same convention voe_math_float4x4_mul_float4 implements on the CPU.
//
// NOTHING HERE FLIPS Y. Vulkan's clip space is Y-down and this engine is Y-up,
// and the whole of that reconciliation is the negative viewport height in
// frame.c. A projection matrix that negated a row as well would flip twice,
// which looks exactly like not flipping at all until something is culled.
//
// NOTHING HERE FLIPS THE TEXTURE COORDINATE EITHER. (0,0) is the top-left of the
// image in Vulkan and in glTF, and assets decodes rows top-down, so all three
// agree and the picture comes out the right way up. The flip everyone adds is
// OpenGL's, whose texture origin is at the bottom.
//
// NINE PARTS ARE INCLUDED, IN THE ORDER THEY DEPEND ON EACH OTHER (ADR-0308).
// tone.slangh, after lighting.slangh, hazes, exposes and tone maps every exit;
// lighting.slangh includes sky.slangh, the sky's radiance, twin of src/sky.c.
// bindings.slangh first: the frame block, the records and the bindings both
// stages read; instances.slangh, a copy's place (ADR-0442). lighting.slangh: the sun, its cascades, the fill, the BRDF
// terms and voe_render_points, the point lights the lit exit adds to the sun's
// direct term and the unshaded one does not, and the light blockers that gate
// the sun, the fill and the lamps by Rooms and crossings (ADR-0350), with the
// decisions in its header;
// it includes point_shadow.slangh, a cube face's pick and projection. terrain.slangh
// after the vertex input: a landscape node's vertex (ADR-0396). water.slangh
// after the vertex stage and voe_render_premultiplied, which it uses.
// terrain_layers.slangh last, after voe_render_repeated_uv: a painted
// landscape's layers blended by height, hex-tiled (ADR-0402). A part is
// never compiled alone, and cmake/voe.cmake rebuilds every shader when one
// changes.
//
// ---- THE FRAGMENT STAGE'S EXITS, AND EVERY DECISION IN THEM ----
//
// A SURFACE WITH NO NORMAL IS DRAWN UNLIT, AND THIS IS THE CARD THAT DECIDED IT.
// A file without normals leaves them at nothing (3d/src/import.c), and
// normalizing a zero vector is a NaN that spreads to the whole fragment. So a
// normal of nothing is answered by drawing the surface as its base colour with
// no light on it: visibly flatter than everything around it, finite, and
// obviously not lit — where a black object would read as a light pointing the
// wrong way. glTF says a client should compute flat face normals for such a
// file; doing that means splitting shared vertices at import and it is a card of
// its own.
//
// OCCLUSION MULTIPLIES THE FILL AND THE BOUNCE, AS glTF SPECIFIES (ADR-0400). An
// occlusion map's red — a `.material`'s ORM map or a glTF model's occlusion
// texture, both the record's occlusion_texture — darkens the indirect term: the
// bounce with the fills as its floor. Never the sun's direct light, a lamp's, or
// shadows: baked occlusion on direct light darkens what the shadow maps already
// darken, and the shadows come out doubled. Emission is not occluded either.
// An empty slot samples white, occlusion one, and changes nothing.
//
// AN UNLIT MATERIAL SKIPS ALL OF THE ABOVE, AND THAT IS A FLAG ON THE RECORD.
// Its base colour, times its base colour texture, is the answer; no sun, no
// BRDF, no occlusion. Text is what asked for it — a letter is a shape somebody
// chose the colour of, and a sun crossing it is a bug rather than an effect —
// and a user interface will want the same thing. It is not the same as a surface
// with no normal, which takes the same exit for the opposite reason: that one is
// a file that did not say, this one is a material that did.
//
// A PASS WITH NO SUN TAKES THE UNLIT EXIT FOR EVERY SURFACE. The light's
// `unshaded` is a pass asking for base colours, and then each surface is its
// base colour; no folder sets it today (ADR-0290). A scene with no light is not
// that: it is the zeroed light, which draws lit surfaces black (ADR-0287).
//
// A NORMAL MAP TILTS THE SHADING NORMAL THROUGH A FRAME BUILT PER PIXEL
// (ADR-0278). No tangent attribute: the tangent (+u) and bitangent (−v, glTF's
// green) come from the screen derivatives of the world position and the UV,
// orthogonalised against the interpolated normal, x and y map 0..1 to −1..1
// and z is made from them. The interpolated normal places the shadow lookup.
// Seams where a frame flips across a UV island are the cost; a tangent
// attribute is the card that lifts it. A mirrored half is right by the determinant's sign,
// its bump turned over with its picture: render/tests/surface_maps.c says so.
//
// EMISSION IS ADDED LAST: the record's emissive times the emissive map, after
// the sun and the fill, unshadowed and unoccluded. Only on the lit path; an
// unlit exit is its base colour. Under the Clip curve a strong one clips.
//
// A VERTEX'S COLOUR MULTIPLIES THE BASE COLOUR, ALPHA INCLUDED (ADR-0421 point
// 1). The vertex carries its complement, so the colour is 1 − that and a zeroed
// vertex is white. It is applied as `base` is formed, before the alpha mode, so
// the cut, the lit, unlit and water exits and the capture's albedo all see it;
// a landscape node's vertex reads white.
//
// A BACK FACE IS SHADED WITH ITS INTERPOLATED NORMAL TURNED ROUND (ADR-0421
// point 2), before the normal map and the lights read it, so a two-sided draw
// is lit from the side seen. Safe for a one-sided draw: its back faces are
// culled and never reach the fragment stage, so every face it shades is front.
//
// A DISTANCE-FIELD BASE COLOUR IS THE THIRD UNIFORM BRANCH IN THIS FILE, AND IT
// IS SPENT KNOWINGLY. `unlit` was the second and the alpha mode was the first;
// there is a standing watch to reconsider the whole arrangement at the fourth,
// so the next person to want one reports it rather than adding it.
//
// WATER IS THE FOURTH, AND IT WAS REPORTED (ADR-0305). A `water` record is a
// blended plane: four sine waves bend its normal and move no vertex, it is lit
// by the same sun, shadow and fill, and its thickness over the pass's depth copy
// sets its coverage and darkens it. Its own function, voe_render_water, in
// water.slangh.
//
// WHAT THE BRANCH DOES: the sheet's three channels are three signed distances to
// the outline, each measured to a different subset of its edges. Their MEDIAN is
// the distance the shape actually has — which is the whole trick, because at a
// corner two of the three agree and the third is the one that would have rounded
// it off. Alpha is then computed rather than sampled: a step at the halfway mark,
// which is the outline. The colour is the base colour factor, untouched — the
// sheet's rgb is numbers and multiplying by it would be arithmetic on distances.
//
// THE STEP IS HARD BECAUSE WORLD TEXT IS NOT ANTIALIASED. Interface text is:
// elements.slang covers its edge across one screen pixel (ADR-0269). World
// text has no caller that shows it small, so it keeps the hard cut. What the
// field buys here is not a soft edge — it is an edge in the right place at any
// size, which is what keeps a hard cut looking like a letter rather than blocks.
//
// ---- ALPHA, AND THE ONE MULTIPLY AT THE END ----
//
// THE ALPHA MODE DECIDES ALPHA AND THE LAST ACT IS ALWAYS colour.rgb *= colour.a.
// Three paths and one multiply:
//
//   opaque   — alpha forced to 1 and the file's value ignored entirely, which is
//              what the glTF specification says an OPAQUE material's alpha
//              means. THIS LINE IS WHAT MAKES THE UNCONDITIONAL MULTIPLY SAFE.
//              Without it, an opaque material carrying a = 0.5 would write half
//              its colour into a pass that does not blend and the surface would
//              simply go dark — which reads as a lighting regression and is the
//              single most likely bug in this whole arrangement.
//   cutout   — discard below the cutoff, then alpha forced to 1. What survives
//              is fully solid and writes depth like anything else. Before
//              the cut the coverage is scaled by 1 + 0.25 × the base colour
//              map's level of detail (ADR-0421 point 3, Ben Golus' method):
//              a mip averages thin opaque texels below the cut, so leaves
//              thin away with distance, and the factor puts back roughly
//              what averaging took. Approximate, not exact coverage. A
//              distance-field record keeps its cut unscaled.
//   blended  — alpha exactly as computed, and the pass it is drawn in blends.
// Beside them an object's `dither` discards the pixels its 4 × 4 pattern drops.
//
// THE COLOUR TARGET HOLDS PREMULTIPLIED COLOUR, AND ANYTHING THAT WRITES INTO IT
// OUTPUTS PREMULTIPLIED COLOUR. That is the engine's contract, it is stated in
// render/include/render/device.h as well, and nothing enforces it: the blend
// state is source-ONE, destination-ONE_MINUS_SRC_ALPHA, so a fragment that had
// not already multiplied would come out too bright and one that multiplied twice
// too dark. The next author of a shader that writes into this target is the
// whole of the mechanism.
//
// THE BASE COLOUR'S ALPHA IS ALREADY LINEAR AND NEEDS NO DECODING. A Vulkan sRGB
// format decodes the three colour channels and passes the fourth through
// untouched, so the alpha of an sRGB base-colour texture is correct as sampled.
// Do not "fix" it.
//
// AND A DISTANCE-FIELD SHEET IS NOT AN sRGB FORMAT AT ALL. It is uploaded as
// VOE_RENDER_TEXTURE_DATA, so nothing decodes anything on the way in and the
// distances arrive as they were written. Uploaded as colour they would each be
// bent towards zero and every edge in the font would land slightly in the wrong
// place — uniformly, everywhere, which reads as the spread being wrong rather
// than as a format mistake.
//
// EVERYTHING IN HERE IS LINEAR LIGHT. A colour texture is an sRGB format, so
// the hardware has already decoded it by the time it reaches this file; the
// numbers written out are linear and the target's own sRGB format encodes them
// once as it stores them. There is no gamma arithmetic in this shader and there
// must not be — that is the double-encode that makes everything pale.
//
// EVERY EXIT IS HAZED, THEN EXPOSED AND TONE MAPPED, BY tone.slangh (ADR-0405
// points 6 and 7), on its straight colour before the premultiply, as the pass's
// sky record says; a zeroed one is no haze, ×1 and Clip, so a bright enough
// light clips to white.

#include "bindings.slangh"
#include "instances.slangh"
#include "lighting.slangh"
#include "tone.slangh"

// The locations are stated rather than left to declaration order, because the C
// side names the same four numbers in its VkVertexInputAttributeDescription and
// two lists that have to agree should not agree by counting.
struct voe_render_draw_vertex_in {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float2 uv : TEXCOORD;
	// The colour's complement, UNORM: the colour is 1 − this.
	[[vk::location(3)]] float4 colour_complement : COLOR;
};

#include "terrain.slangh"

// What crosses between the stages: the clip-space position the rasteriser wants,
// and the three things the lighting needs in world space. `world` is there
// because a specular highlight needs the direction from the surface to the eye,
// and reconstructing a world position from a depth buffer is a later card's
// trick for a later card's reason.
struct voe_render_draw_vertex_out {
	float4 position : SV_Position;
	float3 world : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	// The model's own x and y, which the water path measures its waves over.
	float2 plane : TEXCOORD1;
	// The vertex's colour, linear rgba: what multiplies `base`.
	float4 colour : COLOR;
	// The draw's dither, the record's or its copy's (instances.slangh).
	nointerpolation float dither : TEXCOORD2;
};

[shader("vertex")]
voe_render_draw_vertex_out
voe_render_draw_vertex(voe_render_draw_vertex_in vertex,
		       uint instance : SV_InstanceID)
{
	voe_render_draw_vertex_out out;
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	voe_render_placement placed = voe_render_placed(object, instance);
	float4 world;
	float4 eye;

	if (object.heights != 0)
		vertex = voe_render_terrain(object, vertex);
	world = mul(placed.world, float4(vertex.position, 1.0));
	eye = mul(voe_render_frame.camera.view, world);
	out.position = mul(voe_render_frame.camera.projection, eye);

	// A direction and not a point, so w is zero and the normal matrix's
	// last column — whatever the inverse transpose left there — is ignored.
	// Not normalized here: it is interpolated across the triangle and the
	// fragment stage has to normalize what arrives anyway.
	out.world = world.xyz;
	out.normal = mul(placed.normal, float4(vertex.normal, 0.0)).xyz;
	out.uv = vertex.uv;
	out.plane = vertex.position.xy;
	out.colour = 1.0 - vertex.colour_complement;
	out.dither = placed.dither;
	return out;
}

// What the point-shadow vertex stage hands the rasteriser: the clip position on
// one face and the layer that face is (ADR-0325 point 2).
struct voe_render_point_shadow_out {
	float4 position : SV_Position;
	uint layer : SV_RenderTargetArrayIndex;
};

// The `n`-th set bit of the push constant's 96-bit face mask, counting from the
// lowest; the CPU draws exactly as many instances as bits are set.
uint voe_render_point_shadow_layer(uint n)
{
	uint words[3] = { voe_render_draw_current.faces0,
			  voe_render_draw_current.faces1,
			  voe_render_draw_current.faces2 };

	for (uint w = 0; w < 3; w++) {
		uint bits = words[w];
		uint count = countbits(bits);

		if (n < count) {
			for (uint i = 0; i < n; i++)
				bits &= bits - 1;
			return w * 32 + firstbitlow(bits);
		}
		n -= count;
	}
	return 0;
}

// THE POINT-SHADOW PASS'S VERTEX STAGE: instance n draws onto the layer of the
// mask's n-th set bit, slot layer / 6 + 1's face layer % 6, the light read from
// this pass's region at index slot − 1. No fragment stage follows.
[shader("vertex")]
voe_render_point_shadow_out
voe_render_point_shadow_vertex(voe_render_draw_vertex_in vertex,
			       uint instance : SV_InstanceID)
{
	voe_render_point_shadow_out out;
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	uint layer = voe_render_point_shadow_layer(instance);
	voe_render_point_light light = voe_render_point_lights[
		voe_render_frame.region * VOE_RENDER_POINT_LIGHTS + layer / 6];

	if (object.heights != 0)
		vertex = voe_render_terrain(object, vertex);
	float4 world = mul(object.world, float4(vertex.position, 1.0));

	out.position = voe_render_point_shadow_clip(world.xyz - light.position,
						    layer % 6, light.range);
	out.layer = layer;
	return out;
}

// THE LAST ACT OF EVERY PATH OUT OF THE FRAGMENT STAGE, AND THE ENGINE'S
// CONTRACT IN ONE LINE: the colour target holds premultiplied colour, so a
// fragment leaves here with its rgb already multiplied by its own alpha. The
// blend state is source-ONE, destination-ONE_MINUS_SRC_ALPHA to match. It is
// unconditional — the opaque path forces the alpha to one first, which is what
// makes multiplying by it a no-op there rather than a surface going dark.
float4 voe_render_premultiplied(float3 colour, float coverage)
{
	return float4(colour * coverage, coverage);
}

// The middle of three, which is what turns three one-sided distance fields back
// into one shape with its corners intact. Written out rather than sorted: three
// min/max are cheaper than any sort and there is no branch in them.
//
// THERE IS A SECOND COPY, IN elements.slang, CALLED voe_render_element_median.
// The two are the same three min/max and must stay the same, or one sheet comes
// out as two different letters depending on which path drew it. It is copied and
// not shared because the build compiles every .slang in shaders/ as a module of
// its own and tracks no #include dependency, so a shared file would be compiled
// on its own as well and would leave both dependents stale when it changed.
// Change one, change the other.
float voe_render_median(float3 field)
{
	return max(min(field.r, field.g), min(max(field.r, field.g), field.b));
}

// The pixel's threshold in the 4 × 4 ordered (Bayer) pattern, (index + 0.5) ÷ 16:
// sixteen distinct values in 0..1, so a share d keeps exactly 16 d of every
// sixteen pixels and −d the rest (ADR-0432 point 5).
float voe_render_dither_threshold(float2 pixel)
{
	static const uint bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6,
					3, 11, 1, 9, 15, 7, 13, 5 };
	uint2 at = uint2(pixel) & 3u;

	return (float(bayer[at.y * 4 + at.x]) + 0.5) / 16.0;
}

// Slot 0, the white pixel: voe_render_texture in render/device.h.
static const uint VOE_RENDER_NO_TEXTURE = 0;

// The shading normal a normal map gives `normal` (ADR-0278 point 3). dP/du and
// dP/dv come from solving the screen derivatives of the world position against
// those of the UV; the determinant's sign is the frame's handedness, and it
// multiplies rather than divides because only the direction is kept. Each axis
// is orthogonalised against the interpolated normal. glTF's green runs along
// −v, because its v runs down. A UV with no area on screen, or an axis along
// the normal, has no frame, and the interpolated normal is the answer.
//
// THE TEXEL'S BLUE IS NEVER READ (ADR-0477 point 1). A BC5 normal map holds x
// and y alone and samples blue as nought, so z is made again from them, the
// unit vector's own z, for every map: one still waiting for compression, its
// blue whatever the file wrote, draws the same as its compressed copy. A
// painted landscape's blended texel comes through here too.
float3 voe_render_mapped_normal(float3 normal, float3 texel, float3 world_dx,
				float3 world_dy, float2 uv_dx, float2 uv_dy)
{
	float det = uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y;
	float hand = det < 0.0 ? -1.0 : 1.0;
	float3 along_u = (world_dx * uv_dy.y - world_dy * uv_dx.y) * hand;
	float3 along_v = (world_dy * uv_dx.x - world_dx * uv_dy.x) * hand;

	along_u -= normal * dot(normal, along_u);
	along_v -= normal * dot(normal, along_v);
	// Not VOE_LEAST: these are per-pixel steps, a millionth apart on a far
	// surface, and a frame that small is still a frame.
	if (det == 0.0 || dot(along_u, along_u) < 1e-30 ||
	    dot(along_v, along_v) < 1e-30)
		return normal;

	float3 m;
	m.xy = texel.xy * 2.0 - 1.0;
	m.z = sqrt(saturate(1.0 - dot(m.xy, m.xy)));
	float3 mapped = normalize(along_u) * m.x - normalize(along_v) * m.y +
			normal * m.z;
	if (dot(mapped, mapped) < VOE_LEAST)
		return normal;
	return normalize(mapped);
}

#include "water.slangh"

// The mesh's uv times the record's repeat, 0 read as 1 so a zeroed record is
// unchanged: what every map read in this file starts from.
float2 voe_render_repeated_uv(voe_render_shading shading, float2 uv)
{
	return uv * (shading.uv_repeat != 0.0 ? shading.uv_repeat : 1.0);
}

#include "terrain_layers.slangh"

[shader("fragment")]
float4 voe_render_draw_fragment(voe_render_draw_vertex_out vertex,
				bool front : SV_IsFrontFace) : SV_Target
{
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	voe_render_shading shading = voe_render_shadings[object.shading];
	// The record's factor tinted by the object's colour: every use of the
	// factor below reads this one.
	float4 factor = shading.base_colour * object.colour;
	voe_render_light sun = voe_render_frame.light;
	// The surface about the eye, which every exit hazes by.
	float3 about = vertex.world - voe_render_frame.camera.eye;

	// EVERY SAMPLE HAPPENS BEFORE ANY BRANCH, AND THAT IS DELIBERATE. A
	// Sample needs the derivatives the rasteriser computes across a quad of
	// fragments, which are only defined where the whole quad took the same
	// path. Reading all five here costs one fetch of a one-pixel white
	// texture where a material has no map, and is correct without a rule
	// about where a branch may go.
	//
	// NonUniformResourceIndex is not used and is not needed: the texture
	// index comes from a record picked by a push constant, so it is the same
	// value for every fragment in the draw. That day has come for the other
	// shader and not for this one — elements.slang picks its sheet out of the
	// record the instance index chose, so it wraps its subscript and the
	// device asks for the feature to match. Nothing here changes.
	//
	// A MATERIAL WITH NO TEXTURE SAMPLES WHITE, WHICH IS WHY NOTHING TESTS
	// FOR ONE. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one white
	// pixel, so multiplying by it leaves every factor alone.
	//
	// AND THE BASE COLOUR IS THE ONE MAP READ THROUGH A RECT. `uv * zw + xy`
	// puts a whole sheet of frames behind one geometry — see
	// voe_render_shading_values in render/device.h. The other four maps are
	// read at the repeated coordinates alone: a sheet of frames is what a
	// picture on a surface is not, and a material whose roughness came out
	// of a grid of cells is a card nothing has asked for.
	//
	// THE REPEAT COMES FIRST, ONCE, FOR ALL FIVE (ADR-0399 point 3): every
	// map below and the rect read `uv`, never vertex.uv.
	float2 uv = voe_render_repeated_uv(shading, vertex.uv);
	float2 base_uv = uv * shading.base_colour_uv_rect.zw +
			 shading.base_colour_uv_rect.xy;
	float4 sheet =
		voe_render_textures[shading.base_colour_texture].Sample(base_uv);
	// The level that sample read, which a cut-out scales its coverage by;
	// a query on derivatives too, so it is taken here with the samples.
	float level = voe_render_textures[shading.base_colour_texture]
			      .CalculateLevelOfDetail(base_uv);
	// glTF packs occlusion in red, roughness in green and metalness in blue,
	// and the metallic-roughness map is the green and blue of that. A file
	// that puts its occlusion in the same picture points its occlusion
	// texture at it as well, and then this samples the same slot twice.
	// A named map replaces the roughness and metallic factors, not times
	// them (0401); a non-zero id naming no live texture samples white and so
	// reads fully rough and fully metal.
	float4 orm =
		voe_render_textures[shading.metallic_roughness_texture].Sample(uv);
	float occlusion =
		voe_render_textures[shading.occlusion_texture].Sample(uv).r;
	float3 tilt =
		voe_render_textures[shading.normal_texture].Sample(uv).rgb;
	float3 glow =
		voe_render_textures[shading.emissive_texture].Sample(uv).rgb;
	// The tangent frame's derivatives, taken here for the same reason.
	float3 world_dx = ddx(vertex.world);
	float3 world_dy = ddy(vertex.world);
	float2 uv_dx = ddx(uv);
	float2 uv_dy = ddy(uv);

	// The distance-field reading of that same sample, worked out here whether
	// or not this material wants it. Unconditional because of fwidth: see the
	// header. A material that is an ordinary picture pays three min/max and
	// one derivative for it.
	//
	// A HARD CUT AT THE OUTLINE, BECAUSE WORLD TEXT IS NOT ANTIALIASED.
	// The median is the signed distance and half is the outline, so a step
	// is the whole of the decision: a fragment is inside the letter or it is
	// not. Interface text is covered across a pixel instead (ADR-0269, in
	// elements.slang); world text keeps this cut by choice, not by a limit
	// of the sheet.
	//
	// THE DISTANCE FIELD IS STILL WHAT MAKES THIS WORK, AND THAT IS THE
	// thing to understand before changing it. A hard cut through a field
	// puts the edge exactly where the outline is at any size, so a letter
	// walked up to has clean straight edges and square corners. A hard cut
	// through a stored picture of coverage would put the edge on the nearest
	// texel and a magnified letter would come out in blocks. The sheet is
	// not what antialiased the text; it is what makes not antialiasing it
	// look like a letter.
	//
	// WHAT IT COSTS IS THE FAR END OF THE SCENE. Text small enough that one
	// pixel spans several texels breaks into disconnected specks, and they
	// move as the camera moves. That is accepted here and it is not a bug to
	// fix at this line.
	float field = voe_render_median(sheet.rgb);
	float field_alpha = step(0.5, field);

	// THE THIRD UNIFORM BRANCH IN THIS FILE. See the header before adding a
	// fourth. The sheet's rgb is distances and never multiplies the colour;
	// its alpha channel is not read at all. The factor's own alpha still
	// applies, so a tint may fade text the way it fades anything else.
	float4 base = shading.base_colour_distance_field != 0 ?
			      float4(factor.rgb, factor.a * field_alpha) :
			      factor * sheet;
	// The vertex's colour, before the alpha mode reads `base` (ADR-0421).
	base *= vertex.colour;

	bool mapped = shading.metallic_roughness_texture != VOE_RENDER_NO_TEXTURE;
	float roughness = saturate(mapped ? orm.g : shading.roughness);
	float metallic = saturate(mapped ? orm.b : shading.metallic);
	bool tilted = shading.normal_texture != VOE_RENDER_NO_TEXTURE;

	// A painted landscape takes colour, normal and ORM from its layers
	// (ADR-0402 point 5); alpha and lit stay its own record's.
	if (object.heights != 0 && object.paint_a != 0) {
		voe_render_layer_surface ground =
			voe_render_layer_surface_at(object, vertex.uv, false);
		base.rgb = ground.base_colour * object.colour.rgb;
		tilt = ground.normal_texel;
		tilted = ground.has_normal;
		occlusion = ground.occlusion;
		roughness = saturate(ground.roughness);
		metallic = saturate(ground.metallic);
	}
	float alpha = roughness * roughness;

	// THE ALPHA MODE, RESOLVED HERE AND NOT AT THE RETURN, because there are
	// two returns and because the discard belongs after every Sample above
	// and before everything else. `coverage` and not `alpha`: alpha is
	// roughness squared in this file and has been since card 019.
	//
	// OPAQUE FORCING IT TO ONE IS THE LINE THAT MATTERS. See the header: it
	// is what makes the multiply at the bottom safe for a material whose
	// base colour happens to carry an alpha of a half.
	float coverage = base.a;

	if (shading.alpha_mode == VOE_RENDER_ALPHA_CUTOUT) {
		// A far cut-out stays solid (ADR-0421 point 3). See the header.
		if (shading.base_colour_distance_field == 0)
			coverage *= 1.0 + 0.25 * max(0.0, level);
		if (coverage < shading.alpha_cutoff)
			discard;
		coverage = 1.0;
	} else if (shading.alpha_mode != VOE_RENDER_ALPHA_BLENDED) {
		coverage = 1.0;
	}

	// The object's share of the pixels, 0 whole (ADR-0432 point 5): d keeps
	// a threshold under d, −d one at or above d, so the two cover each pixel
	// once. Here for the reason the cut-out's discard is.
	if (vertex.dither != 0.0) {
		float threshold =
			voe_render_dither_threshold(vertex.position.xy);
		if (vertex.dither > 0.0 ? threshold >= vertex.dither :
					  threshold < -vertex.dither)
			discard;
	}

	// An unlit material, which is text and will be the user interface: the
	// base colour is the answer and no light of any direction changes it.
	// It shares its exit with the one below, and the two mean different
	// things — this one was asked for and that one is a file with no
	// normals in it.
	if (shading.unlit != 0)
		return voe_render_premultiplied(
			voe_render_expose(voe_render_haze(base.rgb, about)),
			coverage);

	// A pass with no sun: every surface unshaded. See the header.
	if (sun.unshaded != 0)
		return voe_render_premultiplied(
			voe_render_expose(voe_render_haze(base.rgb, about)),
			coverage);

	// THE FOURTH UNIFORM BRANCH: water, before the vertex normal is read,
	// because the plane's normal comes from its matrices. See the header.
	if (shading.water != 0)
		return voe_render_water(object, vertex, base.rgb);

	// A back face, reached only by a two-sided draw: its normal turned
	// round, so it is lit from the side seen. See the header.
	if (!front)
		vertex.normal = -vertex.normal;

	// A normal of nothing: unlit rather than NaN. See the header.
	if (dot(vertex.normal, vertex.normal) < VOE_LEAST)
		return voe_render_premultiplied(
			voe_render_expose(voe_render_haze(base.rgb, about)),
			coverage);

	// The geometric normal places the shadow lookup; the mapped one shades.
	float3 surface = normalize(vertex.normal);
	float3 n = surface;
	if (tilted)
		n = voe_render_mapped_normal(surface, tilt, world_dx, world_dy,
					     uv_dx, uv_dy);
	float3 v =normalize(voe_render_frame.camera.eye - vertex.world);

	// A metal has no diffuse response and reflects its own colour; a
	// non-metal reflects four per cent of white and keeps its colour in the
	// diffuse half. Metalness is the interpolation between the two and is
	// not a blend of two shading models.
	float3 f0 = lerp(float3(VOE_DIELECTRIC_REFLECTANCE), base.rgb,
			 metallic);
	float3 diffuse_colour = base.rgb * (1.0 - metallic);

	// Which light blockers hold the surface (ADR-0347), placed by the
	// geometric normal as the shadow lookup is; each directional light's
	// reach is nought where they stop it (ADR-0350 point 3), and its fill
	// reads that.
	uint mask = voe_render_surface_mask(vertex.world, surface);
	float3 fill;
	float3 sun_reaching;
	float3 lamp_reaching;
	// The directional lights, the point lights beside them (ADR-0320), the
	// bounce with the fills as its floor and the sky's reflection on metal
	// beside it (ADR-0412 point 5), as bright as the light on the surface
	// (ADR-0424), the terms occlusion darkens (ADR-0400), and emission
	// last, unshadowed and unoccluded (ADR-0278).
	float3 direct = voe_render_directionals(vertex.world, surface, n, v,
						diffuse_colour, f0, alpha, mask,
						fill, sun_reaching) +
			voe_render_points(vertex.world, surface, n, v,
					  diffuse_colour, f0, alpha, mask,
					  lamp_reaching);
	// The indirect light for a base colour of one, so L can read it; times
	// the base colour it is the same floats as before.
	float3 indirect_light = voe_render_indirect(
		fill, vertex.world, n, float3(1.0, 1.0, 1.0), mask);
	float3 reaching = sun_reaching + lamp_reaching + VOE_PI * indirect_light;
	float3 indirect =
		base.rgb * indirect_light +
		voe_render_sky_reflection(n, v, f0, roughness, metallic, mask) *
			voe_render_reflection_light(reaching);
	float3 lit = direct + indirect * occlusion + shading.emissive * glow;

	return voe_render_premultiplied(
		voe_render_expose(voe_render_haze(lit, about)), coverage);
}

// How far a probe's picture reaches, in its volume's cells: the bounce
// record's spacing times this is the volume's reach (0332 point 4), as
// VOE_RENDER_BOUNCE_REACH / VOE_RENDER_BOUNCE_SPACING in render/device.h.
static const float VOE_RENDER_BOUNCE_REACH_CELLS = 12.0;

// What the capture vertex stage hands on (ADR-0326 point 3): the clip position
// on one probe's face, the world normal, the position about the probe, the uv,
// the vertex's colour, and the layer last, so the fragment's input below
// matches the rest in order.
struct voe_render_capture_out {
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 about : POSITION;
	float2 uv : TEXCOORD;
	float4 colour : COLOR;
	uint layer : SV_RenderTargetArrayIndex;
};

// The same as the fragment stage reads it, without the layer.
struct voe_render_capture_in {
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float3 about : POSITION;
	float2 uv : TEXCOORD;
	float4 colour : COLOR;
};

// The capture scratch's two targets, in the pipeline's order.
struct voe_render_capture_targets {
	float4 albedo : SV_Target0;
	float4 normal : SV_Target1;
};

// THE CAPTURE PASS'S VERTEX STAGE: the point-shadow one with the pass's probes
// as its lights, slot layer / 6 + 1 read at index layer / 6 of this pass's
// region, its face layer % 6, out to the volume's reach.
[shader("vertex")]
voe_render_capture_out
voe_render_capture_vertex(voe_render_draw_vertex_in vertex,
			  uint instance : SV_InstanceID)
{
	voe_render_capture_out out;
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	uint layer = voe_render_point_shadow_layer(instance);
	voe_render_point_light probe = voe_render_point_lights[
		voe_render_frame.region * VOE_RENDER_POINT_LIGHTS + layer / 6];

	if (object.heights != 0)
		vertex = voe_render_terrain(object, vertex);
	float4 world = mul(object.world, float4(vertex.position, 1.0));

	out.about = world.xyz - probe.position;
	out.position = voe_render_point_shadow_clip(
		out.about, layer % 6,
		VOE_RENDER_BOUNCE_REACH_CELLS * voe_render_frame.bounce[0].spacing);
	out.normal = mul(object.normal, float4(vertex.normal, 0.0)).xyz;
	out.uv = vertex.uv;
	out.colour = 1.0 - vertex.colour_complement;
	out.layer = layer;
	return out;
}

// THE CAPTURE PASS'S FRAGMENT: linear albedo, the base factor × texture × object
// colour × vertex colour (a painted landscape's layers' colour × object colour), alpha one; and the world normal,
// nought for a normal of nothing, with the distance from the probe in w.
[shader("fragment")]
voe_render_capture_targets voe_render_capture_fragment(voe_render_capture_in vertex)
{
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	voe_render_shading shading = voe_render_shadings[object.shading];
	float4 factor = shading.base_colour * object.colour;
	float2 base_uv = voe_render_repeated_uv(shading, vertex.uv) *
				 shading.base_colour_uv_rect.zw +
			 shading.base_colour_uv_rect.xy;
	float4 sheet =
		voe_render_textures[shading.base_colour_texture].Sample(base_uv);
	float3 n = dot(vertex.normal, vertex.normal) < VOE_LEAST ?
			   float3(0.0, 0.0, 0.0) :
			   normalize(vertex.normal);
	voe_render_capture_targets result;

	result.albedo = float4((factor * sheet * vertex.colour).rgb, 1.0);
	// A painted landscape's albedo is its layers' blended colour (ADR-0402).
	if (object.heights != 0 && object.paint_a != 0)
		result.albedo.rgb = voe_render_layer_surface_at(object, vertex.uv,
								true)
					    .base_colour *
				    object.colour.rgb;
	result.normal = float4(n, length(vertex.about));
	return result;
}

// What the sky's vertex stage hands on (ADR-0405 point 4): the clip position
// and the world direction through it, not normalized.
struct voe_render_sky_out {
	float4 position : SV_Position;
	float3 direction : TEXCOORD;
};

// THE SKY'S VERTEX STAGE: one triangle over the whole target from the vertex
// index, at the far plane. The view direction through a corner is the clip
// position over the projection's scale terms, looking along −Z, turned into the
// world by the view's rotation transposed; an orthographic projection (its
// [3][3] not nought) looks along the view's −Z everywhere.
[shader("vertex")]
voe_render_sky_out voe_render_sky_vertex(uint index : SV_VertexID)
{
	voe_render_sky_out out;
	float2 corner = float2((index << 1) & 2, index & 2) * 2.0 - 1.0;
	float4x4 projection = voe_render_frame.camera.projection;
	float4x4 view = voe_render_frame.camera.view;
	float3 eye = float3(0.0, 0.0, -1.0);

	if (projection[3][3] == 0.0)
		eye = float3(corner.x / projection[0][0],
			     corner.y / projection[1][1], -1.0);
	out.position = float4(corner, 0.0, 1.0);
	out.direction = eye.x * view[0].xyz + eye.y * view[1].xyz +
			eye.z * view[2].xyz;
	return out;
}

// THE SKY'S FRAGMENT, veiled by the haze it is seen through (ADR-0419 point
// 4), f tone.slangh's voe_render_haze_veil along the direction. With `drawn`:
// the radiance, the backdrop with the pass's shown lights' glows and discs,
// mixed towards the haze's colour by f, then exposed and tone mapped as every
// mesh exit is. Without (the stage runs then only with haze): the clear colour
// mixed towards the exposed and toned haze colour by f, so f 0 is the clear
// colour byte for byte. Opaque: everything later draws over it.
[shader("fragment")]
float4 voe_render_sky_fragment(voe_render_sky_out vertex) : SV_Target
{
	float3 view = vertex.direction;

	if (dot(view, view) < VOE_LEAST)
		view = float3(0.0, 1.0, 0.0);
	view = normalize(view);
	float f = voe_render_haze_veil(view);
	if (voe_render_frame.sky.drawn == 0)
		return voe_render_premultiplied(
			lerp(VOE_RENDER_CLEAR,
			     voe_render_expose(voe_render_haze_colour()), f),
			1.0);
	float3 radiance = voe_render_sky_radiance(view);
	return voe_render_premultiplied(
		voe_render_expose(lerp(radiance, voe_render_haze_colour(), f)),
		1.0);
}
