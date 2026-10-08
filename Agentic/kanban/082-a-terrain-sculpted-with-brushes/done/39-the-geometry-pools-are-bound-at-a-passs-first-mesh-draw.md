# 39 — The geometry pools are bound at a pass's first mesh draw
folder: render/src
after: 38
decisions: 0168, 0385

## Change
Bug 04: the sponsor's Windows layer raises
`BestPractices-vkEndCommandBuffer-VtxIndexOutOfBounds`, because every pass
binds the static geometry pools as it opens, and a frame whose passes draw
only elements (the interface) ends with a vertex buffer bound that no draw
used. Per 0385 the pools are bound at a pass's first mesh draw instead. The
scratch layer cannot raise this id; the proof here is grep and the suite,
whose pixel tests draw meshes out of both pool pairs.

- `render/src/device_internal.h`: beside `bound_transient`, a
  `bool pools_bound` — whether the open pass has bound a pool pair yet;
  false as a pass opens; meaningless while `recording` is false. The
  `bound_transient` comment says it is read only once `pools_bound` is true.
- `render/src/pass.c`: where the pass opens it no longer calls
  `voe_render_bind_pools`; it sets `pools_bound` false. The "THE STATIC
  POOLS ARE BOUND HERE" comment becomes: no pool is bound as a pass opens,
  the first mesh draw binds the pair it needs, and why (a pass of elements
  only would leave a vertex buffer bound that nothing reads, bug 04).
- `render/src/draw.c`: `draw_with` binds when `pools_bound` is false or
  the pair differs from `bound_transient`. `voe_render_bind_pools` sets
  `pools_bound` true and becomes `static` here, since pass.c no longer
  calls it. Header's "WHICH PIPELINE IS BOUND, AND WHICH POOLS" and the
  comment above the bind function: a pass opens with the solid pipeline and
  no pools; the first mesh draw binds its pair.
- `render/src/frame_internal.h`: remove the `voe_render_bind_pools`
  declaration and its comment.
- `render/src/element.c`: the comment "THE VERTEX AND INDEX BUFFER
  BINDINGS SURVIVE THIS" says a mesh draw after an element draw finds
  whatever pair was bound, or binds one if none was.
- `render/src/frame.c`: the header's list of what is open between the
  calls names `pools_bound` beside `bound_transient`, set by pass.c and
  draw.c.

## Done when
- `! grep -n "voe_render_bind_pools" render/src/pass.c render/src/frame_internal.h`
  exits 0.
- `grep -n "pools_bound" render/src/pass.c render/src/draw.c` prints at
  least two lines.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
  prints FINDINGS: 0.
