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
// ONE FILE, TWO ENTRY POINTS, ONE SPIR-V MODULE. slangc compiles this whole file
// to draw.spv with both entry points in it, and device.c names them by the names
// spelled here — see -fvk-use-entrypoint-name in cmake/voe.cmake, which is what
// keeps those two strings true.
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
// ---- THE LIGHTING, AND EVERY DECISION IN IT ----
//
// ONE DIRECTIONAL LIGHT, NO SHADOWS, NO AMBIENT, NO IMAGE-BASED ANYTHING. The
// sun arrives with the camera, once a frame, and every surface is lit from the
// same direction with the same strength. Nothing tests whether something is in
// the way and nothing lights the side facing away, so a surface pointing away
// from the sun is black — which is what a scene with one light and no bounce
// looks like, and not a bug to patch with a fudge factor here.
//
// METALNESS-ROUGHNESS, WHICH IS glTF'S MODEL AND THEREFORE THE IMPORTER'S. The
// base colour, the metalness, the roughness and the occlusion come out of the
// shading record and the three maps; a second shading model would mean
// converting every file this engine can read, so there is one.
//
// THE ARITHMETIC IS THE glTF SPECIFICATION'S BRDF, WRITTEN OUT. A GGX
// distribution, Smith's height-correlated visibility with the specular
// denominator folded into it, and Schlick's Fresnel; the diffuse half is
// Lambert, scaled by what the Fresnel term did not reflect. The one place to
// look when a highlight is wrong is `roughness * roughness`: alpha is roughness
// squared and using roughness directly is the classic mistake — it makes every
// surface too shiny and no maths goes wrong to say so.
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
// NORMAL MAPS AND EMISSION ARE READ FROM NO PICTURE HERE. Both ids are in the
// shading record, both are uploaded, and neither is sampled: a normal map needs
// tangents, which the importer does not produce, and emission without tone
// mapping is a colour that clips. Rule 10 — each arrives with the card that
// needs it.
//
// A DISTANCE-FIELD BASE COLOUR IS THE THIRD UNIFORM BRANCH IN THIS FILE, AND IT
// IS SPENT KNOWINGLY. `unlit` was the second and the alpha mode was the first;
// there is a standing watch to reconsider the whole arrangement at the fourth,
// so the next person to want one reports it rather than adding it.
//
// WHAT THE BRANCH DOES: the sheet's three channels are three signed distances to
// the outline, each measured to a different subset of its edges. Their MEDIAN is
// the distance the shape actually has — which is the whole trick, because at a
// corner two of the three agree and the third is the one that would have rounded
// it off. Alpha is then computed rather than sampled: a step at the halfway mark,
// which is the outline. The colour is the base colour factor, untouched — the
// sheet's rgb is numbers and multiplying by it would be arithmetic on distances.
//
// THE STEP IS HARD BECAUSE THIS ENGINE HAS NO ANTIALIASING ANYWHERE. It was a
// smoothstep across one screen pixel, taken from the median's own derivative,
// until that rule was settled. What the field buys is not a soft edge — it is an
// edge in the right place at any size, which is what keeps a hard cut looking
// like a letter rather than like blocks.
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

// The camera. One per frame, and the same one for every draw in it.
struct voe_render_view {
	float4x4 view;
	float4x4 projection;
	// Where the eye is, in world space: half of the direction a surface is
	// being looked from, which is what a specular highlight needs.
	float3 eye;
	float reserved;
};

// The sun. `direction` is where the light travels — a sun overhead is
// (0, -1, 0) — and it arrives unit length, because whoever owns the light is
// where a direction of nothing is a bug worth hearing about.
struct voe_render_light {
	float3 direction;
	float intensity;
	float3 colour;
	float reserved;
};

