// The writing: the font, the sign lettered on both faces, the heads-up line on
// its dark panel and the readout's entity. See text.h for why it is its own
// file; what each string is for and fails like stands above add_text.
//
// THE READOUT GETS NO GEOMETRY HERE. Its block is built inside every frame by
// main.c and hung on the entity made here; this file only makes the entity and
// its material.
#include "text.h"
#include "quad.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <math/quat.h>
#include <scene/transform_system.h>

// Half a turn, radians: what stands the sign's back face behind its front.
#define HALF_TURN 3.14159265f

// The two strings, and the two placements the card asks to see: one standing in
// the world and one locked to the camera.
//
// THE SIGN IS TWO ENTITIES SHARING ONE MESH, BACK TO BACK. A text block is four
// vertices and six indices per glyph — one face — and the engine culls back
// faces, so a single sign vanishes for half of the camera's lap. Two entities
// with the same geometry, one of them turned half a turn about Y, is a sign
// lettered on both sides; it costs one more transform and one more draw and
// nothing else. That is a double-sided *arrangement* and not a double-sided
// material, the same distinction dev/src/quad.h already makes about the quads.
//
// IT HAS AN ACCENTED CHARACTER IN IT ON PURPOSE. Most accented characters are
// composite glyphs — references to other glyphs with an offset — and a reader
// that handles only simple outlines draws them as blanks while looking perfectly
// correct on an English string. If the `å` is missing, that is the bug and
// text/tests/truetype.c is where it should have been caught.
#define SIGN_TEXT "VOE3D\nunlit · blended · Oxanium\nÅNGSTRÖMÄ · éüåäöÇ"
#define SIGN_EM 0.30f
#define SIGN_HEIGHT 3.4f

// The heads-up line's string and its size. Where it stands is src/facing.c's.
//
// NEITHER STRING HOLDS A CHARACTER OUTSIDE LATIN-1, AND THAT IS DELIBERATE. The
// atlas covers the space to U+00FF; anything else is drawn as the font's
// missing-glyph box, which is right and looks exactly like a bug. An em dash
// stood here until it did.
#define HUD_TEXT "locked to the camera · Tab to fly"
#define HUD_EM 0.055f

// What the two are tinted. The base colour factor of an unlit material is
// exactly what comes out, because nothing multiplies it by a light — so these
// are the colours on screen and not a starting point for one.
#define SIGN_TINT_R 1.0f
#define SIGN_TINT_G 0.86f
#define SIGN_TINT_B 0.45f
// The dark panel behind the heads-up line, and what it is for.
//
// IT IS A READING AID AND IT IS NOT PART OF THE ENGINE'S ANSWER TO ANYTHING. The
// line is pale blue over whatever the scene happens to put behind it, and over
// the teal background the two are close enough in luminance that a perfectly
// sharp edge still reads as soft — which is a question about contrast and not
// about the glyphs. A dark, opaque panel behind it settles that by making the
// contrast the largest it can be: what still looks soft on this is soft, and
// what does not was never soft.
//
// OPAQUE AND IN THE OVERLAY, WHICH IS WHY IT DOES NOT NEED SORTING. The overlay
// draws its solid group first and writes depth, then its blended group tests
// against it — so a panel pushed a little further from the eye than the line is
// behind it by depth and not by luck. It is the same arrangement the solid
// overlay quad already proves; see voe_dev_add_the_quads in src/quad.c.
//
// THERE IS NO KEY TO TURN IT OFF, AND THAT IS `platform`'s DOING RATHER THAN A
// CHOICE. Every key voe_platform_key names is already bound — P is the present
// mode and Tab is the camera — so a toggle would mean adding one, which is
// another folder. It costs the strip of scene directly behind the line, which is
// background in every frame this program draws.
#define HUD_PANEL_R 0.04f
#define HUD_PANEL_G 0.05f
#define HUD_PANEL_B 0.06f

#define HUD_TINT_R 0.75f
#define HUD_TINT_G 0.92f
#define HUD_TINT_B 1.0f

