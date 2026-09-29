# 0288 — The light is a directional light, and "sun" is only a name
date: 2026-09-29
by: tech-lead

## Decision
The light a scene has today is called a directional light in the editor, the docs and new work. "Sun"
is only a name a developer may give the entity that holds one, and the engine does not treat any
light as the sun. A scene will be able to hold several directional lights at once (a sun and a moon),
next to 043's point lights. Until then a frame still draws one, and which of several casts shadows is
not decided here. Existing code and cards that say "sun" need not be renamed for this decision alone.
Anything that touches those names from now on uses "directional light".

## Reasoning
The sponsor wants a sun and a moon at the same time, and "the sun" as a type name would make the
second one odd. Rejected: keep "sun" (singular by meaning); rename every existing "sun" now (churn,
no product change).

## Replaces
nothing. Amends 0273 and 0274, where "the sun" means this directional light.
