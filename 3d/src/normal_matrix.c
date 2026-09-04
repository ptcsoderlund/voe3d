// The normal matrix: the inverse transpose of a world matrix, and the one branch
// that keeps a degenerate transform from asserting inside `math`.
//
// WHY THE INVERSE TRANSPOSE IS THE ANSWER, IN ONE PARAGRAPH. A normal n and a
// tangent t of the same surface are perpendicular: n·t = 0. A transform M moves
// the tangent to M·t, because a tangent is a difference of two points and points
// are what M carries. Whatever matrix N moves the normal has to keep the two
// perpendicular — (N·n)·(M·t) = 0 — and writing that out as
// nᵀ·Nᵀ·M·t = 0 says it holds for every tangent exactly when Nᵀ·M is the
// identity, so N = (M⁻¹)ᵀ. That is the whole derivation and it is three lines
// because the alternative is a comment saying "this is the standard trick".
//
// THE DETERMINANT IS CHECKED HERE RATHER THAN AVOIDED. voe_math_float4x4_inverse
// asserts on a singular matrix — deliberately, so that nothing in the engine
// propagates infinities quietly — and a world matrix with a zero scale on one
// axis is singular and is a thing a file may contain. So the determinant is
// asked for first and the identity is the answer when there is no inverse; see
// the header for why the identity and not something cleverer.
#include <3d/normal_matrix.h>

#include <math.h>

// How small a determinant is treated as none, and it is a threshold rather than
// a comparison with zero for a reason. The determinant of a transform
// component's matrix is the product of its three scales, so this is a threshold
// on volume: an object flattened by an exporter often comes out with a scale of
// 1e-8 rather than a clean zero, and the inverse of that is a matrix full of
// numbers big enough to overflow to infinity on the way through the shader's
// normalize. A tenth of a millimetre cubed — the volume of a box a tenth of a
// millimetre on each side, in an engine whose unit is the metre — is the
// smallest thing this will claim to have a surface direction for; below it, the
// model's own normals are used unchanged.
#define LEAST_VOLUME 1e-12f

voe_math_float4x4 voe_3d_normal_matrix(voe_math_float4x4 world)
{
	if (fabsf(voe_math_float4x4_determinant(world)) < LEAST_VOLUME)
		return voe_math_float4x4_identity();

	return voe_math_float4x4_transpose(voe_math_float4x4_inverse(world));
}
