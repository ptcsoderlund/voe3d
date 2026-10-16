# 0439 — Panels float and dock anywhere inside the editor window
date: 2026-10-16
by: tech-lead

## Decision
Every editor panel with a header (Frame, Scene list, Inspector, Assets, Errors, Project and later ones) can be
dragged by its header. Dropped on an edge of another panel, of the scene views or of the editor's dock, it docks
there, splitting that place; while dragging, an outline shows where it will land. Dropped anywhere else it
floats over the editor as a box that is moved by its header and resized by its edges and stays inside the window.
Double-clicking a floating panel's header docks it back where it last was docked. The top bar stays where it is.
The scene views fill whatever the docked panels leave. The whole layout (where each panel is docked or floats,
and its size) is the person's, kept in the editor settings file (0220), and comes back after a restart; a
missing or unreadable layout means today's. Closing and reopening (0351) keeps a panel's place. Panels never
become separate desktop windows, and two panels never share one place behind tabs; both are later if ever.

## Reasoning
"Place it where I want" means docking, not only floating; floating alone leaves panels covering the scene.
- Float only: smaller, but every moved panel hides part of a view.
- Tabs as well: more to build and test; stacking can come on its own once docking exists.
- Separate desktop windows: Wayland does not let a program place its own windows, and each needs its own swap chain.

## Replaces
nothing. Carries out the "docking can come later" of 0351 and 0363.
