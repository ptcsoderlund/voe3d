# 014 — vertex and index buffers, and a cube

status: todo
claimed-by: -
blocked-by: 013

First real geometry. First buffer the engine uploads to the graphics card. First
depth buffer.

## Goal

A cube in the dev window, visibly solid — near faces hiding far ones.

## Scope

- **Vertex buffer, index buffer, and a staging upload.** Device-local memory, copied
  through a host-visible staging buffer. This is the pattern every later upload
  follows, so it matters more than the cube does.
- **A vertex input description**, and a vertex struct. Position and colour.
- **A depth target**, alongside the offscreen colour target from card 013 and
  **per frame slot** like it (ADR-0050).
- **Model, view and projection matrices**, sent to the shader. First matrices on the
  graphics card.

## No texture coordinates

The principal's decision. Coordinates nothing samples are code with no caller.

**Known consequence, so it is not a surprise later:** a cube needs **8 vertices
without UVs and 24 with them**, because a corner needs different coordinates per
face. The texture card rewrites this cube's data, and that is accepted — the value
of this card is the upload and depth plumbing, not the cube.

## The two things to get right

- **Reverse-Z, and it is not optional.** `D32_SFLOAT`, comparison `GREATER`, depth
  **cleared to 0**, range 0..1, and the default perspective has an infinite far
  plane (`CLAUDE.md`, ADR-0033). Cleared to 1 with `LESS` is the habit from every
  tutorial and it will look almost right.
- **The row-major test ADR-0035 asks for lands here.** This is the first card that
  sends a matrix to the graphics card, so it is the first that can prove
  `-matrix-layout-row-major` actually took effect rather than trusting the flag.
  Upload a known matrix, have the shader report back what it read.

## Verify

- The cube is solid from every angle — no face showing through another.
- **Flip the depth comparison on purpose and confirm it looks wrong.** A depth test
  that is silently inverted on a convex shape can look plausible.
- The matrix test fails if the `slangc` flag is removed.
- `check.cmake` zero; `render` standalone. Windows is the principal's.
