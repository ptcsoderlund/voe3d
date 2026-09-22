// The cube's vertices and indices, and the two cubes built from them: the
// pictures they wear, their materials and their entities. See cubes.h for what
// this is and why it is twenty-four vertices.
//
// THE FIRST VERTEX OF EACH FACE IS ITS TOP-LEFT CORNER and +u runs to that
// face's right, because (0,0) is the top-left of a picture in Vulkan and in
// glTF — see assets/include/assets/image.h for why nothing turns an image over
// on the way in.
#include "cubes.h"
#include "shrink.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <assets/image.h>
#include <scene/transform_system.h>

// Half a metre, so the cube is one metre across. Units are metres (CLAUDE.md).
#define H 0.5f

const voe_render_vertex voe_dev_cube_vertices[VOE_DEV_CUBE_VERTEX_COUNT] = {
	// +Z
	{ { -H, H, H }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { H, H, H }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { H, -H, H }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, H }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	// -Z
	{ { H, H, -H }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
	{ { -H, H, -H }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, -H }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { H, -H, -H }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
	// +X
	{ { H, H, H }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, H, -H }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, -H, -H }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { H, -H, H }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	// -X
	{ { -H, H, -H }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { -H, H, H }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, H }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, -H }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	// +Y
	{ { -H, H, -H }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, H, -H }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, H, H }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, H, H }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
	// -Y
	{ { -H, -H, H }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, -H, H }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, -H, -H }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, -H }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
};

// Two triangles a face, and the order is what makes each one counter-clockwise
// from outside. Thirty-two bits, which is what render's index pool holds.
const uint32_t voe_dev_cube_indices[VOE_DEV_CUBE_INDEX_COUNT] = {
	3, 2, 1, 3, 1, 0, // +Z
	7, 6, 5, 7, 5, 4, // -Z
	11, 10, 9, 11, 9, 8, // +X
	15, 14, 13, 15, 13, 12, // -X
	19, 18, 17, 19, 17, 16, // +Y
	23, 22, 21, 23, 21, 20, // -Y
};

// The turning cube's scale, and the three numbers are different on purpose: a
// non-uniform scale is the only case where a normal matrix and a world matrix
// disagree, so this is what makes that difference something a person can look
// at.
//
// THE TURNING CUBE IS SQUASHED, AND THAT IS THE NORMAL MATRIX ON SCREEN. Its
// scale is not the same on all three axes (CUBE_SCALE_*), which is the one case
// where transforming a normal by the world matrix is visibly wrong: the shading
// would slide across the faces as it turned instead of staying stuck to them.
// 3d/tests/normal_matrix.c is the automated half; this is the half a person can
// see.
#define CUBE_SCALE_X 1.4f
#define CUBE_SCALE_Y 0.6f
#define CUBE_SCALE_Z 1.0f

// The two pictures on the placeholder cubes, embedded at build time. One each,
// which is a change from the "F" both of them used to share.
//
// THEY ARE WRITING, WHICH IS WHAT THE "F" WAS FOR AND IS WHY SWAPPING THEM COSTS
// NOTHING. A checker board looks right upside down and looks right mirrored, and
// both are mistakes this engine can make; a word does not. VOE3D read backwards
// is obvious across the room, and the icon's yellow "3D" badge sits in ONE
// corner, so which corner is which is still a thing the picture says rather than
// a thing to take on trust. What went with the old texture is only the four
// coloured corner blocks, and the writing says the same thing.
//
// THE WIDE ONE GOES ON THE WIDE CUBE AND THE SQUARE ONE ON THE CUBE THAT IS NOT
// SQUASHED, which is worth doing rather than pretty. The logo is 4800 by 2000
// and the turning cube's front face is CUBE_SCALE_X by CUBE_SCALE_Y, which is
// nearly the same ratio — so the wordmark reads almost undistorted on the two
// faces that matter, and is visibly squeezed on the four that are a different
// shape. That is the texture coordinates doing exactly what they should, and a
// picture that came out square on a face that is not would be the bug.
static const uint8_t LOGO_PNG[] = {
#embed "logo.png"
};

static const uint8_t APP_ICON_PNG[] = {
#embed "app icon light.png"
};

// One cube, one entity: geometry it shares with its neighbour, a material it
// shares with its neighbour, a transform of its own, and the world layer.
//
// IT NAMES ITS LAYER RATHER THAN LETTING A ZEROED STRUCT PICK ONE. World is
// nought, so this line changes nothing and is here because every drawable in
// this file says which layer it is in — see src/text.c on the two placements and
// why neither is the normal case.
static bool add_cube(voe_ecs_world *world, voe_render_geometry geometry,
		     voe_3d_material material, voe_math_float3 position,
		     voe_math_float3 scale, voe_ecs_entity *out)
{
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = scale,
	};

	if (!voe_ecs_entity_create(world, out))
		return false;
	if (!voe_scene_transform_add(world, *out, transform))
		return false;
	if (!voe_3d_mesh_add(world, *out,
			     (voe_3d_mesh){ .geometry = geometry,
					    .layer = VOE_3D_LAYER_WORLD }))
		return false;
	return voe_3d_material_add(world, *out, material);
}

