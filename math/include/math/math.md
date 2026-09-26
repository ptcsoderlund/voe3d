# math

The public headers, one entry each; the fuller account of every one of these
stays on `math/math.md`.

- `double3.h` — three doubles, for a world position; add, sub, widen and narrow.
- `float2.h` — a two-component vector, and the operations on it.
- `float3.h` — a three-component vector; the only one with a cross product.
- `float4.h` — a four-component vector; the one a `float4x4` multiplies.
- `float4x4.h` — a row-major 4x4 matrix, with the layout rule and multiplication
  convention it carries.
- `quat.h` — a unit quaternion, the engine's rotation type, with a length and a
  normalise that makes one out of anything that is not.
