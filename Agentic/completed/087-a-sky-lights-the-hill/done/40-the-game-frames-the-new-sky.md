# 40 — The game frames the new sky
folder: game
after: 39
decisions: 0168, 0411, 0412

## Change
The sky row lost `sun_size`, `sun_glow` and `sunset` and gained `strength` (card 36); a light row
gained `disc_size` and `glow_size` (card 37). Read the header of `game/tests/frame.c` and its sky
row near line 517.

- `game/tests/frame.c`: the sky row it adds carries `strength` 1 in place of the three gone
  fields; any header line naming them follows. Nothing else changes.
- `game/tests/tests.md` (`frame.c`): changed only if it names a gone field, one sentence of at
  most 300 characters.

## Done when
The test `game/frame` passes after the folder's build.

Human, in the editor on lumberhill_project, `feature.md`'s How to test steps 1 to 12. In short:
add a sky; changing its colours or height leaves the lit hill unchanged; strength 0 darkens the
sky, the disc stays; an orange light gives an orange disc and glow at any strength, disc and glow
size 0 hide them; a second light adds a second disc; fill lifts only shade and removing the sky
leaves the hill lit the same; far ridges haze and darken with the sky; a metal sphere mirrors the
sky and the disc; exposure and Soft tone work; the tank game is unchanged; the sky is a small
share of the frame breakdown; save, reopen, Play and ship agree.