// One embedded picture into a texture slot, and the shading record that wears
// it. Both cubes want the same material but for the picture, so the material is
// built here once and the caller says which bytes.
//
// THE PICTURE ARRIVES THE WAY THE SHADERS DO: `#embed`ded at build time, which
// is what keeps "nothing is read from disk at run time" true. The decoded pixels
// are scratch — `render` has taken its own copy by the time the upload returns —
// so the arena goes back on every path out, the failing ones included.
//
// What is wrong if it looks wrong:
//
//   - Everything pale and washed out, or muddy and too dark — a colour space.
//     One of the two sRGB halves (the texture format and the target format) is
//     doing its job without the other; see render/src/texture.c.
static bool textured_material(voe_render_device *gpu, voe_base_arena *arena,
			      const uint8_t *png, size_t size,
			      voe_3d_material *out, voe_base_error *error)
{
	voe_render_texture texture = { 0 };
	voe_assets_image picture;
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	bool ok;

	*out = (voe_3d_material){
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 0.8f,
	};

	ok = voe_assets_png_decode(png, size, arena, &picture, error);
	// AND THEN HALVED UNTIL IT IS A SENSIBLE SIZE, WHICH IS dev's OWN DOING
	// AND NOT THE ENGINE'S. The two pictures here are far bigger than any
	// face they land on, and this engine samples one level with no mip
	// chain, so the level the file happened to ship is the level that gets
	// sampled. src/shrink.h is the whole argument, including why this is
	// not mipmapping and what it does not fix.
	if (ok)
		voe_dev_image_shrink(&picture, VOE_DEV_SHRINK_LONG_SIDE);
	// A colour, so it goes up in the sRGB format and the hardware decodes it
	// before the shading multiplies by it. An ORM map would be the other kind
	// — see voe_render_texture_kind.
	if (ok)
		ok = voe_render_texture_create(gpu, VOE_RENDER_TEXTURE_COLOUR,
					       VOE_RENDER_SAMPLING_SMOOTH,
					       picture.width, picture.height,
					       picture.pixels, &texture, error);
	voe_base_arena_rewind(arena, mark);
	if (!ok)
		return false;

	out->base_colour_texture = texture;
	return voe_3d_material_upload(gpu, out, error);
}

// The two placeholder cubes: their geometry into the pools, a picture each into
// a texture slot, a shading record each, and two entities.
//
// A flat blue-green background with three lit things in it, from left to right:
// these two cubes and the figure src/model.c reads in.
//
//   - A cube standing still at the origin, which is where the camera looks.
//   - A squashed cube turning on a tilted axis, just to its right.
//
// The two placeholder cubes wear WRITING — the still one the app icon, the
// turning one the wordmark — so every face says which way up and which way
// round it is, and a mirrored or upside-down face is unreadable rather than
// merely wrong.
//
// What is wrong if it looks wrong:
//
//   - Nothing on screen, or a cube inside out — the depth test or the winding.
//     render/tests/offscreen.c is the automated form of that one.
//   - An opaque surface gone dark — the alpha mode, and it looks like a lighting
//     regression rather than an alpha one. See render/shaders/draw.slang.
//   - The picture upside down — the one Y flip went the wrong way or happened
//     twice. Every word on every cube is upright when it is right, and the app
//     icon's yellow "3D" badge is in its BOTTOM-RIGHT corner.
//   - WRITING THAT READS BACKWARDS — a mirror, and this is the failure worth
//     staring at. A model can come out mirrored from a transposed rotation or a
//     coordinate conversion nobody should have added, and a mirrored cube looks
//     completely normal until you try to read it. 3d/tests/import.c is the
//     automated form.
//   - The still cube not still, or not centred — the model matrix or the
//     look-at. scene/tests/transform.c and scene/tests/camera.c check both on
//     the CPU, so this should have failed before it got here.
//   - THE WORDMARK SQUEEZED ON THE TURNING CUBE'S NARROW FACES IS CORRECT, and
//     so is it reading almost undistorted on the two wide ones: the picture is
//     4800 by 2000 and those faces are CUBE_SCALE_X by CUBE_SCALE_Y, which is
//     nearly the same shape. A wordmark that looked the same on all six faces of
//     a cube that is not a cube would be the bug.
//   - The shading sliding across the squashed cube as it turns rather than
//     staying on its faces — the normal matrix, and the one thing that cube is
//     there to show.
bool voe_dev_add_the_cubes(voe_ecs_world *world, voe_render_device *gpu,
			   voe_base_arena *arena, voe_ecs_entity *turning,
			   voe_base_error *error)
{
	voe_render_geometry geometry = { 0 };
	voe_3d_material icon;
	voe_3d_material logo;
	voe_ecs_entity still = { 0 };

	if (!voe_render_geometry_create(gpu, voe_dev_cube_vertices,
					VOE_DEV_CUBE_VERTEX_COUNT,
					voe_dev_cube_indices,
					VOE_DEV_CUBE_INDEX_COUNT, &geometry,
					error))
		return false;

	if (!textured_material(gpu, arena, APP_ICON_PNG, sizeof APP_ICON_PNG,
			       &icon, error))
		return false;
	if (!textured_material(gpu, arena, LOGO_PNG, sizeof LOGO_PNG, &logo,
			       error))
		return false;

	// ONE GEOMETRY, TWO SHADING RECORDS, TWO ENTITIES, AND THE PAIR IS WORTH
	// MORE THAN THE ONE RECORD IT REPLACED. Both cubes are the same
	// twenty-four vertices; what differs is a texture id in a shading
	// record, so this is also the smallest demonstration in the program
	// that one mesh can be worn two ways. The second one is squashed, which
	// is what makes the normal matrix visible — see CUBE_SCALE_X.
	return add_cube(world, geometry, icon,
			(voe_math_float3){ 0.0f, 0.0f, 0.0f },
			(voe_math_float3){ 1.0f, 1.0f, 1.0f }, &still) &&
	       add_cube(world, geometry, logo,
			(voe_math_float3){ VOE_DEV_CUBES_APART, 0.0f, 0.0f },
			(voe_math_float3){ CUBE_SCALE_X, CUBE_SCALE_Y,
					   CUBE_SCALE_Z },
			turning);
}
