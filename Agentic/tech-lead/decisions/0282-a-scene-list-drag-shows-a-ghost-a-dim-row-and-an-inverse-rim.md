# 0282 — A Scene list drag shows a ghost, a dimmed row and an inverse rim
date: 2026-09-28
by: planner

## Decision
For 037 bug 01, how a Scene list drag is shown, within 0194 and 0196:

1. **A drag starts when the pointer has moved 1 mm from where it went down on a row**
   (`VOE_EDITOR_SCENE_DRAG_START`). Before that the press is a click and nothing extra is drawn,
   so a plain click never flashes a ghost. A release with no drag started drops nothing.
2. **The ghost** is a raised panel holding the dragged thing's name, anchored in the root surface
   a little right of and below the pointer, taking no pointer.
3. **The dragged row is dimmed**: drawn under a pushed copy of the theme in force whose `inverse`
   is `control` and whose text roles and `inverse_ink` are `text_disabled`, so held it reads as a
   flat control with the dimmest text.
4. **The drop highlight is an `inverse` rim, not a colour.** The bug asks for "the theme's accent
   colour"; 0194 removed the accent and 0196 made `inverse` its successor. Every row and the
   heading sit in a keyed wrapper panel padded by the rim width on all sides, surface NONE; the
   one a release would parent onto (or the heading, when a release would unparent) is RAISED under
   a pushed theme whose `border` and `surface_raised` are `inverse`. A rim differs from hover (a
   fill step) and from selection (an inverse fill), so it reads even on the selected row.
5. **Only an accepting target is lit.** What a release would do is worked out by the one function
   the release itself uses, after the frame, and drawn the next frame; a refused target (itself,
   under it, already its parent, the heading for a root) is not lit.
6. **Escape during a drag cancels it** ahead of every other Escape use: no ghost, dim or rim, and
   the release that follows neither parents nor selects.

## Reasoning
A rim is the one 0194 state marker left free on a row: hover owns the fill step, selection and
held own inversion. Always wrapping every row keeps each button's key and the list's spacing the
same whether a rim shows or not. A threshold is what keeps click and drag apart without a timer.
Rejected: an inverse fill on the target (the same as a selected row); a new `ui` widget state (a
public `ui` change and a card per user for what a pushed theme and a panel already draw); a real
accent colour (reverses 0194, not this feature's to decide).

## Replaces
nothing. Reads 0194, 0196, 0281.
