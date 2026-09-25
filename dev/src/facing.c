// The transforms that turn an exhibit to the camera: where the heads-up line,
// its panel and the readout stand this frame. See facing.h for who calls them
// and when.
//
// ALL THREE ARE BUILT FROM ONE struct view_basis, the eye's own frame read out
// of the pose they are handed, so nothing here keeps state between frames.
#include "facing.h"

#include <base/assert.h>
#include <math/double3.h>
#include <math/float4x4.h>
#include <math/quat.h>

#include <math.h>

// Where the heads-up line stands: how far in front of the eye, and how far below
// the middle of the view. It is placed by a transform intent every frame, from
// where the camera actually is, which is the whole of what "locked to the
// camera" means here — there is no screen-space path and there is not going to
// be one. The readout stands at the same distance.
#define HUD_DISTANCE 1.0f
#define HUD_DROP 0.36f

// How far past the line's own box the panel reaches, in ems of the line. Enough
// to read as a plate the writing sits on rather than as a box cropping it.
#define HUD_PANEL_MARGIN 0.5f

// How much further from the eye the panel is than the line. Small, because the
// two have to stay square to each other, and any positive number at all is
// enough for the depth test — this is not a sorting nudge, it is the whole
// distance between two planes that are parallel.
#define HUD_PANEL_BEHIND 0.01f

// How far the readout sits in from the view's top and left edges, in ems of its
// own size. Its first baseline sits one em below that, so the tallest glyph
// clears the edge.
#define READOUT_MARGIN_EMS 0.5f

// The camera's frame, for placing things that travel with it: where the eye is,
// which way it looks, which way is right and up across the view, and the
// rotation that stands a quad square to it. The three things locked to the
// camera below are all placed from one of these.
//
// THE DIRECTIONS ARE THE POSE MATRIX'S COLUMNS: +X is right, +Y is up and -Z is
// where the eye looks, as the view built from the same pose has it. The rotation
// is the pose's own, so a quad stood with it is square to the view whatever the
// pose is, roll included.
struct view_basis {
	voe_math_double3 eye;
	voe_math_float3 forward;
	voe_math_float3 right;
	voe_math_float3 up;
	voe_math_quat rotation;
};

static struct view_basis basis_of(voe_scene_transform eye)
{
	const voe_math_float4x4 m = voe_scene_transform_matrix(eye, eye.position);
	struct view_basis basis = {
		.eye = eye.position,
		.forward = { -m.m[0][2], -m.m[1][2], -m.m[2][2] },
		.right = { m.m[0][0], m.m[1][0], m.m[2][0] },
		.up = { m.m[0][1], m.m[1][1], m.m[2][1] },
		.rotation = eye.rotation,
	};

	VOE_BASE_ASSERT(voe_math_float3_length(basis.forward) > 0.0f,
			"facing an eye scaled to nothing");
	VOE_BASE_ASSERT(voe_math_float3_length(basis.right) > 0.0f,
			"facing an eye scaled to nothing");
	return basis;
}

// A transform standing square to the camera `offset` metres from the eye, with
// the scale given: the eye's double position plus the float offset (ADR-0250).
static voe_scene_transform_intent square_to_the_camera(
	const struct view_basis *basis, voe_ecs_entity entity,
	voe_math_float3 offset, voe_math_float3 scale)
{
	return (voe_scene_transform_intent){
		.entity = entity,
		.transform = {
			.position = voe_math_double3_add(
				basis->eye, voe_math_double3_from_float3(offset)),
			.rotation = basis->rotation,
			.scale = scale,
		},
	};
}

// Where the heads-up line goes this frame: in front of the eye, square to it,
// centred across it and a little below the middle.
//
// IT IS AN ORDINARY TRANSFORM IN THE WORLD AND THAT IS THE POINT. There is no
// screen-space path in this engine and a "just for debug" one is exactly what
// that rule exists to prevent, so a heads-up display is a quad standing in front
// of the camera and moved with it.
//
// WHAT STOPS A CUBE GETTING IN FRONT OF IT IS THE LAYER AND NOT THIS FUNCTION.
// Standing a metre from the eye used to mean flying into anything put that thing
// in front of the writing; the line is in the overlay now, so it is drawn after
// the world's depth is thrown away. This function still only decides where the
// line is, and it would put it in exactly the same place if it were in the world
// — those are two separate answers to two separate questions and neither one
// implies the other.
voe_scene_transform_intent voe_dev_facing_the_camera(voe_scene_transform eye,
						     voe_ecs_entity text,
						     voe_math_float2 size)
{
	struct view_basis basis = basis_of(eye);
	voe_math_float3 offset =
		voe_math_float3_scale(basis.forward, HUD_DISTANCE);

	offset = voe_math_float3_add(
		offset, voe_math_float3_scale(basis.right, -size.x * 0.5f));
	offset = voe_math_float3_add(offset, voe_math_float3_scale(basis.up, -HUD_DROP));

	return square_to_the_camera(&basis, text, offset,
				    (voe_math_float3){ 1.0f, 1.0f, 1.0f });
}

