# tests

One plain C program per `math` module, found by the build, each checking that
module's promises from outside.

- `float2.c` — the float2 operations, checked from outside.
- `float3.c` — the float3 operations, and that the cross product is
  right-handed.
- `float4.c` — the float4 operations, and that w counts in all of them.
- `float4x4.c` — the layout and the multiplication convention as two separate
  claims. Its header says which half of the layout rule a GPU has to prove
  instead.
- `quat.c` — which way a rotation turns and by how much, on all three axes,
  through the matrix it becomes; that composing two of them agrees with
  multiplying the matrices they become in the same order; and that normalising
  keeps the rotation rather than only the length.
