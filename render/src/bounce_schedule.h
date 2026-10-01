// Which bounce probes one update refreshes, and how much (ADR-0308 point 5).
// Pure CPU, no Vulkan: a target keeps one schedule, zeroed before its first
// update, and asks it for a list every update.
//
//     voe_render_bounce_schedule schedule = { 0 };
//     uint32_t probes[VOE_RENDER_BOUNCE_PROBES_ALL];
//     float blends[VOE_RENDER_BOUNCE_PROBES_ALL];
//     uint32_t n = voe_render_bounce_schedule_next(&schedule, cell, corner,
//                                                  &sun, stale, stale_count,
//                                                  probes, blends,
//                                                  VOE_RENDER_BOUNCE_PROBES_ALL);
//
// A PROBE INDEX IS TOROIDAL: x mod 32 + 32 × (y mod 32) + 1024 × (z mod 32) of
// its world cell, so a grid that scrolls keeps every probe it still covers
// where it was, and the cells it brings in take the slots of those it left.
// voe_render_bounce_wrap is that mod, the one wrap of a world cell into 0..31,
// negative cells included; the index and every cell sent to a shader use it.
//
// THE ORDER IS THE POINT. First every cell the move from the last call's lowest
// cell brings in, blend 1, whatever the budget — the first call, or a move of 32
// cells or more on any axis, is the whole grid. Then cells whose centre lies in
// a stale sphere, then a cycle over the grid with a stride coprime with 32³, so
// neighbours are refreshed far apart in time. Those two together stop at
// VOE_RENDER_BOUNCE_BUDGET and blend VOE_RENDER_BOUNCE_BLEND. A light record
// (direction, intensity, colour) unlike the last one restarts the cycle, so the
// next eight calls with nothing else to do cover the grid.
//
// No index is listed twice in a call, and nothing is written past `room`: a
// room smaller than the entered cells truncates them, and those left out are
// not listed again — `room` of VOE_RENDER_BOUNCE_PROBES_ALL never truncates.
//
// CONSTRAINTS. The entered cells are found by scanning the whole grid, 32³
// compares a call; a stale sphere scans only the cells its box covers. Slab
// arithmetic per axis would lift the first if a profile ever names it.
#pragma once

#include <render/device.h>

#include <stdint.h>

// Every probe of one grid, the room that never truncates a list.
#define VOE_RENDER_BOUNCE_PROBES_ALL                                  \
	(VOE_RENDER_BOUNCE_PROBES * VOE_RENDER_BOUNCE_PROBES *        \
	 VOE_RENDER_BOUNCE_PROBES)

// World cell `cell` mod VOE_RENDER_BOUNCE_PROBES, in 0..31 for a negative cell
// too: the one place a cell becomes a probe coordinate, so writer and reader
// cannot disagree.
static inline uint32_t voe_render_bounce_wrap(int64_t cell)
{
	const int64_t side = VOE_RENDER_BOUNCE_PROBES;

	return (uint32_t)(((cell % side) + side) % side);
}

// One target's schedule. Zeroed is "no update yet"; only
// voe_render_bounce_schedule_next writes it.
typedef struct voe_render_bounce_schedule {
	int32_t cell[3];
	bool has_cell;
	uint32_t cycle;
	voe_math_float3 direction;
	float intensity;
	voe_math_float3 colour;
} voe_render_bounce_schedule;

// `cell` is the grid's lowest world cell and `corner` its lowest corner about
// the eye; `stale` holds spheres about the eye, xyz centre and w radius. Writes
// probe indices and each one's blend; returns how many.
uint32_t voe_render_bounce_schedule_next(voe_render_bounce_schedule *s,
					 const int32_t cell[3],
					 voe_math_float3 corner,
					 const voe_render_light *sun,
					 const voe_math_float4 *stale,
					 uint32_t stale_count, uint32_t *probes,
					 float *blends, uint32_t room);
