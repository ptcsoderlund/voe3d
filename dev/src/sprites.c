// The sheet's pixels, where the twelve sprites stand, and the two rotations that
// turn towards the camera. See sprites.h for what this file is.
//
// ---- THE SHEET IS BUILT HERE AND NOT READ FROM A FILE ----
//
// Four cells across and two down, thirty-two texels each: a filled disc that
// grows a little in every cell, in a different colour in every cell, with an
// opaque white square in each cell's top-left corner. The square is what makes a
// wrong rectangle obvious — a cell picked one column over, or a sheet read
// upside down, moves it somewhere a person notices immediately, which a disc on
// its own would not. The last cell is a triangle instead, and the comment on
// TRIANGLE_TOP says why.
//
// THE DISC'S EDGE IS SOFT AND THE TEXELS OUTSIDE IT ARE EMPTY, WHICH IS THE
// CASE WORTH LOOKING AT. Alpha ramps across two texels at the rim and the colour
// is written nowhere else — no edge is bled outward into the transparent texels
// (ADR-0082). An engine that filtered its pictures would show a dark halo round
// every disc, because filtering would average the rim's colour with the nothing
// beside it. This one samples NEAREST everywhere, so there is no path from these
// texels to a colour that is not in them, and the halo cannot happen. If a halo
// ever appears here, something started filtering.
//
// ---- WHAT EACH GROUP OF SPRITES IS FOR ----
//
// THE ROW OF FOUR is one sheet, one texture and one quad, read four different
// ways. Four entities, four frames, and the only thing that differs between
// their materials is a rectangle.
//
// THE CUTOUT ONE UNDER IT is the same frame as the row's last, drawn the other
// way. Its edge is hard and it crawls as the camera moves — that is what a
// cutout is (ADR-0078) and it is not a defect to fix here. Held against the
// blended disc directly above it, the difference is the whole of what the two
// alpha modes are.
//
// THE PAIR AT THE STILL CUBE is the layer, and it is two claims at once. The
// world one stands just behind the cube, so the cube covers it from one side of
// the lap and it comes out from the other; the two overlay ones stand inside the
// cube and are never covered at all. Both are in metres and both are seen
// through the same camera — an overlay is "above the world", not screen space.
// The overlay pair straddle the cube's middle in Z, so which of them is nearer
// swaps twice a lap and the nearer one's colour has to be on top of the overlap:
// a layer built out of a disabled depth test would get that half wrong.
//
// NEITHER LAYER IS THE NORMAL CASE, which is why there are sprites in both and
// why they are the same sprites. A character in the world and a health bar over
// it are both ordinary.
//
// THE SEE-THROUGH PAIR OFF TO ONE SIDE straddle the origin in Z at one X,
// exactly as the world's two see-through quads do, so the sort is checked for
// sprites the same way it is checked for quads: whichever the camera is behind
// is the one whose colour is on top, and a frame that looks right from one side
// and wrong from the other is the sort and nothing else.
//
// THE TWO AT THE BACK are the facing rotations, side by side. They wear the same
// material — the triangle — so the only difference a person can see is the
// rotation, and a triangle is what makes a rotation something a person can see
// at all. Tab into the flying camera and climb: the left one stays upright and
// foreshortens, the right one leans back and does not. See voe_dev_sprites_face
// at the bottom of this file, which is the whole of what "the engine does not
// billboard" costs a program.
#include "sprites.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <base/assert.h>
#include <math/float3.h>
#include <math/quat.h>
#include <scene/transform_system.h>
#include <sprite/quad.h>
#include <sprite/sheet.h>

#include <math.h>
#include <stdint.h>

// ---- THE SHEET ----

#define CELL 32
#define SHEET_COLUMNS 4
#define SHEET_ROWS 2
#define SHEET_FRAMES 8
#define SHEET_WIDTH (CELL * SHEET_COLUMNS)
#define SHEET_HEIGHT (CELL * SHEET_ROWS)

// The disc in the first cell and how much bigger it gets in each one after it.
// The largest still leaves room for the ramp inside the cell, which matters:
// a disc that ran off the edge would be clipped by the rectangle and read as a
// wrong rectangle rather than as a disc that is too big.
#define DISC_RADIUS 5.0f
#define DISC_STEP 1.1f
// Texels the alpha ramps over at the rim. Two, so it is unmistakably soft.
#define DISC_EDGE 2.0f

// The corner square, in texels. Far enough outside the largest disc to stay a
// separate mark.
#define MARK 4

