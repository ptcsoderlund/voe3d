claimed-by: claude-code (kanban-coder)
status: review

math needs vec2,vec3,mat4 and quat. 
Pure value types.
Do the same naming as slangc (shader language slang).  
To keep the math consistent between our engine and gpu.

## Decided during the card

Asked, and answered by Human:

- **The card's type names are examples, not names.** "I named examples, dont use
  their naming. Keep it slangc." So the types are `voe_math_float2`,
  `voe_math_float3`, `voe_math_float4`, `voe_math_float4x4` — not vec2/vec3/mat4.
- **Row-major.** "Slangc is row-major. Except for when running in slangc for
  legacy reasons. Lets go with row-major."
- **No quaternion.** "we wait, dont do anything we dont have use for. We do it
  when we need it." The card names quat; Human cut it. Not unfinished — removed.

Taken without asking, both cheap to reverse:

- `float4` was added. The card's list was examples, and a `float4x4` has nothing
  to multiply without it.
- **No projection or look-at matrices.** They encode a clip-space convention —
  depth range, Y direction — which is a Vulkan fact, and `math` must not know
  Vulkan. `float4x4` carries the algebra and TRS only.

## What was built

    math/include/math/float2.h  float3.h  float4.h  float4x4.h
    math/src/float2.c  float3.c  float4.c  float4x4.c

Each vector type carries the same 13 operations — add, sub, mul, div, scale,
neg, dot, length, normalize, lerp, min, max, clamp — with `float3` adding cross.
Component-wise `_mul` is Slang's `a * b`, not a dot product; the scalar form is
`_scale`. `float4x4` has identity, from_translation, from_scale, mul,
mul_float4, transform_point, transform_dir, transpose, determinant, inverse.

`math/include/math/version.h` and `math/src/version.c` were deleted. Their own
header comments said "Delete when the first real file lands."

## Verified

`cmake -P check.cmake` fails at step 1 on this machine: no `slangc` on PATH.
Everything else the script tests is present — clang 22.1.8, cmake 4.3.0, ninja
1.13.2, and gcc, so the compiler guard fires rather than skipping.

Re-run from a copy of `check.cmake` at the repository root with exactly one line
changed — the `slangc -v` probe replaced by `cmake -E true`. `diff` confirmed
that was the only edit; the copy was deleted afterwards and the real
`check.cmake` was never touched. All nine steps `ok`, exit zero:

    ok    tools (clang 22, cmake 4.3.0, slangc)   <- probe substituted
    ok    standalone base
    ok    standalone math
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (none yet)

`math` also configures and builds standalone on its own, clean under
`-Wall -Wextra -Wpedantic -Werror` at C23.

Behaviour was checked numerically, not by eye. A throwaway harness compiled
against `libvoe_math.a` outside the repository — the test mechanism is undecided
per CLAUDE.md, and spikes do not live here — ran 20 assertions, all passing:

- `M * inverse(M) = I` and `inverse(M) * M = I` on a general, non-affine matrix.
  This is what proves the cofactor expansion, rather than assuming it.
- A hand-written row-major `Rz` = `{{0,-1,0,0},{1,0,0,0},{0,0,1,0},{0,0,0,1}}`
  sends x to y, so `m[row][col]` here means what `m[row][col]` means in Slang.
- `from_translation` puts translation at `m[row][3]`, so `mul_float4(M, v)` is
  Slang's `mul(M, v)` on a column vector.
- `mul(T, S)` applies `S` first — composition reads right to left.
- `transform_point` takes the translation, `transform_dir` does not, and the two
  agree with `mul_float4` at w = 1.
- cross is right-handed; normalize, lerp, clamp, dot, length, determinant and
  transpose all check out.

Markers left in the code: none. No `DEVIATION:`, no `BLOCKED:`.

## For the reviewer

- **Git is unusable in this checkout, so this card was moved with `mv`, not
  `git mv`.** `.git` is a gitlink file reading `gitdir: ../.git/modules/voe3d`,
  and `/home/ptcsoderlund/Projekt/voe3d_pre_study/.git/` does not exist. Every
  git command fails with "not a git repository". Nothing was committed. This
  predates the card and is unrelated to it.
- **A later card owes slangc `-matrix-layout-row-major`.** Row-major is Slang's
  language default but not slangc's, which is column-major for legacy reasons.
  Without the flag the engine and the shader disagree silently: nothing fails to
  compile, transforms just come out transposed. Whichever folder invokes slangc
  owns this. It is noted in `float4x4.h` so it cannot be lost.
- Slang's own docs could not be reached to confirm the quaternion question —
  `shader-slang.org`, `docs.shader-slang.org` and `raw.githubusercontent.com`
  are all blocked by this machine's egress proxy.

## Suggestions, not done here

- If "dont do anything we dont have use for" applies to operations as well as
  types, some of the 13 per-vector operations may not earn their place yet —
  `div`, `min`, `max` and `clamp` have no caller. Say the word and they go.
- Projection and look-at want a card of their own, after clip-space convention
  is decided.
