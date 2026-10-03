# 03 — A new enemy waits a second before it shoots

## Seen
"spawning enemies should wait at least 1 second before shooting."

An enemy tank can fire as soon as it appears, so the player is shot before they have had a chance to see
it arrive.

## Expected
Every enemy tank waits at least 1 second from the moment it appears before it fires its first shell. That
holds for an enemy that comes in a wave and for one that is in the level when it starts. During that
second it may turn and move as it does now; it only holds its fire. After the second, it shoots as it does
today. It behaves the same in the editor's Play and in the shipped game.

## How to reproduce
1. Open `examples/tank_game` and press Play, or run the shipped game, and start from the menu.
2. Stay where an enemy tank will appear, in its line of fire.
3. Watch it appear: it can shoot at once. It should not fire for the first second.