// The last cell is not a disc, and that is the one thing in this sheet that is
// there for a reason other than looking different.
//
// A DISC CANNOT SHOW A ROTATION. It is the same shape from every angle, so the
// two sprites at the back — the whole point of which is that one of them turns
// differently from the other — would be two circles that stay circles. A
// triangle standing on its base has an unmistakable up, and leaning it back or
// squashing it is visible at a glance.
//
// AND ITS EDGE IS HARD WHERE THE DISCS' ARE SOFT, so one sheet holds both kinds
// of picture. Nothing in the engine has to be told which is which.
#define TRIANGLE_TOP 4.0f
#define TRIANGLE_BOTTOM 28.0f
#define TRIANGLE_HALF 12.0f

static const uint8_t FRAME_COLOURS[SHEET_FRAMES][3] = {
	{ 230, 70, 60 },   { 235, 140, 40 },  { 235, 205, 50 },
	{ 120, 210, 70 },  { 60, 200, 180 },  { 70, 140, 235 },
	{ 140, 100, 230 }, { 230, 90, 180 },
};

// ---- WHERE THEY STAND ----

// The row of four, above and behind the cubes.
#define ROW_Y 1.9f
#define ROW_Z (-1.4f)
#define ROW_X (-1.8f)
#define ROW_STEP 1.2f
#define ROW_SIZE 0.9f

// The cutout one, directly under the row's last disc.
#define CUTOUT_Y 0.85f
#define CUTOUT_X 1.8f
#define CUTOUT_SIZE 0.9f

// The still cube is at the origin and is one metre across, so this stands just
// behind its back face and these stand inside it.
#define BEHIND_Z (-0.9f)
#define BEHIND_SIZE 0.9f
#define OVERLAY_Z 0.22f
#define OVERLAY_SIZE 0.5f

// The see-through pair, off to the -X side of everything else.
#define PAIR_X (-2.6f)
#define PAIR_Y 0.9f
#define PAIR_Z 1.3f
#define PAIR_SIZE 1.1f

// The two that turn to face the camera, standing on the ground at the back. Half
// a size up, so their feet are on it.
#define FACING_X 0.8f
#define FACING_Z (-3.4f)
#define FACING_SIZE 1.0f
#define FACING_Y (FACING_SIZE * 0.5f)

// Which frame each of them wears. Named rather than written into the calls
// because "the same frame twice" is a claim two of them make on purpose.
#define FRAME_ROW_FIRST 0
#define FRAME_ROW_STEP 2
#define FRAME_CUTOUT 6
#define FRAME_BEHIND 4
#define FRAME_OVERLAY_NEAR 2
#define FRAME_OVERLAY_FAR 3
#define FRAME_PAIR_NEAR 1
#define FRAME_PAIR_FAR 5
#define FRAME_FACING 7

// How much of a texel the disc covers: one at the middle, nothing outside, and a
// ramp two texels wide across the rim.
static float disc_coverage(float x, float y, float radius)
{
	float dx = x - (float)CELL * 0.5f;
	float dy = y - (float)CELL * 0.5f;
	float distance = sqrtf(dx * dx + dy * dy);

	return (radius - distance) / DISC_EDGE + 0.5f;
}

// The triangle standing on its base: in or out, with no ramp at all.
static float triangle_coverage(float x, float y)
{
	float half;

	if (y < TRIANGLE_TOP || y >= TRIANGLE_BOTTOM)
		return 0.0f;

	half = (y - TRIANGLE_TOP) / (TRIANGLE_BOTTOM - TRIANGLE_TOP) *
	       TRIANGLE_HALF;
	if (fabsf(x - (float)CELL * 0.5f) > half)
		return 0.0f;
	return 1.0f;
}

// One cell of the sheet: this frame's shape in this frame's colour, the corner
// mark, and nothing anywhere else.
static void draw_cell(uint8_t *pixels, uint32_t frame)
{
	uint32_t cell_x = (frame % SHEET_COLUMNS) * CELL;
	uint32_t cell_y = (frame / SHEET_COLUMNS) * CELL;
	float radius = DISC_RADIUS + (float)frame * DISC_STEP;

	for (uint32_t y = 0; y < CELL; y++) {
		for (uint32_t x = 0; x < CELL; x++) {
			uint8_t *texel =
				&pixels[((cell_y + y) * SHEET_WIDTH + cell_x +
					 x) * 4];
			float at_x = (float)x + 0.5f;
			float at_y = (float)y + 0.5f;
			float coverage =
				frame == SHEET_FRAMES - 1 ?
					triangle_coverage(at_x, at_y) :
					disc_coverage(at_x, at_y, radius);

			if (x < MARK && y < MARK) {
				texel[0] = 255;
				texel[1] = 255;
				texel[2] = 255;
				texel[3] = 255;
				continue;
			}

			if (coverage <= 0.0f) {
				// Empty, and empty means empty: no colour is
				// left here for a filter to find. See the
				// header.
				texel[0] = 0;
				texel[1] = 0;
				texel[2] = 0;
				texel[3] = 0;
				continue;
			}
			if (coverage > 1.0f)
				coverage = 1.0f;

			texel[0] = FRAME_COLOURS[frame][0];
			texel[1] = FRAME_COLOURS[frame][1];
			texel[2] = FRAME_COLOURS[frame][2];
			texel[3] = (uint8_t)(coverage * 255.0f + 0.5f);
		}
	}
}

