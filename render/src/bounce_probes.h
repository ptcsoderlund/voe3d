// Which probes of a captured bounce grid are captured, and when the grid is
// relit (ADR-0326 points 2, 4 and 6). Pure CPU, no Vulkan: a target keeps one
// voe_render_bounce_probes, zeroed before its first place, and each frame
// places it, takes captures from it and asks whether to relight.
//
//     voe_render_bounce_probes probes = { 0 };
//     voe_render_bounce_lights lights;
//     uint32_t taken[VOE_RENDER_BOUNCE_CAPTURE];
//     voe_render_bounce_probes_place(&probes, cell, corner, spacing, stale,
//                                    stale_count, &sun, sun_bounces,
//                                    sun_strength, &more, &point_lights,
//                                    &blockers, &lights);
//     while ((n = voe_render_bounce_probes_take(&probes, taken, 16)) > 0) ...
//     if (voe_render_bounce_probes_relight_needed(&probes, &lights)) {
//             ... relight ...
//             voe_render_bounce_probes_relit(&probes, &lights);
//     }
//
// A PROBE INDEX IS TOROIDAL: each axis of its world cell wrapped into
// 0..size − 1, x + 24·y + 288·z, so a grid that scrolls keeps every probe it
// still covers where it was. voe_render_bounce_probe_wrap is the only place a
// world cell becomes a probe coordinate, negative cells included.
//
// A PLACE moves the grid to its new lowest cell, counted in `spacing`, the
// metres between its probes (ADR-0332 point 3). Probes it brings in (all on the
// first place, a jump of a whole grid on any axis, or a spacing other than the
// last) lose their picture, are queued, marked changed and not ready. A stale
// sphere of radius w queues the probes whose centre at that spacing lies within
// w + min(VOE_RENDER_BOUNCE_REACH × spacing / VOE_RENDER_BOUNCE_SPACING,
// VOE_RENDER_BOUNCE_UNSEEN_RADII × w): a probe's reach, but no further than
// where the caster is under one face texel (ADR-0389 point 7). Every fading
// probe gains 1 of readiness, up to VOE_RENDER_BOUNCE_FADE, and stops fading
// the place after it reached it. It writes the
// bouncing lights: the sun, every further sun with bounces and intensity, the
// first VOE_RENDER_BOUNCE_LAMPS point lights with bounces, and every blocker.
//
// A TAKE lists up to `room` queued probes, nearest the eye first (centre at
// the cell's middle at the placed spacing, ties to the lower index), unqueues
// them and marks them holding a picture and changed. One that held no picture
// starts fading at readiness 0; one that held one keeps its readiness.
//
// A RELIGHT IS NEEDED when any probe is changed or fading, or the bouncing
// lights differ from the last relit: field for field, but a lamp's position
// about the grid's world origin (corner − cell × spacing, about the eye) within
// a millimetre per axis and its shadow by whether it is slotted. An eye that
// moves shifts both alike, a grid that scrolls keeps the origin, and the point
// shadows reorder slots; none relights (ADR-0389 point 5). Blockers likewise:
// a different count relights, and so does a row's xyz or the sphere's radius
// beyond 1e-4, its centre about the origin beyond a millimetre, or row.w +
// row.xyz · origin beyond 1e-4. Walls, indoors, the sun's mask, or a further
// sun added, taken away or changed relight (ADR-0350, 0357).
//
// CONSTRAINTS. A place scans the whole grid twice, and once more per stale
// sphere; a take scans it once per probe taken, room × 6912 compares. Fine at
// 16 a pass; a per-axis box for spheres and a sort for takes would lift them.
#pragma once

#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// Every probe of one grid.
#define VOE_RENDER_BOUNCE_PROBES_TOTAL                                        \
	(VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_PROBES_Y *           \
	 VOE_RENDER_BOUNCE_PROBES_XZ)

// Places a new picture takes to reach full weight (ADR-0389 point 4).
#define VOE_RENDER_BOUNCE_FADE 16
// Past this many of its radii a caster is under one face texel (ADR-0389 7).
#define VOE_RENDER_BOUNCE_UNSEEN_RADII 16

// World cell `cell` mod `size`, in 0..size − 1 for a negative cell too.
static inline uint32_t voe_render_bounce_probe_wrap(int64_t cell, uint32_t size)
{
	const int64_t side = size;

	return (uint32_t)(((cell % side) + side) % side);
}

