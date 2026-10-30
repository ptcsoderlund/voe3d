# 0496 — Sections fold, and each foliage kind is its own folding box
date: 2026-10-30
by: tech-lead

## Decision
1. **Folding is part of the interface toolkit.** `ui` has one folding section that any panel can use: a
   header line with an arrow and a title, and a body below it that the arrow folds away and brings
   back. A folding section may hold others. The title is drawn in a new heading text style that stands
   out from the rows under it, and the body is set off from what follows it, so where one section ends
   and the next begins is plain. Scene list folding may move onto it, as the planner sees fit.
2. **Where it is used now.** Every component section in the Inspector folds. In the landscape's Foliage
   section, each kind is its own folding box, titled with its model's name, holding that kind's
   settings. Clicking a kind's title still picks it to paint with and shows it lit as now. Only the
   arrow folds. A kind just added starts open.
3. **Fold state is remembered.** What is folded is kept with the editor's settings, beside the panel
   layout (0491), not in the project, so it survives a restart and does not travel with a project.
4. **Order.** This is work order 108, built after 106. The hill becomes 109 and stays 0.3's last work
   order.

## Reasoning
The sponsor loses their place among foliage kinds because every kind's settings are a flat run of
rows, with names styled like any other button. They chose folding boxes over a list-then-detail view,
so that several kinds can stay visible and be compared, and want folding in the toolkit, not only in
one panel. One widget in `ui` serves every Inspector section and later panels. A heading style was
missing and is what makes a title read as a title. Fold state is a preference of the person using the
editor, like the panel layout, so it lives with the editor's settings. Alternatives: plain boxes with no
folding (still a long scroll); list-then-detail (only one kind visible at a time); folding built
inside the Foliage section only (every later list builds its own).

## Replaces
Amends 0495 point 3 and 0479 (the hill renumbered 107 → 109).
