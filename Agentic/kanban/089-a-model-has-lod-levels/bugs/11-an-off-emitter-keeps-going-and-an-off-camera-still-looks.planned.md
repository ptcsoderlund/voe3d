# 11 — An off emitter keeps going, and an off camera still looks

## Seen
"any entity which is switched off should count as not present."
Two things still act as if a switched-off entity were there:
- An emitter switched off is not drawn, but it keeps spawning and moving its particles unseen. Switched back
  on, its particles are already in flight instead of starting as a new emitter's would.
- The camera switched off still frames Play and a shipped game exactly as before.

## Expected
Decision 0427: an entity switched off is as if it were not there; its parent being off does the same.
- Particles: an off emitter is as if it were removed. It spawns nothing, its particles are gone, and nothing
  is simulated while it is off. Switched back on, it starts as a newly added emitter would.
  The switch has one job, to make the entity act as if deleted; it keeps nothing warm. Starting an emitter
  already warm is the emitter's own feature, a setting on its component, and is not part of this bug (the
  emitter has no such setting today). The human's words: "keeping particle systems warm is a feature in
  itself in the component. Single responsibility, the off switch is to make stuff act as if deleted."
- Camera, the one exception to "as if removed": a world with no camera on is drawn as if it had a camera
  with the identity transform and default settings, instead of failing. The human's words: "an off camera
  would mean that it doesnt exist. So we just render view as if a camera was with transform identity
  (default)."
Saved, undone, and held in Play and Ship, as the other switches are.

## How to reproduce
1. Open a project with an emitter and press Play.
2. Switch the emitter's entity off, wait a few seconds, switch it on: particles are already spread out.
3. Switch the camera's entity off: Play still looks through it.

## How to test
1. Switch an emitter's entity off: its particles vanish at once. Wait, switch it on: it starts from nothing,
   as a fresh emitter does.
2. Put the emitter on a child and switch the parent off: the same.
3. Switch the camera's entity off: Play and Ship look from the origin with the default camera, along the
   default facing. On: back to the camera as placed.
4. Put the camera on a child and switch the parent off: the same as step 3.
5. Ctrl+Z undoes the switch; save, reopen and Play show it as left.