// One text block, one entity: the mesh the font built, an unlit blended material
// wearing the atlas, and the transform it is placed with.
//
// Above all of it, three lines of writing on nothing — a sign in the world,
// lettered on both faces — and across the bottom of the view, one line of it
// that stays where it is however the camera moves and that nothing gets in front
// of.
//
// THE SIGN STANDS IN THE WORLD AND THE LINE IS LOCKED TO THE CAMERA, which are
// the two placements text has. The sign is three lines of Oxanium above the
// cubes and it is part of the scene: it turns with the orbit, it is read at an
// angle for most of a lap, and something in front of it hides it. The line sits
// a metre in front of the eye and stays where it is on screen however the camera
// moves — and it is still an object in the world, placed by a transform intent
// every frame. There is no screen-space path in this engine and there is not
// going to be one, so a heads-up display is a quad in front of the camera.
//
// AND THE LINE IS IN THE OVERLAY, SO NOTHING COVERS IT. Fly into a cube and the
// writing stays readable on top of it, which it did not before card 024: at one
// metre in front of the eye it went inside anything you walked into. The sign
// stays in the world and is still hidden by whatever gets between it and the
// camera, and having both is the point — the layer is a property of a drawable
// and not a switch the program is in.
//
// THE TEXT DOES NOT CHANGE AS THE SUN GOES ROUND, AND THAT IS THE UNLIT FLAG.
// The cubes brighten and darken through the lap; the writing keeps exactly the
// colour its material asks for, because an unlit material skips the whole
// shading model. Writing that dims when the sun crosses it is the flag missing,
// and it is the failure this is here to make obvious.
//
// AND IT IS BLENDED, SO A GLYPH IS A SHAPE AND NOT A BOX. Each letter is a quad
// whose alpha the shader works out from the sheet's distance field; a square of
// background round every letter is the alpha mode wrong, and text noticeably
// paler than the tint it asks for is a colour multiplied by its coverage twice.
//
// AND IT IS SHARP AT EVERY SIZE, WHICH IS WHAT THE SIGN IS FOR. Fly up to the
// sign until one letter fills the screen: its edges stay clean and its corners
// stay square. Fly away and it fades rather than crawling. Soft edges close up
// mean the material forgot base_colour_distance_field or the sheet was uploaded
// smooth; rounded corners mean the three channels came out of the sheet the
// same, which is the colouring in text/src/raster.c having gone wrong.
//
// THE HEADS-UP LINE SITS ON A DARK PANEL, AND THAT IS ABOUT CONTRAST AND NOT
// ABOUT SHARPNESS. Pale blue over teal is a small enough difference in luminance
// that a one-pixel edge still reads as soft, which sends anyone looking at it
// hunting for a blur that is not there. The panel takes that question off the
// table: the edge is the same width over it, and what still looks soft on it is
// soft. See HUD_PANEL_R.
//
// AND THERE IS NO ANTIALIASING ANYWHERE IN THE PICTURE, WHICH IS THE ENGINE'S
// RULE AND NOT A GAP. Letters have hard edges, textures show their texels close
// up and shimmer at a distance, and polygon silhouettes are stair-stepped. Every
// one of those is intended. What to look for instead is that the edges are in
// the RIGHT PLACE: a letter walked up to has straight sides and square corners
// rather than blocks, which is the distance field doing its job under a hard
// cut. Blocks would mean the sheet had become a picture of coverage again.
//
// TEXT A LONG WAY OFF BREAKS INTO SPECKS AND THEY MOVE. Expected, and the direct
// cost of the rule above; the sign at the top of the scene is where to see it.
//
// THE ACCENTED CHARACTERS ARE THE READER'S TEST. `Å`, `Ö`, `é`, `ü`, `å` and `Ç`
// are composite glyphs — references to other glyphs with an offset — and a
// reader that handles only simple outlines draws them as blanks while an English
// string looks perfect. If they are missing, that is the bug.
//
// THE COUNTERS ARE HOLES. The middles of `O`, `D`, `e`, `a`, `o`, `ö` and `å`
// are the background and not the letter. Filled in solid is the fill rule: see
// text/tests/raster.c, which is where that should have been caught.
//
// THEY ARE THE ONLY THINGS IN THE SCENE DRAWN IN THE SECOND PASS ALONGSIDE THE
// QUADS. Everything else is opaque and goes through the ordinary draw in table
// order; see 3d/draw_system.h.
//
// THE FIVE THINGS A TEXT MATERIAL HAS TO SAY, and each of them is visible if it
// is missing. The atlas as the base colour texture, or there is nothing to see.
// The tint as the base colour factor, which for an unlit material is exactly the
// colour on screen. `unlit`, or the letters darken and brighten as the sun goes
// round — text lit by a sun is the failure this flag exists to prevent. BLENDED,
// or every glyph arrives in a square of its own background. And
// `base_colour_distance_field`, or the sheet is read as a picture and every
// glyph is a solid rectangle.
//
// AND THE TINT IS NOT PREMULTIPLIED HERE. It is an ordinary colour with an alpha
// beside it; the shader multiplies at the very end. See dev's other blended
// thing, voe_dev_add_quad in src/quad.c, which says the same in the same words.
//
// THE LAYER IS THE CALLER'S AND IT IS NOT A FIFTH THING THE MATERIAL SAYS. Both
// of this program's strings wear the same material and they are in different
// layers: the sign is part of the scene and the heads-up line is above it. Unlit
// and overlay travel together here by coincidence and not by rule — see the
// two placements above.
//
// What is wrong if it looks wrong:
//
//   - Writing that brightens and dims as the sun goes round — the material's
//     `unlit` flag never reached the shading record.
//   - THE HEADS-UP LINE DISAPPEARING WHEN YOU FLY INTO SOMETHING — the layer.
//     Either the line is not in the overlay or the depth clear between the two
//     is not happening, and both look identical from here.
//   - Everything on top of everything, or the sign no longer hidden by what is
//     in front of it — the opposite failure, and the worse one: the layer has
//     become a switch the whole frame is in rather than a property of one
//     drawable. The sign and the cubes are what to check, not the line.
//   - A square of background round every letter — the text material's alpha mode
//     arriving as opaque, which is the same failure as the quad above wearing a
//     different shape.
//   - Writing noticeably paler than the tint it asks for — a colour multiplied
//     by its coverage twice, which is the atlas having been premultiplied when
//     the shader is what does that. See text/src/font.c.
//   - Letters soft or blurry as the camera closes on the sign — the material's
//     base_colour_distance_field not set, or the sheet uploaded as colour or
//     sampled smooth. All three look the same and text/src/font.c is where the
//     three are decided together.
//   - Corners on `V`, `A` and the flat terminals coming out rounded — the sheet
//     is a distance field but its three channels agree, so the median has
//     nothing to reconstruct. That is the edge colouring, and
//     text/tests/raster.c is the automated form of it.
//   - Accented characters missing while an English string is perfect — composite
//     glyphs, and text/tests/truetype.c is the automated form.
//   - The middles of `O`, `e` and `a` filled in solid — the fill rule.
//     text/tests/raster.c is the automated form of that one.
//   - The sign missing for half the lap — one of its two faces did not get
//     built. A text block is one face, and the pair of entities is what makes it
//     a sign rather than a decal.
//   - A box where a character should be — the character is outside the range the
//     atlas covers, and that box is the font's own missing-glyph glyph. Correct,
//     and a reason to change the string rather than the folder.
static bool add_text(voe_ecs_world *world, voe_render_device *gpu,
		     const voe_text_font *font, voe_text_block block,
		     voe_math_float4 tint, voe_scene_transform transform,
		     voe_3d_layer layer, voe_ecs_entity *out,
		     voe_base_error *error)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_material material = {
		.base_colour = tint,
		.metallic = 0.0f,
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
		.alpha_cutoff = 0.5f,
		.unlit = true,
		.base_colour_texture = voe_text_font_atlas(font),
		// The atlas is a distance field and not a picture. Without this
		// the shader multiplies the tint by three distances and reads
		// the sheet's alpha, which is opaque everywhere: solid coloured
		// rectangles where the writing should be.
		.base_colour_distance_field = true,
	};

	if (!voe_3d_material_upload(gpu, &material, error))
		return false;
	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_mesh_add(world, entity,
			     (voe_3d_mesh){ .geometry = block.geometry,
					    .layer = layer }))
		return false;
	if (!voe_3d_material_add(world, entity, material))
		return false;

	*out = entity;
	return true;
}

