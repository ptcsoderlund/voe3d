# 01 — Render builds the lay and shade pipelines
folder: render
after: none
decisions: 0168, 0508

## Change
Two more mesh pipelines for 0508's steps, built by prepare, bound by nothing yet (card 02 binds
them).

- `render/shaders/draw.slang`: factor the main fragment's whole keep-or-discard decision (the
  cut-out test with its mip-scaled coverage against the cutoff, and the 4×4 LOD dither) into one
  function; `voe_render_draw_fragment` calls it where it decides today, unchanged in effect. Add
  the entry `voe_render_depth_fragment`: the same inputs the main fragment takes, calls only that
  function, shades nothing. Header: twelve entry points; the keep test is one function both
  call, so lay and shade discard alike (0508 point 1).
- Position invariance: both new pipelines use the solid one's vertex entry
  (`voe_render_draw_vertex`) so the shade step's EQUAL meets the laid depth bit for bit. If Slang
  can decorate the position `Invariant` (check `slangc -target spirv-asm` output), do it on that
  entry; in planning, Slang 2026.17 refused `invariant` and `[vk::invariant]`. Say in the vertex
  entry's comment which holds.
- `render/src/pipeline.c`: `mesh_kind` gains `MESH_LAY` and `MESH_SHADE`; `create_pipeline`
  builds them from the one description: LAY = draw vertex + depth fragment, the camera pass's
  attachment formats, colour write mask 0, no blend, depth GREATER with write on, cull dynamic;
  SHADE = the solid one's two stages, no blend, depth compare EQUAL, write off, cull dynamic.
  Prepare builds both after the blended one; header paragraphs THERE ARE TEN and DYNAMIC become
  twelve and the cull in four.
- `render/src/device_internal.h`: `pipeline_lay` and `pipeline_shade` beside `pipeline_blended`,
  with a comment each.
- `render/src/device.c`: both destroyed at close with the others.
- `render/include/render/device.h`: `voe_render_device_prepare_steps`' comment counts ten, and
  thirteen with shaderOutputLayer, naming lay and shade.
- `render/tests/prepare.c`: any expected step count follows (+2).
- Maps: `render/render.md` and `render/src/src.md` (`pipeline.c` entry) say twelve pipelines with
  lay and shade; `render/shaders/shaders.md` `draw.slang` entry: twelve entry points, the depth one.

If validation or Best Practices warns about a fragment entry writing no colour under write mask 0,
have the depth entry return a colour; never add to the allowlist.

## Meaning
- Holds: the LOD dissolve must look exactly as now. Copies in a band are two groups with opposite
  dither; on a swap's own frame the incoming copy's dither is just above 0, below the pattern's
  1/32 floor, so it covers no pixel but still casts. The keep test must stay exactly this.
- Scout: leaves are cut by `discard` in the main fragment (`draw.slang` around 590–596): coverage
  scaled by mip level, dropped below the cutoff; the 4×4 dither also discards. No camera depth
  prepass exists; single-sample everywhere.

## Done when
- `ctest --test-dir build/debug -R '^render/prepare$'` exits 0 with the new count.
- `grep -q voe_render_depth_fragment render/src/pipeline.c` and
  `grep -q VK_COMPARE_OP_EQUAL render/src/pipeline.c` exit 0.
