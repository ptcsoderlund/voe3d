// The engine's two pipelines: a vertex out of the shared pool, placed by three matrices —
// two of them the camera's, shared by every draw in a frame, and one of them the
// object's, read out of a buffer by the object's own number — lit by one
// directional light and shaded by a record read the same way. It replaced
// cube.slang, which had the cube's two entry points in it and a per-object
// matrix in a push constant.
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
// ONE FILE, THREE ENTRY POINTS, ONE SPIR-V MODULE. slangc compiles this whole file
// to draw.spv with all of them in it (the third the bounce map's fragment), and device.c names them by the names
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
// THREE PARTS ARE INCLUDED, IN THE ORDER THEY DEPEND ON EACH OTHER (ADR-0308).
// bindings.slangh first: the frame block, the records and the bindings both
// stages read. lighting.slangh next: the sun, its cascades, the fill and the
// BRDF terms, with the decisions behind them in its header. water.slangh last,
// after the vertex stage and voe_render_premultiplied, which it uses. A part is
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
// OCCLUSION MULTIPLIES THE LIGHT, WHICH IS NOT WHERE glTF PUTS IT. In the
// specification an occlusion map attenuates *indirect* light, and this engine
// has none — so a map read that way would multiply nothing and the channel would
// be uploaded and thrown away. It is applied to the direct result instead, which
// is the only reading that makes an ORM map mean something today. The card that
// brings ambient or image-based light is the card that moves it, and then this
// paragraph is what says what it is moving.
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
// orthogonalised against the interpolated normal, and each channel maps 0..1
// to −1..1. The interpolated normal still places the shadow lookup. Seams
// where a frame flips across a UV island are the cost; a tangent attribute is
// the card that lifts it.
//
// EMISSION IS ADDED LAST: the record's emissive times the emissive map, after
// the sun and the fill, unshadowed and unoccluded. Only on the lit path; an
// unlit exit is its base colour. Untone-mapped, so a strong one clips.
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
//              is fully solid and writes depth like anything else.
//   blended  — alpha exactly as computed, and the pass it is drawn in blends.
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
// NOTHING IS TONE MAPPED, SO A BRIGHT ENOUGH LIGHT CLIPS TO WHITE. Expected, and
// on the later list; see render/include/render/device.h.

#include "bindings.slangh"
#include "lighting.slangh"

// The locations are stated rather than left to declaration order, because the C
// side names the same three numbers in its VkVertexInputAttributeDescription and
// two lists that have to agree should not agree by counting.
struct voe_render_draw_vertex_in {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float2 uv : TEXCOORD;
};

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
};

[shader("vertex")]
voe_render_draw_vertex_out
voe_render_draw_vertex(voe_render_draw_vertex_in vertex)
{
	voe_render_draw_vertex_out out;
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	float4 world;
	float4 eye;

	world = mul(object.world, float4(vertex.position, 1.0));
	eye = mul(voe_render_frame.camera.view, world);
	out.position = mul(voe_render_frame.camera.projection, eye);

	// A direction and not a point, so w is zero and the normal matrix's
	// last column — whatever the inverse transpose left there — is ignored.
	// Not normalized here: it is interpolated across the triangle and the
	// fragment stage has to normalize what arrives anyway.
	out.world = world.xyz;
	out.normal = mul(object.normal, float4(vertex.normal, 0.0)).xyz;
	out.uv = vertex.uv;
	out.plane = vertex.position.xy;
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

// Slot 0, the white pixel: voe_render_texture in render/device.h.
static const uint VOE_RENDER_NO_TEXTURE = 0;

// The shading normal a normal map gives `normal` (ADR-0278 point 3). dP/du and
// dP/dv come from solving the screen derivatives of the world position against
// those of the UV; the determinant's sign is the frame's handedness, and it
// multiplies rather than divides because only the direction is kept. Each axis
// is orthogonalised against the interpolated normal. glTF's green runs along
// −v, because its v runs down. A UV with no area on screen, or an axis along
// the normal, has no frame, and the interpolated normal is the answer.
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

	float3 m = texel * 2.0 - 1.0;
	float3 mapped = normalize(along_u) * m.x - normalize(along_v) * m.y +
			normal * m.z;
	if (dot(mapped, mapped) < VOE_LEAST)
		return normal;
	return normalize(mapped);
}

#include "water.slangh"

