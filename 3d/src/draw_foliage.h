// The foliage's groups (0442 point 5), and their draws. Internal to 3d:
// draw_foliage.c defines voe_3d_draw_system_foliage, declared in
// 3d/draw_system.h, finds each copy's group by the key below, and draws the
// groups it gathered into the camera pass (draw_system.c), each cascade
// (draw_shadows.c) and each bounce sun map (draw_bounce.c).
//
//     struct voe_3d_foliage_key key = { .kind = entry, .level = 1, .part = 0,
//                                       .rest = false,
//                                       .tier = VOE_3D_FOLIAGE_SEEN,
//                                       .casts = true };
//     (void)voe_3d_draw_foliage_solid(device, &frame);
//
// A GROUP IS ONE (kind, level, part, rest, tier, casts): the kind's loaded
// `.glb` entry, so two landscape rows naming one file share its groups; the
// level its copies are drawn at; the level-0 part they wear; whether they are a
// band's level-before copies, dithered the other way (0432 point 3); their
// tier; and whether the row's look of the kind casts. Equal keys are one group,
// whichever row and copy, hand-painted or grown, filled it.
//
// A SHADOWLESS KIND (0476 point 6) is gathered seen or not at all: its copies
// are kept out of the cast and unseen tiers because nothing but the shadow
// passes would draw them there, and its groups' `casts` keeps the seen ones out
// of the cascades and the bounce sun maps. They are still shadowed: the camera
// draws them lit as any copy.
//
// A COPY IS SORTED ONCE A VIEW, NOT PER PASS (0464 point 4): seen, cast or
// unseen (3d/draw_system.h). An instanced group cannot skip copies, and a
// gather per cascade would write each copy five times; a tier writes it once.
// The camera draws seen groups, the cascades seen and cast ones, and the
// bounce sun maps every tier: a sun map covers the level, not the view, so the
// bounce's picture is what it was before anything was culled.
//
// ONE DRAW PER GROUP, in every pass that draws foliage: its part's record and
// the group's instances, so a forest of one kind and two parts is two draws a
// pass however many copies stand in it (0375 point 3).
//
// A BAND'S `rest` GROUPS DO NOT CAST. A shadow is drawn at the level the
// camera chooses (0425 point 5), and the depth-only pass reads no dither, so
// the level before would cast a second, coarser copy of each dissolving tree.
//
// FOLIAGE IS IN THE BOUNCE SUN MAPS, NOT ITS CAPTURES OR THE POINT SHADOWS
// (0450, amending 0442 point 5). A sun map is one view like a cascade, so the
// groups draw there as they are, and the forest floor is in its trees' shade
// when the probes are relit. Captures and point shadows spend the instance
// index on faces and probes, which render refuses an instanced draw in, and a
// draw per copy there is what instancing removed: the bounce sees the trees'
// shade, not the trees.
#pragma once

#include <3d/draw_system.h>
#include <3d/models.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

struct voe_3d_foliage_key {
	const voe_3d_model_entry *kind;
	uint32_t level;
	uint32_t part;
	bool rest;
	voe_3d_foliage_tier tier;
	bool casts;
};

// Every seen group of `frame`, `rest` ones too, drawn solid into the open
// camera pass inside one `foliage` span (0388), two-sided where its worn
// material is (0421 point 2). Nothing with none. False when render refuses a
// draw, which it has said on stderr and which stops the walk.
[[nodiscard]] bool voe_3d_draw_foliage_solid(voe_render_device *device,
					      const voe_3d_frame *frame);

// Every group of `frame` not `rest`, whose `casts` is true and whose worn
// material casts (voe_3d_draw_casts), seen or cast or, with `every_tier`, of
// any tier, drawn
// into the open shadow pass. With `counts`, each such group's instances drawn
// are added to its `cast` and those of the unseen ones left out to
// `cast_skipped`, a copy of a kind of several parts once a part; NULL counts
// nothing. False as _solid.
[[nodiscard]] bool voe_3d_draw_foliage_cast(voe_render_device *device,
					     const voe_3d_frame *frame,
					     bool every_tier,
					     voe_3d_view_counts *counts);
