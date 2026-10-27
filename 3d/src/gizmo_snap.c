// Snap's three roundings — see the header for why a position lands on the
// world grid, why a turn is whole steps from the press and why a scale never
// reaches nought.
//
// Each is round(value / step) × step on the components the handle names, in
// double; a position stays double throughout, an angle and a scale are widened
// for the arithmetic and narrowed once at the end.
#include <3d/gizmo_snap.h>

#include <base/assert.h>

#include <math.h>
#include <stdbool.h>

#define TWO_PI 6.28318530717958647692
#define PI 3.14159265358979323846

// Which world axes a handle names, x, y and z.
typedef struct {
	bool x, y, z;
} named_axes;

static named_axes axes_of(voe_3d_gizmo_handle handle)
{
	VOE_BASE_ASSERT(handle > VOE_3D_GIZMO_NONE &&
				handle <= VOE_3D_GIZMO_CENTRE,
			"a snap asked of no handle");

	const named_axes axes = {
		.x = handle == VOE_3D_GIZMO_X || handle == VOE_3D_GIZMO_XY ||
		     handle == VOE_3D_GIZMO_ZX || handle == VOE_3D_GIZMO_CENTRE,
		.y = handle == VOE_3D_GIZMO_Y || handle == VOE_3D_GIZMO_XY ||
		     handle == VOE_3D_GIZMO_YZ || handle == VOE_3D_GIZMO_CENTRE,
		.z = handle == VOE_3D_GIZMO_Z || handle == VOE_3D_GIZMO_YZ ||
		     handle == VOE_3D_GIZMO_ZX || handle == VOE_3D_GIZMO_CENTRE,
	};
	VOE_BASE_ASSERT(axes.x || axes.y || axes.z, "a handle naming no axis");
	return axes;
}

static void assert_step(double step)
{
	VOE_BASE_ASSERT(isfinite(step), "a snap step of no finite size");
	VOE_BASE_ASSERT(step > 0.0, "a snap step at or below nought");
}

static double nearest(double value, double step)
{
	return round(value / step) * step;
}

voe_math_double3 voe_3d_gizmo_snap_position(voe_math_double3 position,
					    voe_3d_gizmo_handle handle,
					    double step)
{
	assert_step(step);
	VOE_BASE_ASSERT(isfinite(position.x) && isfinite(position.y) &&
				isfinite(position.z),
			"a position of no finite place");

	const named_axes axes = axes_of(handle);
	const voe_math_double3 snapped = {
		.x = axes.x ? nearest(position.x, step) : position.x,
		.y = axes.y ? nearest(position.y, step) : position.y,
		.z = axes.z ? nearest(position.z, step) : position.z,
	};
	VOE_BASE_ASSERT(isfinite(snapped.x) && isfinite(snapped.y) &&
				isfinite(snapped.z),
			"a snapped position of no finite place");
	return snapped;
}

float voe_3d_gizmo_snap_angle(float angle, float step)
{
	assert_step(step);
	VOE_BASE_ASSERT(isfinite(angle), "an angle of no finite size");

	// remainder answers [−π, π]; −π is the same turn as π, which the
	// interval keeps.
	double wrapped = remainder((double)angle, TWO_PI);
	if (wrapped <= -PI)
		wrapped += TWO_PI;

	const float snapped = (float)nearest(wrapped, step);
	VOE_BASE_ASSERT(isfinite(snapped), "a snapped angle of no finite size");
	return snapped;
}

// One component to the nearest step, its size at least one step, its sign
// kept: nought counts as positive.
static float scale_component(float value, double step)
{
	const double size = fmax(nearest(fabs((double)value), step), step);
	return (float)copysign(size, (double)value);
}

voe_math_float3 voe_3d_gizmo_snap_scale(voe_math_float3 scale,
					voe_3d_gizmo_handle handle, float step)
{
	assert_step(step);
	VOE_BASE_ASSERT(isfinite(scale.x) && isfinite(scale.y) &&
				isfinite(scale.z),
			"a scale of no finite size");

	const named_axes axes = axes_of(handle);
	const voe_math_float3 snapped = {
		.x = axes.x ? scale_component(scale.x, step) : scale.x,
		.y = axes.y ? scale_component(scale.y, step) : scale.y,
		.z = axes.z ? scale_component(scale.z, step) : scale.z,
	};
	VOE_BASE_ASSERT((!axes.x || snapped.x != 0.0f) &&
				(!axes.y || snapped.y != 0.0f) &&
				(!axes.z || snapped.z != 0.0f),
			"a snapped scale reaching nought");
	return snapped;
}
