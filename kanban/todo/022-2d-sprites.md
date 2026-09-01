# 022 — 2D and sprites

status: todo
claimed-by: -
blocked-by: 021

> **THIN AND PROVISIONAL.** Same caveat as card 021 — written far ahead at the
> principal's request so the backlog is visible. Expect to rewrite it.

## Goal

2D sprites, as billboarded quads in 3D space.

## The shape is already fixed

**Billboarding, not a flat drawing path** (ADR-0049 — the principal's own words:
*"2D games have to do billboarding sprites as textures"*). A sprite is a quad in
the world facing the camera. There is no orthographic screen-space renderer and
there will not be one.

## Rough scope

- A quad facing the camera. **Decide which billboarding**: full spherical (faces the
  camera entirely) or cylindrical (rotates about up only). They look different and
  the difference matters for anything standing on ground.
- Texture atlas or texture array for sprite sheets.
- **Batching**, because this is the first card where the object count is naturally
  large. *Instancing and batching* is on the *later* capability list — check
  whether this card is what promotes it.
- Depth and sorting. A billboard is subject to the depth buffer like everything
  else, and alpha-blended billboards need back-to-front ordering, which is a real
  cost and a real decision.

## Known unknowns

Whether a 2D game built on this wants a fixed pixel-to-world scale and where that
lives; whether sprite animation is a component or a texture-coordinate trick.
Neither is answerable yet.