// Set 0, binding 0. Matches struct voe_render_frame_block in
// render/src/device_internal.h field for field: descriptors.c builds the layout
// that matches it, asserts on both structs' sizes and offsets, and frame.c
// writes one of these per pass, bound at that pass's dynamic offset.
struct voe_render_frame_block {
	voe_render_view camera;
	voe_render_light light;
};

[[vk::binding(0, 0)]]
ConstantBuffer<voe_render_frame_block> voe_render_frame;

// Set 0, binding 1: every texture at once, indexed by a number out of a shading
// record.
//
// THE LENGTH IS VOE_RENDER_MAX_TEXTURES AND IT IS WRITTEN DOWN IN THREE PLACES —
// device_internal.h, the layout in descriptors.c, and here. A descriptor array
// has a fixed length in all three and they have to agree; there is no way to say
// "as many as the C side thinks" from a shader. Every element is always written,
// slots nothing has claimed pointing at a one-pixel white texture, because
// Vulkan requires the whole array to be valid whether or not this samples it.
[[vk::binding(1, 0)]]
Sampler2D voe_render_textures[64];

// One drawn object. Matches voe_render_object in render/device.h field for
// field, padding included: the two matrices fill the first hundred and
// twenty-eight bytes, the four uints after them fill to a hundred and
// forty-four and the colour ends it at a hundred and sixty, so every layout rule
// a structured buffer might be given puts every member in the same place. The
// colour is linear and multiplies the shading record's base colour factor,
// alpha included.
//
// THE NORMAL MATRIX IS A SECOND MATRIX AND NOT A LUXURY. Under a non-uniform
// scale the world matrix tilts a normal off the surface it belongs to, and what
// that looks like is lighting sliding across a solid object rather than a wrong
// matrix. The CPU hands over the inverse transpose; see voe_3d_normal_matrix.
//
// THE PADDING IS THREE SCALARS AND NOT A uint3, AND THAT IS NOT A STYLE CHOICE.
// A three-component vector has a sixteen-byte alignment in every buffer layout
// rule there is, so a uint3 after a single uint would start at a hundred and
// forty-four rather than a hundred and thirty-two and the record would be a
// different size on this side. Three uints have a four-byte alignment each and
// land where C puts them. The same applies to the shading record below.
struct voe_render_object {
	float4x4 world;
	float4x4 normal;
	uint shading;
	uint reserved0;
	uint reserved1;
	uint reserved2;
	float4 colour;
};

// Set 0, binding 2: this frame's records, one per draw. Read by both stages —
// the vertex stage wants the matrices and the fragment stage wants the shading
// index.
[[vk::binding(2, 0)]]
StructuredBuffer<voe_render_object> voe_render_objects;

// How a surface is shaded. Matches voe_render_shading_values in
// render/device.h, padding included, for the reason above.
struct voe_render_shading {
	float4 base_colour;
	float metallic;
	float roughness;
	// The alpha mode and the cutoff cutout reads, in the eight bytes that
	// used to be reserved_a. A uint and a float and not a float2, for the
	// alignment reason above: a two-component vector would be realigned and
	// two scalars land where C puts them.
	uint alpha_mode;
	float alpha_cutoff;
	float3 emissive;
	// Non-zero: skip the lighting entirely and write the base colour. In the
	// four bytes that used to be reserved_b, for the alignment reason above.
	uint unlit;
	uint base_colour_texture;
	uint metallic_roughness_texture;
	uint normal_texture;
	uint occlusion_texture;
	uint emissive_texture;
	// Non-zero: the base colour texture holds three signed distances per
	// texel rather than a picture. In the first of the three words that used
	// to be reserved_c, for the alignment reason above.
	uint base_colour_distance_field;
	uint reserved_c1;
	uint reserved_c2;
	// Which rectangle of the base colour texture to read: xy is the offset,
	// zw the scale. It lands on a sixteen-byte boundary where C put it, so
	// it is a float4 on both sides and needs no padding in front of it.
	float4 base_colour_uv_rect;
};

