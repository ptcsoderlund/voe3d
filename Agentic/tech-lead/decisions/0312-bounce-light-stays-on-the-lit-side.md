# 0312 — Bounce light stays on the lit side
date: 2026-10-01
by: tech-lead

## Decision
For 046 bug 03. A lit surface bounces light only out of its lit side, into the half-space its lit face looks
at. Nothing behind it, under it or in its shadow is brightened by its bounce: the ground in a box's sun
shadow, right at the box's foot, reads no brighter with the bounce on than with it off (within 2/255 in every
channel), at sun 1 and at π. A shadow may still be lit by other surfaces that face into it and are themselves
in sun (0307's "lit softly by what is around it"), but never by the face that casts it. 0309 is withdrawn: no
VPL is moved behind its surface. 0307's "no visibility" is relaxed to "no ray tracing, no distance fields, no
bake": any cheap means that keeps bounce off the far side of a surface is allowed, and the planner chooses it.
0310's tint, its proof on a hardware card through the editor's scene-view path with `main.scene` loaded from
the file, and 0311's single gain all stand. The proof of this decision is measured the same way, and it must
fail on the build before the fix: a test that passes on the leaking build proves nothing.

## Reasoning
The sponsor sees the bounce brighten the shadow beside a lit box, which is the opposite of bounce light and
makes shadows read washed out. 0309 shone each VPL from 1 m behind its face, which puts light behind the face
by construction, and 0311's raised gain made the leak visible.
- Keep 0309 and lower the gain: loses 0310's visible tint.
- Ray-traced or SDF visibility: ruled out by 0307 for good.
- Accept the leak: the sponsor rejected it.

## Replaces
0309. Amends 0307 ("no visibility") and 0308 point 4.