// Where the readout goes this frame: square to the camera like the line, in the
// top-left corner of the view, a margin in from both edges.
//
// THE CORNER IS WORKED OUT FROM THE FIELD OF VIEW AND THE ASPECT RATIO. At a
// metre in front of the eye the view reaches tan(fov/2) metres up and that times
// the aspect ratio to the side, so the corner is a point on the same plane the
// line stands on and the readout is pinned to the edge of the window, whatever
// size the window is. This is still an ordinary transform in the world: a
// resize moves the corner and the next frame's intent follows it.
//
// IT IS LEFT-ALIGNED AND NOT CENTRED, BECAUSE ITS WIDTH CHANGES. A block's
// origin is the left end of its first baseline, so holding that point still
// holds the left edge still while the digits change; centring on the width, as
// the line does, would shift the whole readout sideways every time a number
// gained a digit.
//
// AND THAT IS WHAT LETS IT BE PLACED HERE AT ALL. The block is built inside the
// frame, after the transform system has run, so a placement that wanted its size
// would be a frame late. This one asks only where the camera is and how big the
// window is.
voe_scene_transform_intent voe_dev_top_left_of_the_view(
	voe_scene_transform eye, voe_scene_camera lens, voe_ecs_entity readout,
	voe_platform_size size, float em)
{
	struct view_basis basis = basis_of(eye);
	// A window with no area has no corner; one is as good as any other
	// then, because nothing is about to be drawn.
	float aspect = size.width > 0 && size.height > 0 ?
			       (float)size.width / (float)size.height :
			       1.0f;
	// How far the view reaches above its centre at the line's distance:
	// the tangent of half the vertical field of view, per metre.
	float half_height = tanf(lens.fov_y * 0.5f) * HUD_DISTANCE;
	float half_width = half_height * aspect;
	float margin = em * READOUT_MARGIN_EMS;
	voe_math_float3 offset =
		voe_math_float3_scale(basis.forward, HUD_DISTANCE);

	offset = voe_math_float3_add(
		offset, voe_math_float3_scale(basis.right, -(half_width - margin)));
	offset = voe_math_float3_add(
		offset, voe_math_float3_scale(basis.up, half_height - margin -
							    em));

	return square_to_the_camera(&basis, readout, offset,
				    (voe_math_float3){ 1.0f, 1.0f, 1.0f });
}

// Where the panel behind the heads-up line goes this frame: the same plane as the
// line, a shade further from the eye, centred on the line's own box and a margin
// larger than it.
//
// IT SHARES THE LINE'S ROTATION AND NOT ITS POSITION. The line's origin is the
// left end of its first baseline, so a panel placed there would hang off to one
// side and sit too low; it is moved right by half the width and up by a quarter
// of the height, which puts it around the band the glyphs actually occupy rather
// than around the line box. A quarter and not a half because a line box is
// mostly above its baseline.
//
// THE SCALE IS NOT UNIFORM, WHICH IS WHY THIS DOES NOT USE voe_dev_quad_at (src/quad.c). A line of
// writing is wide and short and the quad it sits on has to be the same shape.
voe_scene_transform_intent voe_dev_behind_the_line(voe_scene_transform eye,
						   voe_ecs_entity quad,
						   voe_math_float2 size)
{
	struct view_basis basis = basis_of(eye);
	float margin = size.y * HUD_PANEL_MARGIN;
	voe_math_float3 offset = voe_math_float3_scale(
		basis.forward, HUD_DISTANCE + HUD_PANEL_BEHIND);

	offset = voe_math_float3_add(offset, voe_math_float3_scale(basis.up, -HUD_DROP));
	offset = voe_math_float3_add(offset,
				 voe_math_float3_scale(basis.up, size.y * 0.25f));

	return square_to_the_camera(&basis, quad, offset,
				    (voe_math_float3){ size.x + margin * 2.0f,
						       size.y + margin * 2.0f,
						       1.0f });
}
