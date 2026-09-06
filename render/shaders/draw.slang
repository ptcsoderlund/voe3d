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
// NORMAL MAPS AND EMISSION ARE READ FROM NO PICTURE HERE. Both ids are in the
// shading record, both are uploaded, and neither is sampled: a normal map needs
// tangents, which the importer does not produce, and emission without tone
// mapping is a colour that clips. Rule 10 — each arrives with the card that
// needs it.
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
// writes one of these per frame slot.
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
// twenty-eight bytes and the four uints after them fill to a hundred and
// forty-four, so every layout rule a structured buffer might be given puts every
// member in the same place.
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
	float reserved_b;
	uint base_colour_texture;
	uint metallic_roughness_texture;
	uint normal_texture;
	uint occlusion_texture;
	uint emissive_texture;
	uint reserved_c0;
	uint reserved_c1;
	uint reserved_c2;
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

[shader("fragment")]
float4 voe_render_draw_fragment(voe_render_draw_vertex_out vertex) : SV_Target
{
	uint index = voe_render_objects[voe_render_draw_current.object].shading;
	voe_render_shading shading = voe_render_shadings[index];
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
	// value for every fragment in the draw. It would be needed the day the
	// index came from a vertex attribute or from the instance index.
	//
	// A MATERIAL WITH NO TEXTURE SAMPLES WHITE, WHICH IS WHY NOTHING TESTS
	// FOR ONE. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one white
	// pixel, so multiplying by it leaves every factor alone.
	float4 base = shading.base_colour *
		      voe_render_textures[shading.base_colour_texture].Sample(vertex.uv);
	// glTF packs occlusion in red, roughness in green and metalness in blue,
	// and the metallic-roughness map is the green and blue of that. A file
	// that puts its occlusion in the same picture points its occlusion
	// texture at it as well, and then this samples the same slot twice.
	float4 orm =
		voe_render_textures[shading.metallic_roughness_texture].Sample(vertex.uv);
	float occlusion =
		voe_render_textures[shading.occlusion_texture].Sample(vertex.uv).r;

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