// The sheet, uploaded. SHARP because it is a sheet something indexes into and
// not a picture that tiles: addressed REPEAT, a coordinate a hair outside one
// cell wraps to the far side and fetches a different sprite. COLOUR because it
// is a picture — the distance-field path is the glyph sheet's alone.
static bool upload_sheet(voe_render_device *gpu, voe_base_arena *arena,
			 voe_render_texture *out, voe_base_error *error)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	uint8_t *pixels = voe_base_arena_push(
		arena, (size_t)SHEET_WIDTH * SHEET_HEIGHT * 4);
	bool made;

	for (uint32_t frame = 0; frame < SHEET_FRAMES; frame++)
		draw_cell(pixels, frame);

	made = voe_render_texture_create(gpu, VOE_RENDER_TEXTURE_COLOUR,
					 VOE_RENDER_SAMPLING_SHARP,
					 SHEET_WIDTH, SHEET_HEIGHT, pixels, out,
					 error);
	// The device took its own copy, so the pixels were scratch.
	voe_base_arena_rewind(arena, mark);
	return made;
}

// The material every frame of the sheet is a copy of. Unlit, because a sprite is
// a picture somebody chose the colours of and a sun crossing it is wrong rather
// than pretty (ADR-0071); the alpha mode and the rectangle are what differ.
//
// THE BASE COLOUR IS WHITE AND THE ALPHA IS ONE, so what is seen is exactly the
// sheet. The disc's own ramp is what makes a blended sprite see-through at its
// rim; a factor of a half here would fade the whole disc as well and the two
// would be impossible to tell apart.
static voe_3d_material sprite_material(voe_render_texture sheet,
				       voe_render_alpha_mode alpha_mode)
{
	voe_3d_material material = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 1.0f,
		.alpha_mode = alpha_mode,
		.alpha_cutoff = 0.5f,
		.unlit = true,
		.base_colour_texture = sheet,
	};

	return material;
}

// One sprite: the shared quad, one of the sheet's materials, a transform, and
// the layer it is drawn in. The material is already uploaded — a frame's record
// is made once and worn by however many entities want that frame.
static bool add_sprite(voe_ecs_world *world, voe_render_geometry quad,
		       voe_3d_material material, voe_math_float3 position,
		       float size, voe_3d_layer layer, voe_ecs_entity *out)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { size, size, 1.0f },
	};

	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_mesh_add(world, entity,
			     (voe_3d_mesh){ .geometry = quad, .layer = layer }))
		return false;
	if (!voe_3d_material_add(world, entity, material))
		return false;

	// Only the two that turn are of any use afterwards.
	if (out != NULL)
		*out = entity;
	return true;
}

