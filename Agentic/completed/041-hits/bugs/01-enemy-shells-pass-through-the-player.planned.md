# 01 — Enemy shells pass through the player

## Seen
"The enemies are shooting at me. But their projectiles are passing through, not colliding. So I am
not losing any lives."

## Expected
An enemy shell that reaches the player's tank stops on it, and the tank loses a life; the HUD's
`Lives N` goes down by one (feature step 4).

## How to reproduce
1. Open the tank game in the editor and press Play.
2. Drive into an enemy's view and stand still until it fires at you.
3. Watch the shell reach your tank: it flies straight through, and the lives count does not change.
