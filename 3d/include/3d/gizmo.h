// The move gizmo as arithmetic (ADR-0205): how big it is in metres at the
// distance it stands, which handle a ray meets, where on that handle a drag is
// measured from, and this frame's triangles for all of it. One position and
// one camera in, numbers out; nothing here knows what a selection, a pointer or
// a theme is, and nothing is uploaded or drawn — 3d/draw_system.h is what hands
// the triangles to the card.
//
//     voe_3d_gizmo gizmo = voe_3d_gizmo_at(position, view, size, 90.0f);
//     voe_3d_ray ray = voe_3d_pick_ray(camera, size, point);
//     voe_math_float3 from;
//     voe_3d_gizmo_mesh plain, marked;
//
//     voe_3d_gizmo_handle under = voe_3d_gizmo_hit(gizmo, ray);
//     if (under != VOE_3D_GIZMO_NONE && voe_3d_gizmo_grab(gizmo, under, ray, &from))
//             ... // `from` is where this drag started, in world metres
//     if (voe_3d_gizmo_quads(gizmo, under, arena, &plain, &marked))
//             ... // two meshes, two draws: at rest and under the pointer
//
// ONE CALL ANSWERS BOTH WHAT IS DRAWN AND WHAT IS GRABBED. The triangles a pass
// draws and the segments a ray is tested against come out of the same struct and
// the same fractions below, so a handle is grabbed exactly where it is seen.
// Splitting the two is how a gizmo comes to be half a centimetre off the thing
// you can grab (ADR-0205).
//
// THE AXES ARE THE WORLD'S. X, Y and Z here are the world's directions, and a
// gizmo that turns with the entity it moves is a later card — it is these three
// directions that become an entity's own when it comes, and nothing else here
// changes.
#pragma once

#include <3d/pick.h>

#include <base/arena.h>

#include <math/float3.h>

#include <render/device.h>

#include <stdbool.h>

// Every measurement but `shaft` is a fraction of it, so the whole gizmo has one
// size and one place to change it.

// The arrowhead's length, a shade under a third of the shaft, so an arrow reads
// as an arrow at the size a gizmo is drawn at.
#define VOE_3D_GIZMO_HEAD_LENGTH 0.30f
// Half the head's width at its base: a third of its length, the proportion that
// reads as a point rather than a spike or a flag.
#define VOE_3D_GIZMO_HEAD_HALF_WIDTH 0.10f
// The plane square's near corner, along both of its axes: clear of where the
// three arrows meet, so a press near the origin is an axis and never a plane.
#define VOE_3D_GIZMO_PLANE_NEAR 0.30f
// The plane square's side, which leaves it ending well short of the heads so a
// square never covers an arrowhead.
#define VOE_3D_GIZMO_PLANE_SIDE 0.25f
// The axis label's height at the tip, about a head long, which is as small as a
// letter drawn in strokes stays readable.
#define VOE_3D_GIZMO_LABEL_HEIGHT 0.18f
// How far off an axis still counts as a hit: a shade wider than the head is, so
// everything drawn can be grabbed without aiming down the line of it.
#define VOE_3D_GIZMO_GRIP 0.12f
// Half a shaft's width, and half a label stroke's: a fortieth of the shaft,
// which reads as a line beside a head a fifth of the shaft across.
#define VOE_3D_GIZMO_LINE_HALF_WIDTH 0.025f
// What a marked handle's widths are multiplied by — half again as wide under
// the pointer, and its lengths unchanged so it is hit over the same reach.
#define VOE_3D_GIZMO_MARKED_STEP 1.5f

// What the two meshes below cost together, whatever is marked: per axis a shaft
// quad, a head triangle and its label's strokes, and per plane a square of two
// triangles. A program sizes voe_render_capacities' transient vertices and
// indices from these, and counts two transient ranges and two draws per pass.
#define VOE_3D_GIZMO_LABEL_STROKES 8 // two for X, three for Y, three for Z
#define VOE_3D_GIZMO_VERTICES \
	(3 * 4 + 3 * 3 + 3 * 4 + VOE_3D_GIZMO_LABEL_STROKES * 4)
#define VOE_3D_GIZMO_INDICES \
	(3 * 6 + 3 * 3 + 3 * 6 + VOE_3D_GIZMO_LABEL_STROKES * 6)

// Which handle: nothing, one of the three axes, or one of the three plane
// squares. The squares are named for the two axes they lie in, in the order the
// axes roll round, so XY, YZ and ZX.
typedef enum {
	VOE_3D_GIZMO_NONE = 0,
	VOE_3D_GIZMO_X,
	VOE_3D_GIZMO_Y,
	VOE_3D_GIZMO_Z,
	VOE_3D_GIZMO_XY,
	VOE_3D_GIZMO_YZ,
	VOE_3D_GIZMO_ZX
} voe_3d_gizmo_handle;

// The gizmo itself: where it stands, who is looking at it and how big it is.
// Plain numbers, copied by value, owned by whoever made it.
typedef struct {
	voe_math_float3 origin; // where the gizmo stands, in metres
	voe_math_float3 eye; // the camera's position, in metres
	float shaft; // one arrow's shaft, in metres
} voe_3d_gizmo;

// The gizmo at `origin`, sized so one shaft covers `pixels` pixels of a picture
// `size` big drawn through `view`.
//
// The origin's depth through `view.view` is floored at a tenth of a millimetre,
// so a gizmo at the eye itself is a very large gizmo and not a division by
// nought. `size.height` is what the size is worked out against, because the
// projection's field of view is the vertical one (3d/outline.h).
//
// THE SIZE IS WORKED OUT ONCE, FROM THE ORIGIN'S DEPTH. An outline's width is
// per vertex, because an edge running away from the eye is nearer at one end
// than at the other and one width would taper wrongly (3d/outline.h). A gizmo is
// one object that has to stay one shape: an arrow sized from its own tip's depth
// would make a different gizmo of every angle, and the handle a person aimed at
// would not be the one the ray meets. So the origin's depth sizes all of it.
voe_3d_gizmo voe_3d_gizmo_at(voe_math_float3 origin, voe_render_view view,
			     voe_platform_size size, float pixels);

