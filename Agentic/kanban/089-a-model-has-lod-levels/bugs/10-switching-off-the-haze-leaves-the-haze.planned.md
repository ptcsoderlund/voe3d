# 10 — Switching off the haze leaves the haze

## Seen
"When i disable the haze component, its not disabled. With the new editor disable feature. so thats a bug."
The entity holding the Haze row is switched off with the Enabled switch, and the distance still hazes as before.

## Expected
Decision 0427: an entity switched off is as if it were not there. Its scene-wide rows go with it, not only the
things that are drawn: an off Haze row gives no haze, an off Sky row gives the old picture without a sky (as if
no Sky row existed), an off Picture row gives the default exposure and tone. Switching it back on brings each
back as it was. Its parent being off does the same. Saved, undone, and held in Play and Ship, as the other
switches are.

## How to reproduce
1. Open the hill project, which has a Haze row.
2. Select the entity with the Haze row and switch Enabled off.
3. The far hill and the sky are still hazed.

## How to test
1. Switch the Haze's entity off: the far hill is clear and the sky behind it unveiled. On: the haze is back.
2. Switch the Sky's entity off: the sky is gone, the picture is as with no Sky row. On: it is back.
3. Switch the Picture's entity off: exposure and tone go to the defaults. On: back as set.
4. If two of these rows share an entity, one switch takes both away.
5. Put the Haze row on a child, switch the parent off: no haze.
6. Ctrl+Z undoes the switch; save, reopen and Play show it as left.