// The toroidal index of the probe at world cell (x, y, z).
static inline uint32_t voe_render_bounce_probe_index(int64_t x, int64_t y,
						     int64_t z)
{
	const uint32_t xz = VOE_RENDER_BOUNCE_PROBES_XZ;
	const uint32_t h = VOE_RENDER_BOUNCE_PROBES_Y;

	return voe_render_bounce_probe_wrap(x, xz) +
	       xz * voe_render_bounce_probe_wrap(y, h) +
	       xz * h * voe_render_bounce_probe_wrap(z, xz);
}

// One further sun that bounces: its light, bounces, strength and place mask.
typedef struct voe_render_bounce_sun {
	voe_render_light light;
	uint32_t bounces;
	float strength;
	uint32_t mask;
} voe_render_bounce_sun;

// The lights that bounce in one update. Unused lamp and sun slots are zero.
typedef struct voe_render_bounce_lights {
	voe_render_light sun;
	uint32_t sun_bounces;
	float sun_strength;
	uint32_t more_count;
	voe_render_bounce_sun more[VOE_RENDER_DIRECTIONAL_LIGHTS - 1];
	uint32_t lamp_count;
	voe_render_point_light lamps[VOE_RENDER_BOUNCE_LAMPS];
	uint32_t blocker_count;
	voe_render_light_blocker blockers[VOE_RENDER_LIGHT_BLOCKERS];
	// The blockers' kinds and the sun's mask (ADR-0350 point 2); `sun_mask`
	// because `sun` is the light above.
	uint32_t walls;
	uint32_t indoors;
	uint32_t sun_mask;
} voe_render_bounce_lights;

// One grid's state. Zeroed is "nothing yet"; only the calls below write it.
typedef struct voe_render_bounce_probes {
	int32_t cell[3];
	bool placed;
	voe_math_float3 corner;
	float spacing;
	uint32_t holds[VOE_RENDER_BOUNCE_PROBES_TOTAL / 32];
	uint32_t queued[VOE_RENDER_BOUNCE_PROBES_TOTAL / 32];
	uint32_t changed[VOE_RENDER_BOUNCE_PROBES_TOTAL / 32];
	uint32_t fading[VOE_RENDER_BOUNCE_PROBES_TOTAL / 32];
	uint8_t ready[VOE_RENDER_BOUNCE_PROBES_TOTAL];
	voe_render_bounce_lights relit;
	voe_math_float3 relit_origin;
} voe_render_bounce_probes;

// `cell` is the grid's new lowest world cell in `spacing` metres, finite and
// above nought, and `corner` its lowest corner about the eye; `stale` holds
// spheres about the eye, xyz centre and w the caster's own radius, to which
// the place adds the reach above; `blockers` are about the eye
// too, at most VOE_RENDER_LIGHT_BLOCKERS; `more` are the further suns, at most
// VOE_RENDER_DIRECTIONAL_LIGHTS − 1. Writes this update's bouncing lights, the
// blockers copied, into `lights`.
void voe_render_bounce_probes_place(voe_render_bounce_probes *p,
				    const int32_t cell[3], voe_math_float3 corner,
				    float spacing, const voe_math_float4 *stale,
				    uint32_t stale_count,
				    const voe_render_light *sun,
				    uint32_t sun_bounces, float sun_strength,
				    const voe_render_directional_lights *more,
				    const voe_render_point_lights *point_lights,
				    const voe_render_light_blockers *blockers,
				    voe_render_bounce_lights *lights);

// Writes up to `room` queued probe indices, nearest the eye first; returns how
// many, 0 when none is queued.
uint32_t voe_render_bounce_probes_take(voe_render_bounce_probes *p,
				       uint32_t *probes, uint32_t room);

// Whether `lights`, about the placed world origin, differ from those last relit
// about theirs.
bool voe_render_bounce_probes_lights_changed(const voe_render_bounce_probes *p,
					     const voe_render_bounce_lights *lights);

// Whether any probe is fading in.
bool voe_render_bounce_probes_fading(const voe_render_bounce_probes *p);

// Whether a probe changed or fades, or the lights changed.
bool voe_render_bounce_probes_relight_needed(const voe_render_bounce_probes *p,
					     const voe_render_bounce_lights *lights);

// Clears every changed mark and keeps `lights` and the placed world origin as
// the last relit.
void voe_render_bounce_probes_relit(voe_render_bounce_probes *p,
				    const voe_render_bounce_lights *lights);
