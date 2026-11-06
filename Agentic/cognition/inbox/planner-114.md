## Assumed
- The forest's cost is fragment overdraw of cut-out leaves under discard with depth write; 114 lays depth first rather than adding a counter to measure it first (0508).
- A lay-step draw counts in the Frame panel's draws but adds no triangles; triangles still mean what the picture holds.
- The prepass covers only seen foliage groups in the camera pass; cut-out models and landscapes keep the single draw.

## Learned
- Slang 2026.17 has no spelling found for a SPIR-V Invariant position: `invariant` is a syntax error, `[vk::invariant]` an unknown attribute, and `precise` on a matrix multiply emits no NoContraction.
- 0375 point 4's condition (draws named as the cost) is not met by 114: the forest is a few instanced draws a pass and shadows or LOD cuts do not move its time.
