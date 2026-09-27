# 0267 — 0.2 is chosen by its game, and documentation waits for 1.0
date: 2026-09-27
by: tech-lead

## Decision
0.1 is done: 0186's road ends with the coin game shipped, a game built from built-in shapes and
one light, open source on GitHub. 0.2 is picked the same way 0.1 was. First the sponsor and the
tech-lead choose the next game. Then they list the engine features that game needs, and that list
is 0.2, written as its own decision with its milestones. Nothing goes into 0.2 unless the game
needs it. The engine is for people who want to hack on it and read the code until 1.0. Nobody
writes user documentation, getting-started guides, tutorials or contributor material (issue
templates, contribution guides) before 1.0. The README stays as 0263 set it, and the folder
`.md` files stay what they are: notes for whoever works in that folder.

## Reasoning
Letting one game decide 0.1 kept every feature justified, and nothing was built on speculation.
The engine is not usable for real games yet, so documenting it now would describe things that are
about to change. Alternatives: a feature-driven 0.2 without a game (real content, a looks pass),
which risks building what no game uses; open-source readiness first (CI, guides, templates),
which is wasted before the engine is usable.

## Replaces
nothing. It follows 0186, whose road it closes.