// Which handle `ray` meets, nearest along the ray first: each axis as the
// segment from the origin to the far end of its head, met when the ray passes
// within VOE_3D_GIZMO_GRIP of it, and each plane square as the patch of its own
// plane between the near corner and the far one.
//
// VOE_3D_GIZMO_NONE when it meets none of them, when the gizmo is behind the eye
// and when the shaft is nought.
//
// THE HANDLES ARE TESTED BY DISTANCE AND NOT IN ORDER. A plane square stands
// between the two arrows that bound it and overlaps them on the picture, so
// which handle a person meant is which one is in front of the other from where
// they are looking. Nearest along the ray wins; a fixed order would answer with
// whichever was written first, which is right from one side and wrong from the
// other.
//
// "IN FRONT OF THE EYE" IS THE RAY'S OWN PARAMETER BEING POSITIVE. A pick ray
// starts on the near plane and runs away from the eye (3d/pick.h), so a handle
// whose closest approach is at a negative parameter is behind the person: that
// is what a gizmo behind the camera is, and it is met by nothing rather than
// grabbed through the back of their head.
voe_3d_gizmo_handle voe_3d_gizmo_hit(voe_3d_gizmo gizmo, voe_3d_ray ray);

// Where on `handle` this ray lands, in world metres: for an axis, the point of
// that axis line closest to the ray; for a plane, where the ray meets that plane
// through the origin. A drag is this point now against this point when the
// handle was pressed.
//
// False, with `*out` untouched, for VOE_3D_GIZMO_NONE and for a ray too nearly
// along the axis or in the plane to name one point — an ordinary frame the
// caller answers by keeping the position it had.
//
// A REFUSED GRAB IS AN ORDINARY FRAME AND NOT A FAILURE. A ray lying in the
// square's plane, or running along the axis, has no one point on it to measure a
// drag from; the grab says false, writes nothing, and the caller keeps the
// position it had until the next frame. That is a pointer at a glancing angle,
// which happens, and there is nothing to report.
[[nodiscard]] bool voe_3d_gizmo_grab(voe_3d_gizmo gizmo,
				     voe_3d_gizmo_handle handle,
				     voe_3d_ray ray, voe_math_float3 *out);

// The gizmo as geometry, all of it in the arena the build was handed.
typedef struct {
	const voe_render_vertex *vertices;
	uint32_t vertex_count;
	const uint32_t *indices;
	uint32_t index_count;
} voe_3d_gizmo_mesh;

// Builds this frame's triangles into `arena`: every handle but `marked` into
// `plain`, and `marked`'s own handle into `marked_out` at the marked step's
// widths. The three labels are always in `plain`, whatever is marked, and
// VOE_3D_GIZMO_NONE leaves `marked_out` an empty mesh — a mesh the caller draws
// nothing from.
//
// Every vertex's normal is the direction from it to the eye and every triangle
// is wound to face the eye, because the pipeline culls back faces and a zeroed
// normal means something of its own to the shader (3d/outline.h says both at
// length). Nothing is uploaded, and both meshes are the caller's arena's (rule
// 11) and die with it.
//
// False, with both untouched, for a gizmo of no shaft.
//
// THE HANDLES ARE CAMERA-FACING QUADS AND NOT SOLID CONES. An arrow is a flat
// quad widened across its axis towards the eye, with one triangle on the end of
// it for a point: one size, one set of triangles, and every arrow as readable
// from one angle as from another, because each turns to keep its face to the
// person. A cylinder and a cone would be a few hundred triangles that have to be
// lit to read as round, in a pass that draws one unlit white record (ADR-0205),
// and the arrow pointed at the eye would still be a disc. Where an axis and the
// direction to the eye are nearly parallel the width is taken across any
// perpendicular to the axis, so that arrow is a sliver and never nothing.
//
// THE PLANE SQUARES ARE THE ONE PART THAT DOES NOT FACE THE EYE. A square that
// turned to face the person would stop saying which plane it moves the thing in,
// which is the whole of what it is for. It lies in its own plane and is wound
// from whichever side of that plane the eye is on, so it is never culled away.
//
// A LABEL IS STROKES AND NOT A `text` BLOCK (ADR-0206): camera-facing quads at
// the tip of each arrow, two crossed for X, three for Y and three for Z, built
// by this call out of this shaft. A block of text is a mesh and a texture
// wearing a distance-field material, which is another transient range, another
// material and a second record kind per axis for a pass that draws one. The
// letters are a second cue and not reading matter.
//
// THE MARKED HANDLE IS A SECOND MESH AND NOT A SECOND COLOUR. A drawn object's
// colour is one record per draw (ADR-0191), so the one handle under the pointer
// is a draw of its own: it comes out in `marked_out` and out of `plain`, at the
// marked step's widths and at its own lengths, so it covers the same reach it is
// hit over. Two meshes, two transient ranges and two draws, which is what
// ADR-0205 counted.
[[nodiscard]] bool voe_3d_gizmo_quads(voe_3d_gizmo gizmo,
				      voe_3d_gizmo_handle marked,
				      voe_base_arena *arena,
				      voe_3d_gizmo_mesh *plain,
				      voe_3d_gizmo_mesh *marked_out);
