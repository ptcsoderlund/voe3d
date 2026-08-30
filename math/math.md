# math

Vectors and matrices, named and laid out the way Slang names and lays them out,
so that a value here and a value in a shader are the same thing. Pure data used
inline; no systems here, because nothing here owns an identity.

- `include/math/float2.h` — a two-component vector, and the operations on it.
- `include/math/float3.h` — a three-component vector; the only one with a cross
  product.
- `include/math/float4.h` — a four-component vector; the one a `float4x4`
  multiplies.
- `include/math/float4x4.h` — a row-major 4x4 matrix. Its header carries the
  layout rule, the multiplication convention, and the slangc flag they cost.
- `src/float2.c` — the float2 operations.
- `src/float3.c` — the float3 operations.
- `src/float4.c` — the float4 operations.
- `src/float4x4.c` — the float4x4 operations, including the inverse.
