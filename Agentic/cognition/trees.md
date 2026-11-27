# Trees

Where the hill's trees come from, what the withdrawn tree maker leaves undecided, and how their leaves behave.

## Where they come from
- The sponsor gave up the engine's tree maker as never good enough: "Its not good and it will never be good. It's better to do blender with procedural material(s) and bake into pbr"; see 0520. [intent · tech-lead · 2026-11-27]
- Off-standard tools are welcome when they are fun: "we can do stuff thats not following standards just for the fun of creativity". [intent · tech-lead · 2026-11-20]
- The sponsor does not know the industry standard for such tools and wants told what would actually work and what code can solve. [intent · tech-lead · 2026-11-20]

## What the tree maker leaves
- Per-branch wind and chopping into logs lived only on the grown skeleton; with Blender trees, where they come from is undecided until a game asks for them; see 0520. [open · tech-lead · 2026-11-27]

## Its leaves
- A leaf's alpha cuts out only when its `.material` has Cutout above 0; the default 0 is opaque, which is what made the alpha look solid: "cutout fixes it". [confirmed · tech-lead · 2026-11-24]
- Sun and point-light shadows have no fragment stage, so a cut-out leaf card would cast its whole quad as shadow; read by the scout, not seen on screen yet. [open · tech-lead · 2026-11-27]
