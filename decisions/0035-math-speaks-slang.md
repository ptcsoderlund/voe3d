# 0035. `math` uses Slang's type names and Slang's memory layout

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human (type names, in the `math` card), Tech Lead (layout, ratifying the coder's finding)
- **Supersedes:** —
- **Amends:** ADR-0033 §2 — storage order only. The vector convention is unchanged.
- **Superseded by:** —

## Context

Two things arrived together and are one decision: **the CPU side of `math`
should be spelled and laid out the way the GPU side is**, because the same
values cross that boundary every frame.

The principal's `math` card fixed the first half: *"Do the same naming as slangc
(shader language slang). To keep the math consistent between our engine and
gpu."* So the types are `voe_math_float3`, `voe_math_float4x4` — not `vec3`,
not `mat4`. This contradicts the example in ADR-0014 (`voe_math_vec3`), which is
why it is written down: without an ADR, a later card "corrects" `float3` to
`vec3` and the reason is lost.

The second half came from the coder implementing that card, in the `float4x4.h`
header comment, and it is a correction to me. **ADR-0033 §2 said "column-vector,
column-major", which conflated two independent axes:**

- **Semantics** — is a vector a column or a row? Does translation live in the
  last column or the last row? Does `A * B` apply `B` first?
- **Storage** — given `m[i][j]`, is `i` the row or the column?

GLSL and glTF pair column-vector semantics with column-major storage, and I
wrote that pairing down because it is the one the literature uses. **Slang and
HLSL pair the same column-vector semantics with row-major source indexing**:
`m[row][column]` in Slang source means row `row`, column `column`. Our shader
language is Slang (ADR-0015), not GLSL.

## Options considered

### Option A — Column-major storage (ADR-0033 as written)
Matches glTF's file layout, so an imported matrix is a straight copy. Matches
the literature, so a transcribed formula needs no thought.

Costs: `m[i][j]` in C and `m[i][j]` in Slang pick out different elements, which
is a transposition bug that compiles, runs, and produces a plausibly wrong
image. Requires `slangc -matrix-layout-column-major` and requires every author
to hold two indexing conventions at once.

### Option B — Row-major storage, column-vector semantics
`m[row][column]` means the same thing in C and in Slang. Uploading a matrix to
the GPU is a straight `memcpy` of 16 floats. Semantics are unchanged from
ADR-0033: vectors are columns, `mul(M, v)` is `M·v`, translation is the last
column at `m[0][3]`, `m[1][3]`, `m[2][3]`, and composition reads right to left.

Costs: a glTF matrix must be transposed at import — 16 floats, once per matrix,
in one function. And `slangc` must be invoked with `-matrix-layout-row-major`,
because its default is the other one.

## Decision

**Option B, and Slang's type names throughout `math`.**

Deciding factor: matrices cross the CPU/GPU boundary every frame and glTF
matrices are imported once, so the layout should be free on the frequent path
and cost a transpose on the rare one — and more importantly, an index written in
C and the same index written in a shader must mean the same element, because the
failure mode when they don't is silent and looks like a maths bug.

Fixed:

1. **Type names are Slang's.** `voe_math_float2`, `float3`, `float4`,
   `float4x4`. Not `vec`/`mat`. This overrides the illustrative name in
   ADR-0014; that ADR's actual rule — spelled-out namespace, `voe_` prefix — is
   untouched.
2. **Operation names follow Slang's operators, not mathematical vocabulary.**
   `voe_math_float3_mul` is Slang's `a * b`, component-wise. It is **not** a dot
   product. Scalar multiplication is `_scale`. This is the sharpest edge in the
   decision and the header says so.
3. **Storage is row-major: `m[row][column]`.** Amends ADR-0033 §2.
4. **Semantics are unchanged from ADR-0033.** Column-vector, `M·v`, translation
   in the last column, composition right to left. Right-handed, Y-up, −Z
   forward, reverse-Z — all stand.
5. **`slangc` is invoked with `-matrix-layout-row-major`.** This closes D-040.
   The folder that invokes `slangc` owns the flag. Slang's default is the other
   layout, and getting it wrong fails nothing at compile time — it transposes
   every transform. **It must be proven by a test** that uploads a known matrix
   from C and checks it multiplies the same way in a shader; the flag being on
   the command line is not evidence that it took effect.
6. **The glTF importer transposes matrices.** glTF stores column-major. This is
   the one place the conversion exists, it is 16 floats, and — per ADR-0033 —
   it converts *layout only*. It is still forbidden to convert coordinates.
7. **`math` does not build projection matrices.** They encode a clip-space
   convention and `math` does not know about any graphics API. That belongs to
   whichever folder owns the camera.

## Blast radius

**Reversibility: moderate.** Renaming the types later is mechanical and wide.
Changing the storage order later is worse: it is invisible to the compiler, so
the only thing that catches a missed site is a test or a wrong-looking image.
Point 5's test is what makes this safe, and it does not exist yet.

## Consequences

- **Two conventions now disagree with the graphics literature**, in opposite
  directions: our storage matches HLSL/Slang while our handedness matches
  glTF/OpenGL. Anyone transcribing a formula must check both. The file headers
  in `math` carry this; it is exactly the kind of thing ADR-0034 would otherwise
  have left implicit.
- **`voe_math_float3_mul` will be mistaken for a dot product.** Accepted,
  because the alternative is a name that disagrees with the shader beside it.
  Mitigated by the header comment and by it being tested.
- **The transpose at glTF import is a place a bug can hide** — apply it twice
  and matrices come back wrong. One function, one test.
- **D-040 is answered before the folder that owns the flag exists.** Recorded
  against D-035 so the first `render` card inherits it.
- **ADR-0033 is now amended within a day of being written.** The error was
  conflating storage with semantics; the substance of ADR-0033 — handedness, up
  axis, reverse-Z, one Y flip — is unaffected.

## Rejected options and why

- **Option A** — the pairing the textbooks use, and wrong for a project whose
  shader language is Slang. It buys a free glTF import, which happens once, at
  the cost of a two-convention indexing rule on every matrix expression in the
  engine, forever.
- **Keeping `vec3`/`mat4` names while using Slang's layout** — half the benefit,
  and it leaves the CPU and GPU spellings different for no reason once the
  layouts already match.

## Questions this opens

- **Closes D-040.**
- **Nothing else.** It narrows D-035 by one more item.