[shader("fragment")]
float4 voe_render_draw_fragment(voe_render_draw_vertex_out vertex) : SV_Target
{
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	voe_render_shading shading = voe_render_shadings[object.shading];
	// The record's factor tinted by the object's colour: every use of the
	// factor below reads this one.
	float4 factor = shading.base_colour * object.colour;
	voe_render_light sun = voe_render_frame.light;

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
	// read at the vertex's own coordinates: a sheet of frames is what a
	// picture on a surface is not, and a material whose roughness came out
	// of a grid of cells is a card nothing has asked for.
	float2 base_uv = vertex.uv * shading.base_colour_uv_rect.zw +
			 shading.base_colour_uv_rect.xy;
	float4 sheet =
		voe_render_textures[shading.base_colour_texture].Sample(base_uv);
	// glTF packs occlusion in red, roughness in green and metalness in blue,
	// and the metallic-roughness map is the green and blue of that. A file
	// that puts its occlusion in the same picture points its occlusion
	// texture at it as well, and then this samples the same slot twice.
	float4 orm =
		voe_render_textures[shading.metallic_roughness_texture].Sample(vertex.uv);
	float occlusion =
		voe_render_textures[shading.occlusion_texture].Sample(vertex.uv).r;
	float3 tilt =
		voe_render_textures[shading.normal_texture].Sample(vertex.uv).rgb;
	float3 glow =
		voe_render_textures[shading.emissive_texture].Sample(vertex.uv).rgb;
	// The tangent frame's derivatives, taken here for the same reason.
	float3 world_dx = ddx(vertex.world);
	float3 world_dy = ddy(vertex.world);
	float2 uv_dx = ddx(vertex.uv);
	float2 uv_dy = ddy(vertex.uv);

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

	float roughness = saturate(shading.roughness * orm.g);
	float metallic = saturate(shading.metallic * orm.b);
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
		if (coverage < shading.alpha_cutoff)
			discard;
		coverage = 1.0;
	} else if (shading.alpha_mode != VOE_RENDER_ALPHA_BLENDED) {
		coverage = 1.0;
	}

	// An unlit material, which is text and will be the user interface: the
	// base colour is the answer and no light of any direction changes it.
	// It shares its exit with the one below, and the two mean different
	// things — this one was asked for and that one is a file with no
	// normals in it.
	if (shading.unlit != 0)
		return voe_render_premultiplied(base.rgb, coverage);

	// A pass with no sun: every surface unshaded. See the header.
	if (sun.unshaded != 0)
		return voe_render_premultiplied(base.rgb, coverage);

	// THE FOURTH UNIFORM BRANCH: water, before the vertex normal is read,
	// because the plane's normal comes from its matrices. See the header.
	if (shading.water != 0)
		return voe_render_water(object, vertex, base.rgb);

	// A normal of nothing: unlit rather than NaN. See the header.
	if (dot(vertex.normal, vertex.normal) < VOE_LEAST)
		return voe_render_premultiplied(base.rgb, coverage);

	// The geometric normal places the shadow lookup; the mapped one shades.
	float3 surface = normalize(vertex.normal);
	float3 n = surface;
	if (shading.normal_texture != VOE_RENDER_NO_TEXTURE)
		n = voe_render_mapped_normal(surface, tilt, world_dx, world_dy,
					     uv_dx, uv_dy);
	float3 v =normalize(voe_render_frame.camera.eye - vertex.world);
	// Towards the light, which is the opposite of the way the light travels.
	float3 l = -sun.direction;
	float3 h = normalize(l + v);

	float n_dot_l = saturate(dot(n, l));
	float n_dot_v = saturate(dot(n, v));
	float n_dot_h = saturate(dot(n, h));
	float v_dot_h = saturate(dot(v, h));

	// A metal has no diffuse response and reflects its own colour; a
	// non-metal reflects four per cent of white and keeps its colour in the
	// diffuse half. Metalness is the interpolation between the two and is
	// not a blend of two shading models.
	float3 f0 = lerp(float3(VOE_DIELECTRIC_REFLECTANCE), base.rgb,
			 metallic);
	float3 diffuse_colour = base.rgb * (1.0 - metallic);

	float3 fresnel = voe_render_fresnel(f0, v_dot_h);
	float3 specular = fresnel * voe_render_distribution(n_dot_h, alpha) *
			  voe_render_visibility(n_dot_l, n_dot_v, alpha);
	// What the Fresnel term did not reflect is what the diffuse half is left
	// to work with, which is the whole of what keeps this from adding energy
	// the light did not have.
	float3 diffuse = (1.0 - fresnel) * diffuse_colour / VOE_PI;

	float reach = n_dot_l * voe_render_sunlight(vertex.world, surface);
	float3 radiance = voe_render_sun_radiance(reach);
	// The bounce with the fill as its floor, and emission last, unshadowed
	// and unoccluded (ADR-0278).
	float3 lit = (diffuse + specular) * radiance * occlusion +
		     voe_render_indirect(reach, vertex.world, n, base.rgb) +
		     shading.emissive * glow;

	return voe_render_premultiplied(lit, coverage);
}

// The bounce map's two targets (ADR-0308 point 1), in the pipeline's order.
struct voe_render_bounce_out {
	float4 flux : SV_Target0;
	float4 normal : SV_Target1;
};

// THE BOUNCE PASS'S FRAGMENT: what light leaves this caster towards the sun's
// view, before any N·L — the reflector's own facing is the gather's to weigh.
// Flux is the sun's colour times its intensity times the base colour (factor,
// sheet and object colour), alpha one; normal is the world normal in xyz, nought
// for a normal of nothing. The pass's `light` is the sun the caller handed in.
[shader("fragment")]
voe_render_bounce_out voe_render_draw_bounce(voe_render_draw_vertex_out vertex)
{
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	voe_render_shading shading = voe_render_shadings[object.shading];
	float4 factor = shading.base_colour * object.colour;
	float2 base_uv = vertex.uv * shading.base_colour_uv_rect.zw +
			 shading.base_colour_uv_rect.xy;
	float4 sheet =
		voe_render_textures[shading.base_colour_texture].Sample(base_uv);
	voe_render_light sun = voe_render_frame.light;
	float3 n = dot(vertex.normal, vertex.normal) < VOE_LEAST ?
			   float3(0.0, 0.0, 0.0) :
			   normalize(vertex.normal);
	voe_render_bounce_out result;

	result.flux = float4(sun.colour * sun.intensity * (factor * sheet).rgb, 1.0);
	result.normal = float4(n, 1.0);
	return result;
}
