# 02 — The views' border is too bright

## Seen
"022 is good. The line is too bright though. It should be same color as border i think?"
Agreed with the tech-lead that this goes for every draggable border, the side-panel borders from 021 too
(decision 0231).

## Expected
The border between the two scene views, and every side-panel border, is drawn in the same colour as the
editor's other borders while nothing is happening. When the pointer is over one of them, and while it is
being dragged, that border lights up so it clearly stands out against whatever is beside it, in every theme
including the monochrome ones. It goes back to the border colour when the pointer leaves and the drag has
ended. All the draggable borders look and behave the same, and each is as easy to grab as it is today.

## How to reproduce
1. Open the editor in any theme.
2. Look at the border between the two scene views: it is a bright dark-and-light line, unlike the other
   borders.
3. Look at a side-panel border: it does not light up when the pointer is over it or while it is dragged.
4. Expected instead: every draggable border matches the other borders at rest and lights up on hover and
   while dragged. Check this in Near black, Near white and a monochrome theme, with a view showing light
   content and then dark content.
