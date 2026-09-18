# math

`math`'s public headers: the vector and matrix types, named and laid out the way
Slang names and lays them out. Pure data used inline; nothing here owns an
identity.

- `float2.h` — a two-component vector, and the operations on it.
- `float3.h` — a three-component vector; the only one with a cross product.
- `float4.h` — a four-component vector; the one a `float4x4` multiplies.
- `float4x4.h` — a row-major 4x4 matrix. Its header carries the layout rule, the
  multiplication convention, and the slangc flag they cost.
- `quat.h` — a unit quaternion, the engine's rotation type, with a length and a
  normalise that makes one out of anything that is not. Its header says why its
  name is not Slang's, why only what is called for is here, and which way round
  composing two of them reads.
