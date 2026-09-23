# 0231 — A draggable border rests in the border colour and lights up when reached
date: 2026-09-23
by: tech-lead

## Decision
Every border the person can drag — the border between the two scene views (022) and every side-panel
border (021) — is drawn in the theme's ordinary border colour while nothing is happening. While the pointer
is over it, and for as long as it is being dragged, it is drawn bright enough to stand out against anything
beside it, whatever the theme, including monochrome ones. All draggable borders look and behave alike. The
size of the area that can be grabbed does not change.

## Reasoning
The dark-and-light stripes of 0230 guaranteed contrast but were too bright to live with: the human wanted the
line to match the other borders. A border in the border colour alone can vanish against a scene of that
colour, but it only has to be findable when the person reaches for it, and lighting it on hover and drag
does that — the way most editors behave. Applying it to every draggable border keeps the editor consistent,
so the person never guesses which lines can be dragged.
- Border colour only: calm, but can disappear against the scene — the problem of bug 01.
- Border colour beside one dark stripe: quieter than 0230, still two stripes and still unlike other borders.

## Replaces
Decision 0230.
