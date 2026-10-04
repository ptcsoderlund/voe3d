# 21 — Direct light stops at a Wall, and fill at Indoors
folder: render
after: 19
decisions: 0168, 0348, 0350

## Change
draw.slang's lit path gates by 0350 points 3 and 4; the bounce read keeps card 22's work. Read the
headers of `render/shaders/blockers.slangh`, `render/shaders/lighting.slangh`,
`render/shaders/bindings.slangh` and `render/shaders/draw.slang`, the light blocker section of
`render/include/render/device.h`, and `render/tests/blocked_light.c` with the helpers it uses.

- `render/shaders/blockers.slangh`:
  - `bool voe_render_blocker_meets<L>(L list, uint i, float3 from, float3 to)`: whether the segment
    meets blocker i: the sphere's distance to the segment first, then slab clipping of the segment
    in the box's unit space (rows), the face counted as met.
  - `bool voe_render_blockers_stop<L>(L list, uint stoppers, uint source, float3 p, float3 s)`:
    whether any blocker in `stoppers` (walls and Rooms) and not in `source` meets p→s.
  - `bool voe_render_blockers_pass<L>(L list, uint walls, uint indoors, uint source, uint
    receiver, float3 p, float3 s)`: 0350 point 3, Room bits equal and not stopped.
  - The sun's ray as a segment ending far past every blocker (p minus the direction times a length
    the header names and justifies). Header points: the three tests, their cost, the twin note now
    only for the mask.
- `render/shaders/lighting.slangh`: the sun's direct term, its shadow included, by
  `_blockers_pass` with the region's `sun` as source and the sun's ray; the fill by 0350 point 4,
  its fade reading the gated reach; a point light's term by `_blockers_pass` with its pass-begin
  mask as source and its position. The water path unchanged. The blockers paragraph says this.
- `render/shaders/draw.slang`: only what the lit path needs handed; header line if it changes.
- `render/tests/blocker_kinds.c` (new), modelled on `render/tests/blocked_light.c`, a ground quad
  seen from above with a low sun from +X and a fill:
  1. a Wall box floating 2 m above the ground: the ground in its shadow along the sun reads the
     fill alone (not black, below sunlit), the ground just under the box on the sun's side reads
     sunlit, and the ground well clear reads the no-blocker picture;
  2. under a dark sun, a point light beside a Wall standing on the ground: ground behind the Wall
     reads none of its colour, ground beside it does;
  3. an Indoors box over the left half: under the sun it is sunlit as without it; with the sun's
     intensity 0 it reads black where outside reads the fill;
  4. a Room box over the left half with `sun` naming it: the left lit, the right black, fill
     included;
  5. the three words zero over the same Room box: blocked_light's case 1 picture.
- `render/tests/blocked_light.c`: where a case now reads a pixel the box's crossing changes (its
  sun or lamp ray meets the box), the sample or the light is moved so it does not, and the header
  says why; its claims otherwise stand.
- `render/shaders/shaders.md`, `render/tests/tests.md`: the entries for what changed and the new
  test. Each at most 300 characters.

## Done when
The test `render/blocker_kinds` passes, and `render/blocked_light`, `render/point_lights`,
`render/shadow` and `render/bounce_read` still pass, after the folder's build.
