// The quad's vertices and indices. See quad.h for what this is and why there are
// two faces in the same place.
//
// THE TWO FACES ARE THE CUBE'S +Z AND -Z FACES WITH THE DEPTH TAKEN OUT, corner
// for corner and index for index — see cubes.c. Copying the pair that is already
// proven counter-clockwise-from-outside is what keeps a hand-wound face from
// being wound the wrong way and silently culled.
#include "quad.h"
#include "cubes.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <scene/transform_system.h>

// Half a metre, so the quad is one metre across. Units are metres (CLAUDE.md).
#define H 0.5f

const voe_render_vertex voe_dev_quad_vertices[VOE_DEV_QUAD_VERTEX_COUNT] = {
	// +Z
	{ { -H, H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { H, H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { H, -H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	// -Z
	{ { H, H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
	{ { -H, H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { H, -H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
};

const uint32_t voe_dev_quad_indices[VOE_DEV_QUAD_INDEX_COUNT] = {
	3, 2, 1, 3, 1, 0, // +Z
	7, 6, 5, 7, 5, 4, // -Z
};

// The two see-through quads: how far either side of the cubes they stand, how
// big they are, how see-through, and what colour each one is.
//
// THEY ARE ON OPPOSITE SIDES OF THE SCENE BECAUSE THAT IS WHAT MAKES THE SORT
// SOMETHING A PERSON CAN SEE. The camera orbits, so which of the two is nearer
// swaps twice a lap; a sort with its sign the wrong way round is right for half
// the lap and wrong for the other half, and two quads at one depth would not
// show it. The offset in x and y is so that they overlap partly rather than
// exactly — the overlap is where the near one's colour has to be the one on top.
#define QUAD_Z 1.6f
#define QUAD_X 0.8f
#define QUAD_Y 0.4f
#define QUAD_SIZE 2.2f

// Half see-through, which is where a mistake in the blend is most visible: fully
// transparent hides a wrong colour and nearly opaque hides a wrong order.
#define QUAD_ALPHA 0.5f

// The three quads in the overlay: how big they are and how far either side of
// the middle one the other two stand. They are put at the turning cube, so these
// are the numbers that decide whether they are inside it.
//
// SMALL ENOUGH TO FIT INSIDE THE TURNING CUBE ACROSS. That cube is CUBE_SCALE_X
// by CUBE_SCALE_Y by CUBE_SCALE_Z (src/cubes.c) and it spins, so a quad noticeably narrower
// than the smallest of those is enclosed by it from every angle — which is what
// makes "nothing in the world covers it" something a person can watch rather
// than take on trust. They stick out above and below, and that is fine: the
// claim is about the part that is inside.
//
// AND FAR ENOUGH APART TO OVERLAP RATHER THAN COINCIDE. Two quads at one depth
// would show nothing about the order they were drawn in. This is the same reason
// QUAD_Z is not nought, at a smaller scale.
#define OVERLAY_QUAD_SIZE 0.5f
#define OVERLAY_QUAD_Z 0.22f

// The material a quad wears. The three things that differ between the five of
// them are all here; everything else is the same for all of them.
//
// THE COLOUR IS NOT PREMULTIPLIED HERE. A material's base colour is an ordinary
// colour with an alpha beside it; the shader multiplies at the very end, which
// is where the engine's premultiplied contract is applied. See
// render/shaders/draw.slang.
//
// FULLY ROUGH AND NOT METALLIC, so what is seen through a see-through one is the
// blend and not a highlight. A metal has no diffuse response, which would make
// most of the quad nearly black and the blend impossible to judge.
voe_3d_material voe_dev_quad_material(voe_math_float4 colour,
				      voe_render_alpha_mode alpha_mode,
				      bool unlit)
{
	voe_3d_material material = {
		.base_colour = colour,
		.metallic = 0.0f,
		.roughness = 1.0f,
		.alpha_mode = alpha_mode,
		.alpha_cutoff = 0.5f,
		.unlit = unlit,
	};

	return material;
}

// Where one quad stands and how big it is. They are all square and all upright,
// so a position and one number is the whole of a quad's transform.
voe_scene_transform voe_dev_quad_at(voe_math_float3 position, float size)
{
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { size, size, 1.0f },
	};

	return transform;
}

// One panel, one entity: a transform saying where the surface stands and how big
// it is, and a panel component saying how big the surface is in its own
// millimetres and which layer it is drawn in.
//
// NO MATERIAL AND NO MESH, WHICH IS THE SHAPE WORTH NOTICING. An element carries
// its own colour, so there is no shading record to point at and no geometry to
// name — the two rows here are the whole of a drawable surface. The range is
// left at nought and is written every frame by the loop; until the first frame
// writes one, a count of nought draws nothing and is not an error.
//
// What is wrong if it looks wrong:
//
//   - THE PANEL OF RECTANGLES IN THE BOTTOM-RIGHT (card 030). Forty of them, in
//     one draw command, and the console says so once. Three things in it are
//     worth a look: the orange bar at its top is forty millimetres of rectangle
//     clipped to twenty, so half of it is missing on purpose; the row of six
//     bars below it is one colour at six alphas and has to read as a smooth
//     ramp, because a shader that forgot to premultiply leaves the faintest one
//     still obvious and one that did it twice makes the row vanish too early;
//     and the thirty-two squares below that are thirty-two different colours,
//     which is the thing one draw of one shading record could not be.
//   - The exhibit upside down, or the badge's orange corner mark at the bottom
//     — the element surface's Y. It runs down from the surface's top-left
//     corner, and the negation that makes that true is in
//     voe_render_element_surface_matrix. Nothing here negates anything.
//   - THE EXHIBIT VISIBLE THROUGH A CUBE THAT IS IN FRONT OF IT — the panel has
//     stopped being sorted with the see-through meshes and is being drawn after
//     everything, which is the one thing card 032 exists to prevent. The badge
//     is the opposite case and is meant to be visible through everything: it is
//     in the overlay, on the far side of the depth clear.
//   - The exhibit or the badge stretched, or changing size, when the window is
//     dragged narrow — a bug. Both are objects in metres and the window only
//     changes the camera's aspect. The surface that does answer to the window is
//     the plate and ticks in the top-left corner, and what it does is hold fewer
//     millimetres rather than narrower ones: the ticks keep their size and
//     spacing and the far ones fall off the right edge. See src/surface.h.
bool voe_dev_add_panel(voe_ecs_world *world, voe_math_float3 position,
		       float scale, voe_math_float2 millimetres,
		       voe_3d_layer layer, voe_ecs_entity *out)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { scale, scale, scale },
	};

	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_panel_add(world, entity,
			      (voe_3d_panel){ .size = millimetres,
					      .layer = layer }))
		return false;

	*out = entity;
	return true;
}

// One quad, one entity: geometry it shares with every other quad, a material of
// its own because the colour differs, a transform of its own, and the layer it
// is drawn in.
bool voe_dev_add_quad(voe_ecs_world *world, voe_render_device *gpu,
		      voe_render_geometry geometry, voe_3d_material material,
		      voe_scene_transform transform, voe_3d_layer layer,
		      voe_ecs_entity *out, voe_base_error *error)
{
	voe_ecs_entity entity = { 0 };

	if (!voe_3d_material_upload(gpu, &material, error))
		return false;
	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_mesh_add(world, entity,
			     (voe_3d_mesh){ .geometry = geometry,
					    .layer = layer }))
		return false;
	if (!voe_3d_material_add(world, entity, material))
		return false;

	// Every quad but the panel is placed once and never moved again, so the
	// entity is of no use to them. `out` may be NULL for those.
	if (out != NULL)
		*out = entity;
	return true;
}

// The five quads: one square of geometry into the pools, two entities standing
// either side of the cubes in the world, and three more standing inside the
// turning cube in the overlay.
//
// And two see-through squares, one warm and one cool, standing a metre and a
// half either side of the cubes.
//
// Then three smaller squares — one solid purple, one see-through green, one
// see-through orange — standing inside the turning cube and never hidden by it,
// however far round the orbit goes. Those are the overlay.
//
// THE CUBES ARE VISIBLE THROUGH THEM AND TINTED BY THEM. That is the whole claim
// of the blended pass: half of what a quad covers is the quad's colour and half
// is whatever was behind it. A quad that hides what is behind it is a blend that
// is not happening; a quad that has gone dark is the opaque path forcing alpha
// to one where it should not, or a colour premultiplied twice.
//
// AND WHICH OF THE TWO IS ON TOP CHANGES AS THE CAMERA GOES ROUND, WHICH IS THE
// SORT. They stand at +z and −z either side of the cubes, so the camera is
// behind one of them for half its lap and behind the other for the other half,
// and the nearer one's colour has to be the one on top of the overlap every
// time. If it is right from one side and wrong from the other, the sort's sign
// is backwards — that is the failure 3d/tests/depth_sort.c exists to catch
// before it gets here, and this is what it looks like when it does.
//
// A MATERIAL EACH AND NOT ONE BETWEEN THEM, unlike the cubes. They have to be
// different colours or there is no way to tell from the picture which one ended
// up on top, and a material is where a colour lives.
//
// ---- THE TWO IN THE WORLD ----
//
// The warm one nearer +Z and the cool one nearer −Z, so that whichever the
// camera is behind is the one whose colour is on top of the other. They are
// added in this order deliberately: the table order is warm then cool for the
// whole run, so a frame that looks right from one side and wrong from the other
// is the sort and nothing else.
//
// ---- THE THREE IN THE OVERLAY, AND WHAT EACH ONE IS FOR ----
//
// THEY STAND INSIDE THE TURNING CUBE, WHICH IS THE POINT. The squashed cube is
// solid and it is right there around them, so every one of these would be hidden
// for most of a lap if the layer were not working. They keep real positions in
// metres and are seen through the same camera as everything else — this is
// "always on top", not screen space, and there is no orthographic projection
// anywhere in this engine.
//
// AND THEY STILL OCCLUDE EACH OTHER, WHICH IS THE HALF THAT A LAYER MADE OF A
// DISABLED DEPTH TEST WOULD GET WRONG. The two see-through ones stand at +Z and
// −Z of the solid one exactly as the world's pair straddles the cubes, so which
// of them is nearer swaps twice a lap and the nearer one's colour has to be the
// one on top of the overlap — the same diagnostic, one layer up. The solid one
// between them writes depth like any other solid thing, so it hides whichever of
// the two is behind it and is tinted by whichever is in front.
//
// ONE OF THEM IS LIT AND ONE IS NOT, AND THAT IS A SEPARATE AXIS ON PURPOSE. The
// layer decides order and never lighting. The lit pair brighten and darken as
// the sun goes round; the unlit one keeps exactly the colour its material asks
// for. An overlay that quietly stopped lighting things would look like a
// reasonable convenience and it is the one this arrangement is here to catch.
//
// What is wrong if it looks wrong:
//
//   - A quad hiding what is behind it rather than tinting it — the blend state,
//     or the material's mode arriving as opaque.
//   - The overlap of the two quads showing the far one's colour on top, from
//     some camera angles and not others — the sort's sign.
//   - The three quads inside the turning cube showing the far one's colour on
//     top of the near one's — the sort, inside the overlay, which is the same
//     sort and the same sign as the world's pair. It swaps twice a lap, so a
//     backwards sign is right for half of it.
//   - The solid overlay quad not hiding the see-through one behind it — the
//     overlay's solid group is not writing depth, or the depth clear is
//     happening after it rather than before.
//   - The two lit overlay quads not changing as the sun goes round, or the
//     unlit one changing — the layer has picked up a meaning about lighting
//     that it must not have. It decides order and nothing else.
bool voe_dev_add_the_quads(voe_ecs_world *world, voe_render_device *gpu,
			   voe_render_geometry *quad, voe_base_error *error)
{
	voe_render_geometry geometry = { 0 };
	voe_math_float4 warm = { 0.9f, 0.25f, 0.15f, QUAD_ALPHA };
	voe_math_float4 cool = { 0.15f, 0.35f, 0.9f, QUAD_ALPHA };
	voe_math_float4 over_lit = { 0.25f, 0.9f, 0.4f, QUAD_ALPHA };
	voe_math_float4 over_unlit = { 0.95f, 0.55f, 0.1f, QUAD_ALPHA };
	voe_math_float4 over_solid = { 0.55f, 0.2f, 0.75f, 1.0f };
	voe_math_float3 middle = { VOE_DEV_CUBES_APART, 0.0f, 0.0f };

	if (!voe_render_geometry_create(gpu, voe_dev_quad_vertices,
					VOE_DEV_QUAD_VERTEX_COUNT,
					voe_dev_quad_indices,
					VOE_DEV_QUAD_INDEX_COUNT, &geometry,
					error))
		return false;

	// The one square every quad in this program wears, the heads-up panel
	// included. One upload and one id; what differs between them is a
	// material and a transform.
	*quad = geometry;

	if (!voe_dev_add_quad(
		    world, gpu, geometry,
		    voe_dev_quad_material(warm, VOE_RENDER_ALPHA_BLENDED, false),
		    voe_dev_quad_at((voe_math_float3){ QUAD_X, QUAD_Y, QUAD_Z },
				    QUAD_SIZE),
		    VOE_3D_LAYER_WORLD, NULL, error))
		return false;
	if (!voe_dev_add_quad(
		    world, gpu, geometry,
		    voe_dev_quad_material(cool, VOE_RENDER_ALPHA_BLENDED, false),
		    voe_dev_quad_at((voe_math_float3){ QUAD_X, QUAD_Y, -QUAD_Z },
				    QUAD_SIZE),
		    VOE_3D_LAYER_WORLD, NULL, error))
		return false;

	// The solid one first, so that the two see-through ones are not merely
	// being drawn in an order that happens to look right: it writes depth
	// before either of them is issued, and both of them test against it.
	if (!voe_dev_add_quad(
		    world, gpu, geometry,
		    voe_dev_quad_material(over_solid, VOE_RENDER_ALPHA_OPAQUE,
					  false),
		    voe_dev_quad_at(middle, OVERLAY_QUAD_SIZE),
		    VOE_3D_LAYER_OVERLAY, NULL, error))
		return false;
	if (!voe_dev_add_quad(
		    world, gpu, geometry,
		    voe_dev_quad_material(over_lit, VOE_RENDER_ALPHA_BLENDED,
					  false),
		    voe_dev_quad_at((voe_math_float3){ middle.x, middle.y,
						       OVERLAY_QUAD_Z },
				    OVERLAY_QUAD_SIZE),
		    VOE_3D_LAYER_OVERLAY, NULL, error))
		return false;
	return voe_dev_add_quad(
		world, gpu, geometry,
		voe_dev_quad_material(over_unlit, VOE_RENDER_ALPHA_BLENDED, true),
		voe_dev_quad_at((voe_math_float3){ middle.x, middle.y,
						   -OVERLAY_QUAD_Z },
				OVERLAY_QUAD_SIZE),
		VOE_3D_LAYER_OVERLAY, NULL, error);
}
