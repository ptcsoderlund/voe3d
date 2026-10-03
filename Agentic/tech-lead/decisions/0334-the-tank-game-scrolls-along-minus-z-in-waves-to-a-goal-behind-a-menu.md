# 0334 — The tank game scrolls along −Z, in waves, to a goal, behind a menu
date: 2026-10-03
by: planner

## Decision
For 052, the rules of `examples/tank_game/` the sponsor's level is built for:

1. **The level runs along world −Z**, as the scene's camera looks. A runtime-only `tank_scroll`
   row on the camera keeps `lead`, the camera's z less the first hull's at its first step. After
   the move, each step, the camera's z becomes the lesser of its own and the hull's z plus
   `lead`: it follows the tank forward and never goes back. The row also keeps `bottom` and
   `top`, the z where the view's bottom and top centre rays meet the plane at the hull's height
   (camera z less `far_plane` for a ray that never meets it).
2. **The tank cannot back out of the screen.** A hull's z may not grow past `bottom` less 3 m;
   a drive that would is cut there, and a hull already past is not pulled in.
3. **A spawner is a wave.** It sleeps until the scroll's `top` reaches its z, then spawns as
   before, `count` in all (new, default 4; 0 never ends), counted in a read-only `made`; `most`
   still caps those alive. With no scroll row it is awake, as before.
4. **Score.** `Tank / Breakable` gains `points`, default 100. A player's shot that swaps a
   breakable for its wreck carries those points in its `tank_shot` row.
5. **A goal wins.** `Tank / Goal`, placed by the sponsor, has `points`, default 1000, and needs
   a transform. The player wins when the hull's z reaches the goal's world z, adding its points.
   The player loses at 0 lives.
6. **One runtime-only `tank_state` row on the first hull**: phase (menu, playing, paused, won,
   lost), score, the chosen item and last frame's input levels. A fresh world starts at the
   menu. Its step system makes it, adds this step's shot points while playing, and sets won or
   lost; its interface draws the screens and moves the phase. While not playing, only the state,
   light fade and camera systems run. Paused, won and lost pause the run (0333).
7. **The screens**: menu (Start, Quit); playing, a HUD at the top left with `Score N` and
   `Lives N`; paused (Resume, Menu, Quit); won (`You win`, the score, Menu, Quit); lost (`Game
   over`, the score, Menu, Quit). Menu restarts the run (0333), which lands on the menu.
8. **The keys**: W or S, the pad's d-pad or left stick past 0.5 move the chosen item; Enter,
   Space or the pad's south button press it; a click presses a button. Escape or Start pauses
   while playing; paused, Escape, Start or the east button resumes. Each is an edge against last
   frame's level; the pad is the lowest connected slot.

## Reasoning
Gunsmoke scrolls as the player advances and never back; following the tank keeps the sponsor's
framing and needs no speed to tune. A spawner that wakes when it scrolls into view is "waves where
the sponsor placed them" with one field more and no new component. An end point is one small
component the sponsor places; a boss would need a prefab override the editor lacks. Rejected:
a timed auto-scroll (a number the sponsor did not ask for), a wave component beside the spawner
(two things that spawn), a score kept by the shell system (another module's row).

## Replaces
nothing. Amends 0294 point 4: the lives' HUD moves to the menu's HUD, and 0 lives ends the game.
