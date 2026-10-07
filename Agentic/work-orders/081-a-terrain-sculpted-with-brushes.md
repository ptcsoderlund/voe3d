# 081 — A terrain, sculpted with brushes

## What
A scene can hold a terrain: a ground of a size I choose, flat when it is made. In the editor I sculpt it
with brushes: raise, lower, smooth and flatten. Each brush has a radius, a strength and a soft edge, and
its circle shows on the ground under the pointer. Holding the button paints the brush along where I drag,
and one stroke is one undo step. The terrain is lit, casts and takes shadows like everything else, and
other things can be placed on it. It is saved with the scene and shows the same in Play and in a shipped
game.

## Why
The hill is the ground everything in 0.3 stands on, and 0376 has me shape it in the editor.

## How to test
1. Make a new scene and add a terrain. A flat ground shows in the view. Set its size to 1 km a side.
2. Pick the raise brush and drag over the middle. A hill grows under the pointer while I hold the button,
   and the brush's circle follows the ground.
3. Change radius, strength and edge softness. Each change shows in the circle and in the next stroke.
4. Lower a hollow, smooth a rough slope, and flatten a shelf for the house. Each does what its name says.
5. Undo the last stroke, then redo it. The ground goes back and forth by one stroke.
6. Place a cube on the slope and move the sun. The hill is lit and shadowed, and the cube casts a shadow
   on it.
7. Sculpt fast with a big brush. The editor stays smooth.
8. Save, close and reopen. The hill is as I left it. Press Play: the same in the game window.