// The font, and the four text entities it is drawn into: the sign's two faces,
// the line locked to the camera, and the readout, which gets its geometry every
// frame rather than here.
//
// THE FONT IS MADE HERE AND NOT AT STARTUP, which is the difference between a
// program that draws text and one that does not. Nothing in `render` builds an
// atlas; this call is what builds it, once, and a program that never makes one
// never pays for it. It is what this call returns, rather than a second
// out-parameter, because rule 6 allows one level of dereference and the font
// is made inside this function and handed straight back.
//
// THE SIGN IS CENTRED ON ITS OWN WIDTH. A block's origin is the left end of its
// first baseline, so a sign that was not shifted would hang off to one side of
// whatever it is standing on.
[[nodiscard]] voe_text_font *voe_dev_add_the_text(voe_ecs_world *world,
						  voe_render_device *gpu,
						  voe_base_arena *arena,
						  voe_render_geometry quad,
						  voe_ecs_entity *hud,
						  voe_ecs_entity *panel,
						  voe_ecs_entity *readout,
						  voe_math_float2 *hud_size,
						  voe_base_error *error)
{
	voe_text_block sign;
	voe_text_block line;
	voe_math_float4 sign_tint = { SIGN_TINT_R, SIGN_TINT_G, SIGN_TINT_B,
				      1.0f };
	voe_math_float4 hud_tint = { HUD_TINT_R, HUD_TINT_G, HUD_TINT_B, 1.0f };
	voe_scene_transform front;
	voe_scene_transform back;
	voe_ecs_entity unused = { 0 };
	voe_text_font *font;

	// Oxanium, which is dev's face and stays so — 005's own criterion 7
	// keeps the dev program on it while the editor is free to name either.
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, gpu, arena, error);
	if (font == NULL)
		return NULL;

	if (!voe_text_block_create(font, gpu, arena, SIGN_TEXT, SIGN_EM, &sign,
				   error))
		return NULL;
	if (!voe_text_block_create(font, gpu, arena, HUD_TEXT, HUD_EM, &line,
				   error))
		return NULL;

	front = (voe_scene_transform){
		.position = { -sign.size.x * 0.5f, SIGN_HEIGHT, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	// The same mesh, turned half a turn about Y and shifted the other way,
	// so that its left end lands where the front face's right end is and the
	// two sit exactly back to back.
	back = (voe_scene_transform){
		.position = { sign.size.x * 0.5f, SIGN_HEIGHT, 0.0f },
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, HALF_TURN),
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	// The sign is in the world, so that walking something in front of it
	// still hides it. That is half of what the two placements are for.
	if (!add_text(world, gpu, font, sign, sign_tint, front,
		      VOE_3D_LAYER_WORLD, &unused, error))
		return NULL;
	if (!add_text(world, gpu, font, sign, sign_tint, back,
		      VOE_3D_LAYER_WORLD, &unused, error))
		return NULL;

	// The heads-up line starts wherever; the loop places it every frame from
	// where the camera actually is, and its width is what centres it there.
	//
	// AND IT IS IN THE OVERLAY, WHICH IS THE OTHER HALF. A line of writing
	// that tells you what the keys do is no use at the moment you fly into
	// something, and until card 024 that is exactly when it disappeared. It
	// is still an ordinary object in the world with a position in metres —
	// what changed is when it is drawn, not where it is.
	// The panel first, so that the solid thing exists before the blended
	// thing that sits on it. Order of creation decides nothing here — the
	// draw system sorts by layer and alpha mode — but reading it in this
	// order is how the picture is built.
	if (!voe_dev_add_quad(
		    world, gpu, quad,
		    voe_dev_quad_material((voe_math_float4){ HUD_PANEL_R,
							     HUD_PANEL_G,
							     HUD_PANEL_B, 1.0f },
					  VOE_RENDER_ALPHA_OPAQUE, true),
		    voe_dev_quad_at((voe_math_double3){ 0.0, 0.0, 0.0 }, 1.0),
		    VOE_3D_LAYER_OVERLAY, panel, error))
		return NULL;

	*hud_size = line.size;
	if (!add_text(world, gpu, font, line, hud_tint, front,
		      VOE_3D_LAYER_OVERLAY, hud, error))
		return NULL;

	// The readout: an entity wearing the text material and, for now, no
	// geometry. The block it draws is built inside every frame and put on
	// it with voe_3d_mesh_set_geometry, so a zeroed id — which names
	// nothing — is exactly right until the first frame opens, and nothing
	// draws it before then. Same material, same layer and, until the loop
	// places it, the same transform as the line.
	if (!add_text(world, gpu, font, (voe_text_block){ 0 }, hud_tint, front,
		      VOE_3D_LAYER_OVERLAY, readout, error))
		return NULL;

	return font;
}
