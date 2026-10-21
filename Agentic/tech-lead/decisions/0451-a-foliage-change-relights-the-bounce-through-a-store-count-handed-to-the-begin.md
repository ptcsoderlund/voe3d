# 0451 — A foliage change relights the bounce through a store count handed to the begin
date: 2026-10-21
by: planner

## Decision
For 090 bug 04, carrying out 0450.

1. **Render draws copies into a bounce sun map.** `voe_render_frame_draw_instanced` accepts a
   bounce shadow pass as it accepts a cascade; the point-shadow and capture passes still assert.
2. **The begin carries a shade count.** `voe_render_bounce_frame` gains `uint32_t shade`: the
   caller's count of changes to what its sun maps draw that no stale sphere marks. A volume whose
   begin's `shade` differs from the one it was last relit with relights, as a changed light does;
   it recaptures no probe. Zero, as every caller that names none gives, is the old behaviour.
3. **The model store counts.** The store keeps one count, raised whenever an entry is loaded,
   replaced or cleared, a foliage copy or kind changes, or copies are placed again over changed
   heights. The bounce hands it to every begin as `shade`, nought with no store.
4. Foliage is drawn into a sun's bounce map only when no blocker holds that sun, as in its
   cascades; the groups not `rest`, at the levels the view chose.

## Reasoning
- Erasing trees must relight the probes (bug 04, How to test 2), but foliage is not a caster the
  stale walk marks, and recapturing probes would redraw pictures that never hold foliage.
- A stale sphere per changed copy: the bounce would need each change's place, kept per target;
  a count needs only one number and a relight costs what a turned sun costs.
- A count per landscape: the kinds' models load after the landscape, so a load must count too;
  one store-wide count covers both.

## Replaces
Nothing.
