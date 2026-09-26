# 030 — Turn and zoom the coin game's camera

## What
The coin game's third-person camera becomes one I control, the way a 3D game's camera should be.
Only `examples/coin_game/` changes; `examples/capsule/` keeps its simple follow camera.

- **Turn:** while I hold the right mouse button, moving the mouse turns the camera around the
  capsule: sideways turns it all the way round, up and down tilts it between looking from
  almost straight above and from just above the ground. It never flips over. While I hold the
  button the pointer is hidden and does not wander off; when I let go it comes back where it was.
- **Zoom:** the scroll wheel moves the camera nearer to or farther from the capsule, between a
  nearest and a farthest distance.
- **Spring arm:** when a wall, the floor or anything else solid would come between the camera
  and the capsule, the camera pulls in so I still see the capsule, and goes back out to my
  chosen distance once the way is clear.
- **Controls follow the camera:** W runs away from the camera, S towards it, A and D to its
  left and right, whichever way I have turned it.
- The camera only turns and zooms while I play, not on the main menu, You win or Game over,
  where the mouse is for the buttons.
- How fast the mouse turns it, the nearest and farthest distance, and the distance it starts at
  are numbers I set in the Inspector. The planner decides which component carries them.

## Why
It is not a 3D game until I can look around. 029 works, but its camera only follows.

## How to test
1. Open `examples/coin_game/` in the editor and press Play. On the main menu, holding the right
   button and moving the mouse or scrolling does nothing to the camera.
2. Click Start. Hold the right button and move the mouse sideways: the camera goes round the
   capsule, as far as I like, and the pointer is hidden. Let go: the pointer is back.
3. Hold the right button and move the mouse up and down: the camera tilts between high above
   and low behind the capsule, and does not flip over at either end.
4. Scroll: the camera comes nearer and goes farther, and stops at a nearest and a farthest
   distance.
5. Turn the camera to look from the side and press W: the capsule runs away from the camera,
   not the way W ran before. A, S and D likewise go left, towards and right as seen on screen.
6. Run the capsule next to a wall and turn the camera so the wall would be between them: the
   camera comes in front of the wall and I still see the capsule. Turn away: it goes back out to
   the distance I scrolled to. Tilting low does not put the camera under the floor.
7. Take all the coins; on You win, holding the right button and scrolling do nothing to the
   camera. Restart: play and the camera work as in steps 2–6.
8. Stop. In the Inspector, make the turn speed much higher and the farthest distance much
   larger, Play: the camera turns faster and zooms farther out. Save, close and reopen: the
   numbers are as I left them.
