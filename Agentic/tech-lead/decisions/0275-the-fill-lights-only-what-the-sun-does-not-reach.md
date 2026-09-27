# 0275 — The fill lights only what the sun does not reach
date: 2026-09-27
by: tech-lead

## Decision
The sun's fill is no longer a flat term on every surface. It lifts only the shade: a surface in
full sun is lit by the sun alone, exactly as with no fill, and the fill fades in as the sun's
light on the surface falls away, whether from a shadow or from the surface turning away from
the sun. Where the sun does not reach at all, the surface gets the whole fill (fill colour ×
fill intensity × its own colour). The sun's colour is what colours the scene; the fill only
keeps the shade from going black. Everything else in 0273 holds: the fields, their defaults,
zero fill draws as before, `unshaded` reads none, the validation.

## Reasoning
The human does not want the fill to tint the lit parts of the scene; that is the sun's job.
Rejected: the flat ambient term of 0273 (tints and flattens everything), a sky/ground
hemisphere fill (more than asked; light bounce, 046, does it properly). Accepted cost: this
is not physical, so a surface at a shadow's edge can come out brighter than one in full sun
when the fill is strong, and light bounce will replace it.

## Replaces
Amends decision 0273 point 3.
