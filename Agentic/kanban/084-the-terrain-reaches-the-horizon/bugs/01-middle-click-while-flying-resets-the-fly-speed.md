# 01 — Middle-click while flying resets the fly speed

## Seen
"I cannot test this. We need variable camera speed. When I hold RMB I want to be able to scroll to
increase/decrease camera movement speed in scene view. While holding RMB and middle click I want it to
reset to 1.0."

Crossing a 4 km terrain (084's How to test, steps 3 and 4) needs the fly speed to change while flying.
Card 12 made the wheel change it while the right button is held; nothing puts it back once it has been
turned far up or down.

## Expected
- Holding the right button over a scene view and turning the wheel makes the fly faster or slower, in
  even steps, and the view keeps that speed for its next fly. (Card 12; this report keeps it.)
- Holding the right button and clicking the middle button puts the fly speed back to 1.0: the speed a
  view's first fly starts at, the starting speed counting as 1.0. It does not start a middle-button drag
  or move the view.
- The speed is per view, as now.

## How to reproduce
1. Open a project with a terrain and hold the right button over a scene view.
2. Turn the wheel up several notches while moving with the keys: the fly gets faster. Turn it down: it
   gets slower.
3. Still holding the right button, click the middle button. Now the fly moves at the speed it had when
   the editor started, and the view did not jump or pan.
4. Let go and fly again in the same view: it starts at that reset speed.
