# 0278 — `render` holds a thousand textures, frees what a reload replaces, and reads normal and emission maps
date: 2026-09-27
by: planner

## Decision
For 036's models (0277):

1. **1024 texture slots**, up from 64. A card whose per-stage or per-set sampled image or sampler
   limit is below that is refused at startup with a line naming the limit.
2. **Static geometry and shading records can be freed.** `voe_render_geometry_destroy` gives a
   static range back to both pools — free ranges, first fit, neighbours merged — and its slot,
   generation bumped; `voe_render_shading_destroy` gives a record's slot back, generation bumped.
   Both wait for the card to go idle, as a create does. A stale id returns false; a transient
   geometry asserts.
3. **The normal map is read with a tangent frame built per pixel** from the screen derivatives
   of the world position and the UV: no tangent attribute, no change to `voe_render_vertex`. glTF's
   convention: red along +u, green along −v (glTF's v runs down), blue along the normal, each
   channel mapped from 0..1 to −1..1. No normal map is the vertex normal as now.
4. **Emission is added**: the emissive factor times the emissive texture, added to a lit
   surface's colour after the sun and the fill, unshadowed. An unlit surface stays its base colour.

## Reasoning
A tank game's models each bring up to four pictures, so 64 slots is a dozen models; every desktop
card this engine targets reports far more than 1024. A re-export reloads a model many times in a
session, and without frees the pools fill; first fit over a sorted free list is the smallest
allocator that reuses a range. A derivative tangent frame needs no MikkTSpace code and no vertex
change, and is close enough to Blender's bake for a baked normal map on hard-surface models; a
tangent attribute is the later card if seams show. Rejected: a tangent attribute now (every vertex
builder in the engine changes); mipmaps (card 026 removed filtering on purpose).

## Replaces
nothing. Amends the "stored and not read" of card 018 for the normal and emissive maps.