bool voe_dev_sprites_add(voe_ecs_world *world, voe_render_device *gpu,
			 voe_base_arena *arena, voe_dev_sprites *out,
			 voe_base_error *error)
{
	voe_render_geometry quad = { 0 };
	voe_render_texture sheet = { 0 };
	voe_sprite_sheet grid = { .columns = SHEET_COLUMNS,
				  .rows = SHEET_ROWS,
				  .frames = SHEET_FRAMES };
	voe_3d_material frames[SHEET_FRAMES];
	voe_3d_material cutout;

	VOE_BASE_ASSERT(out != NULL, "building the sprites into nothing");

	if (!voe_sprite_quad_create(gpu, &quad, error))
		return false;
	if (!upload_sheet(gpu, arena, &sheet, error))
		return false;

	// Eight records over one geometry and one texture, and the only thing
	// that differs between them is a rectangle.
	if (!voe_sprite_sheet_upload(gpu, grid,
				     sprite_material(sheet,
						     VOE_RENDER_ALPHA_BLENDED),
				     frames, error))
		return false;

	// The ninth, framed by hand rather than by the sheet's loop, because
	// only one frame of the sheet is wanted the other way round.
	cutout = sprite_material(sheet, VOE_RENDER_ALPHA_CUTOUT);
	voe_sprite_sheet_frame(grid, FRAME_CUTOUT, &cutout);
	if (!voe_3d_material_upload(gpu, &cutout, error))
		return false;

	for (uint32_t i = 0; i < SHEET_COLUMNS; i++) {
		voe_math_float3 at = { ROW_X + (float)i * ROW_STEP, ROW_Y,
				       ROW_Z };

		if (!add_sprite(world, quad,
				frames[FRAME_ROW_FIRST + i * FRAME_ROW_STEP],
				at, ROW_SIZE, VOE_3D_LAYER_WORLD, NULL))
			return false;
	}

	if (!add_sprite(world, quad, cutout,
			(voe_math_float3){ CUTOUT_X, CUTOUT_Y, ROW_Z },
			CUTOUT_SIZE, VOE_3D_LAYER_WORLD, NULL))
		return false;

	// Behind the still cube, in the world: covered by it from one side of
	// the lap and out in front from the other.
	if (!add_sprite(world, quad, frames[FRAME_BEHIND],
			(voe_math_float3){ 0.0f, 0.0f, BEHIND_Z }, BEHIND_SIZE,
			VOE_3D_LAYER_WORLD, NULL))
		return false;

	// Inside the same cube, in the overlay: never covered by it, and still
	// occluding each other the right way round.
	if (!add_sprite(world, quad, frames[FRAME_OVERLAY_NEAR],
			(voe_math_float3){ 0.0f, 0.0f, OVERLAY_Z },
			OVERLAY_SIZE, VOE_3D_LAYER_OVERLAY, NULL))
		return false;
	if (!add_sprite(world, quad, frames[FRAME_OVERLAY_FAR],
			(voe_math_float3){ 0.0f, 0.0f, -OVERLAY_Z },
			OVERLAY_SIZE, VOE_3D_LAYER_OVERLAY, NULL))
		return false;

	// The see-through pair, straddling the origin in Z at one X.
	if (!add_sprite(world, quad, frames[FRAME_PAIR_NEAR],
			(voe_math_float3){ PAIR_X, PAIR_Y, PAIR_Z }, PAIR_SIZE,
			VOE_3D_LAYER_WORLD, NULL))
		return false;
	if (!add_sprite(world, quad, frames[FRAME_PAIR_FAR],
			(voe_math_float3){ PAIR_X, PAIR_Y, -PAIR_Z }, PAIR_SIZE,
			VOE_3D_LAYER_WORLD, NULL))
		return false;

	// The two that turn. They wear one material, so nothing but the rotation
	// can differ between them. Placed here and moved every frame by
	// voe_dev_sprites_face.
	if (!add_sprite(world, quad, frames[FRAME_FACING],
			(voe_math_float3){ -FACING_X, FACING_Y, FACING_Z },
			FACING_SIZE, VOE_3D_LAYER_WORLD, &out->cylindrical))
		return false;
	return add_sprite(world, quad, frames[FRAME_FACING],
			  (voe_math_float3){ FACING_X, FACING_Y, FACING_Z },
			  FACING_SIZE, VOE_3D_LAYER_WORLD, &out->spherical);
}

// The whole of billboarding, in game code, where ADR-0080 puts it.
//
// CYLINDRICAL IS THE YAW AND SPHERICAL IS THE YAW AND THEN THE PITCH. That one
// extra term is the entire difference, and what it looks like is worth knowing
// before choosing: the cylindrical one stays upright however high the camera
// climbs, so a sprite standing on the ground goes on standing on it and is seen
// more and more edge-on from above. The spherical one leans back to stay square
// to the eye, so it never foreshortens and its feet come off the ground. Neither
// is right in general, which is why the engine picks neither.
//
// THE FLIGHT'S OWN yaw AND pitch, AND NO TRIGONOMETRY HERE. The flight already
// holds the two angles the camera is aimed with; a program that worked them back
// out of a forward vector would be undoing arithmetic that was never done. The
// spherical one wears the flight's whole pose rotation, the same one the eye is
// submitted with this frame.
void voe_dev_sprites_face(voe_ecs_world *world, voe_dev_flight flight,
			  const voe_dev_sprites *sprites)
{
	static const voe_math_float3 UP = { 0.0f, 1.0f, 0.0f };
	voe_math_quat yaw;

	VOE_BASE_ASSERT(world != NULL, "facing sprites in no world");
	VOE_BASE_ASSERT(sprites != NULL, "facing nothing towards the camera");

	yaw = voe_math_quat_from_axis_angle(UP, flight.yaw);

	(void)voe_scene_transform_submit(
		world,
		(voe_scene_transform_intent){
			.entity = sprites->cylindrical,
			.transform = {
				.position = { -FACING_X, FACING_Y, FACING_Z },
				.rotation = yaw,
				.scale = { FACING_SIZE, FACING_SIZE, 1.0f },
			},
		});

	(void)voe_scene_transform_submit(
		world,
		(voe_scene_transform_intent){
			.entity = sprites->spherical,
			.transform = {
				.position = { FACING_X, FACING_Y, FACING_Z },
				.rotation = voe_dev_flight_pose(flight).rotation,
				.scale = { FACING_SIZE, FACING_SIZE, 1.0f },
			},
		});
}
