# math

Vectors and matrices, named and laid out the way Slang names and lays them out,
so that a value here and a value in a shader are the same thing. Pure data used
inline; no systems here, because nothing here owns an identity.

- `include` — the public headers, in `include/math/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/math/float2.h` — a two-component vector, and the operations on it.
- `include/math/float3.h` — a three-component vector; the only one with a cross
  product.
- `include/math/float4.h` — a four-component vector; the one a `float4x4`
  multiplies.
- `include/math/float4x4.h` — a row-major 4x4 matrix. Its header carries the
  layout rule, the multiplication convention, and the slangc flag they cost.
- `include/math/quat.h` — a unit quaternion, the engine's rotation type, with a
  length and a normalise that makes one out of anything that is not. Its header
  says why its name is not Slang's, why only what is called for is here, and
  which way round composing two of them reads.
