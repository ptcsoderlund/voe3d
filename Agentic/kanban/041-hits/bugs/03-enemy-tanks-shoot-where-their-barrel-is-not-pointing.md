# 03 — Enemy tanks shoot where their barrel is not pointing

## Seen
"Enemy tanks don't rotate. They just shoot in any direction against my tank. But their barrel is
always facing forward."

## Expected
An enemy aims before it fires. Its turret swings toward the player's tank at a readable speed, not in
a snap, so the player can see it coming. It fires only once its barrel points at the player. The
shell leaves from the muzzle and flies along the barrel. A shell never leaves in a direction the
barrel is not pointing.

## How to reproduce
1. Open the tank game in the editor and press Play.
2. Drive to the side of or behind an enemy tank, and let it shoot at you.
3. Watch the enemy: its barrel stays facing forward, but its shells fly sideways or backwards
   toward you.
