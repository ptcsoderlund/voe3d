# 014 — vertex and index buffers, and a cube

status: review
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

## Notes

**Done and verified by Human on 2026-09-03.** `cmake -P check.cmake` exits zero
end to end — tools, all five folders standalone, root build, the three guards,
includes, 8 tests passed, the harness's own failure check, the analyser over 28
files, and the analyser's own finding check. The cube renders.

The machine this was written on could not run any of that: it had `clang` and
`cmake` and the Vulkan loader, and no `ninja`, `slangc`, `wayland-scanner` or
Wayland development package. What was checked here before handing over was the C
only — every `render/src/*.c` and `render/tests/*.c` compiling under
`-std=c23 -Wall -Wextra -Wpedantic -Werror` with a four-byte stub standing in for
the two `.spv` files, `clang --analyze` silent on all of them with no suppression
added, and step 5's includes rule re-implemented and run over `render`. Neither
shader had been through `slangc` and nothing had run on a graphics card; both are
now covered by the run above.

**No textures, and that is this card's decision rather than an omission.** See *No
texture coordinates* above: the cube is 8 vertices because nothing samples
anything yet. Card 017 is where UVs arrive and it says so in its own scope — the
cube becomes 24 vertices there, and 014 is its `blocked-by`, now resolved.

Decisions taken inside the card, each argued at its site:

- Matrices go in a **host-visible uniform buffer per frame slot** with a
  descriptor set per slot, which is the shape `device_internal.h` already
  predicted. Three `float4x4` is 192 bytes and would not fit push constants'
  guaranteed 128.
- The **triangle is gone** and `tests/offscreen.c`'s culling proof was re-expressed
  on the cube, on Human's instruction. Its old claim — mirrored means an empty
  image — cannot survive a cube, which is not empty when mirrored but inside out.
  The new claim is one colour channel at the centre pixel: every corner of the
  cube's +Z face has blue at 1 and every corner of the -Z face has blue at 0, so
  the centre reads 255 drawn the engine's way and 0 drawn mirrored.
- **The depth test's correctness is checked by the cube being drawn at all.** With
  a clear of 0 and `GREATER` every fragment passes; both ways of getting it wrong
  — `LESS` against 0, or `GREATER` against a clear of 1 — reject every fragment
  rather than sorting wrongly. That makes the card's "flip the comparison and
  confirm it looks wrong" an assertion rather than a thing to look at, and it is
  the reason a convex cube needs no second shape to prove depth.
- The **projection is checked on the CPU** in `tests/matrix.c`, with no driver:
  near plane at 1, far approached at 0, nearer is greater, no negated Y, and
  `m[2][2] == 0` for the infinite far plane. That half runs on a build box with
  no graphics card and is the half that catches someone "correcting" reverse-Z.
- The matrix probe is a **second shader and a pipeline built only when a test asks
  for it**, so a shipping device pays for no pipeline it never draws. Its `.spv`
  is in the binary regardless, because `--embed-dir` is private to the folder's
  library and a test cannot `#embed`.
- `voe_render_cube_projection` has no `[[nodiscard]]`: rule 13 attaches it to
  functions that can fail, and `math` declares its pure value functions without.

Markers left in the code: **none.** No `DEVIATION:` and no `BLOCKED:` inside
`render`.

Reported rather than fixed, because each is another folder or another card:

- `BLOCKED: dev` — `dev/src/main.c` and `dev/dev.md` still say the program draws
  a triangle, and its window title is `"voe3d — a triangle"`. `dev` needs no code
  change to show the cube, since it only calls `voe_render_device_frame`, but
  those three strings are now wrong. A one-line card in `dev` fixes it.
- `BLOCKED: testing` — `voe::testing` has no near-integer check, so
  `tests/matrix.c` compares byte values through `VOE_TEST_CHECK_FLOAT` with a
  cast. It reads correctly; a `VOE_TEST_CHECK_INT_NEAR` would read better and is
  `testing`'s to add.
- Not this card: the whole worktree shows as modified with **40156 insertions and
  40156 deletions across 88 files, and `git diff --ignore-all-space` is empty** —
  a CRLF/LF flip that was there before this card started. It means a
  `git commit -a` here would sweep in every file in the repository.
