# 0348 — A light blocker is a Wall, Indoors or a Room
date: 2026-10-04
by: tech-lead

## Decision
Light comes in two kinds. **Direct light** has a source and travels in straight lines: a directional light's
own light, a point light, later a spot light, and bounce light. **Fill** has no source; it is the sky's glow
of a directional light, there or not there by region. Every light blocker stops direct light that would pass
through it: a point gets none of a light whose line to it crosses a blocker the light is outside of, so a
blocker in open air leaves a shadow-shaped patch, with fill in it as in a real shadow. A blocker has a
**Kind**, chosen in the editor and saved with it:
1. **Wall** — a slab that stops direct light and nothing more. Fill passes. Walls lining a house with windows
   and doors left open let light through the openings only, both ways: outside light in, inside light out.
2. **Indoors** — inside it, no fill from any directional light outside. Direct light still gets in where no
   Wall stops it (sun through a window lights a patch of floor, and that patch's bounce lights the room), and
   lights inside light the inside and leak out only through the openings. A directional light placed inside
   an Indoors box behaves as one outside.
3. **Room** — its own place, like 056 built: nothing from outside gets in, direct light, fill or bounce,
   whatever openings the scene has; lights inside, directional lights and their fill included, light only
   the inside and never get out. A directional light's place decides its side: inside a Room it lights that
   Room in its own direction; outside every Room it lights everything outside Rooms, stopped by Walls.
A new blocker is a Room, so a blocker saved during 056 keeps its look. With no blockers the picture is
unchanged. Blockers stay cheap: no shadow map, no mesh, no collider.

## Reasoning
- Walls also stopping the fill along the light's direction: the outside back of a house and the ground
  behind it would lose their fill and go darker than any real shadow.
- One kind only (056's Room): sun cannot come in through a window, and a room's lamp cannot glow out of one.
- A Room with openings instead of Indoors: openings would need their own shapes; Walls plus Indoors give
  the same house from boxes the human already places.
- Fill as a region and not a line: fill has no single direction to test, so in or out of a box is all
  that can be asked of it cheaply.

## Replaces
Amends 0347 points 3–4: the mask rule holds for a Room; Walls and Indoors are added beside it.