// Set 0, binding 3: every shading record, shared by every frame slot because
// nothing rewrites one after it is made.
[[vk::binding(3, 0)]]
StructuredBuffer<voe_render_shading> voe_render_shadings;

// voe_render_alpha_mode in render/include/render/device.h, and these three
// numbers are that enum's three values. Opaque is 0 so a zeroed record is an
// opaque one.
static const uint VOE_RENDER_ALPHA_OPAQUE = 0;
static const uint VOE_RENDER_ALPHA_CUTOUT = 1;
static const uint VOE_RENDER_ALPHA_BLENDED = 2;

// Which object this draw is. Four bytes, and the whole of the push constant.
struct voe_render_draw_push {
	uint object;
};

[[vk::push_constant]]
ConstantBuffer<voe_render_draw_push> voe_render_draw_current;

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
	return out;
}

static const float VOE_PI = 3.14159265358979;

// What a non-metal reflects head-on: four per cent, which is glass, plastic,
// stone, skin and very nearly everything else that is not a metal. A metal's
// answer is its own base colour instead, which is what metalness interpolates
// towards.
static const float VOE_DIELECTRIC_REFLECTANCE = 0.04;

// Nothing here divides by a number that came out of a file without this under
// it. A grazing angle sends the visibility term's denominator to zero and one
// NaN fragment is a black or white speck that moves with the camera.
static const float VOE_LEAST = 1e-7;

// Schlick's approximation: how much more a surface reflects at a grazing angle
// than head-on. The fifth power is the approximation and not a tunable.
float3 voe_render_fresnel(float3 f0, float cosine)
{
	float factor = pow(saturate(1.0 - cosine), 5.0);

	return f0 + (1.0 - f0) * factor;
}

// Trowbridge-Reitz, which everyone calls GGX: how much of the surface's
// microscopic facets point at the half vector. `alpha` is roughness squared —
// see the note at the top of this file.
float voe_render_distribution(float n_dot_h, float alpha)
{
	float a2 = alpha * alpha;
	float d = n_dot_h * n_dot_h * (a2 - 1.0) + 1.0;

	return a2 / max(VOE_PI * d * d, VOE_LEAST);
}

// Smith's height-correlated visibility, which is the shadowing-masking term with
// the specular denominator 4·(n·l)·(n·v) folded into it. Folded because the two
// cancel: written separately they are a division by something that goes to zero
// at exactly the angles where the other factor does.
float voe_render_visibility(float n_dot_l, float n_dot_v, float alpha)
{
	float a2 = alpha * alpha;
	float lambda_l = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0 - a2) + a2);
	float lambda_v = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0 - a2) + a2);

	return 0.5 / max(lambda_l + lambda_v, VOE_LEAST);
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
	// path. Reading all three here costs one fetch of a one-pixel white
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

	// The distance-field reading of that same sample, worked out here whether
	// or not this material wants it. Unconditional because of fwidth: see the
	// header. A material that is an ordinary picture pays three min/max and
	// one derivative for it.
	//
	// A HARD CUT AT THE OUTLINE, BECAUSE THIS ENGINE HAS NO ANTIALIASING.
	// The median is the signed distance and half is the outline, so a step
	// is the whole of the decision: a fragment is inside the letter or it is
	// not. There is no derivative here and no smoothstep, and that is the
	// engine's rule rather than a limit of the sheet.
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

	// A normal of nothing: unlit rather than NaN. See the header.
	if (dot(vertex.normal, vertex.normal) < VOE_LEAST)
		return voe_render_premultiplied(base.rgb, coverage);

	float3 n = normalize(vertex.normal);
	float3 v = normalize(voe_render_frame.camera.eye - vertex.world);
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

	float3 radiance = sun.colour * sun.intensity * n_dot_l;
	float3 lit = (diffuse + specular) * radiance * occlusion;

	return voe_render_premultiplied(lit, coverage);
}
