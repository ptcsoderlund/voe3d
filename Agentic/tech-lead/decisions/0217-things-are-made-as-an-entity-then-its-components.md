# 0217 — Things are made as an entity and then its components, from a nested menu
date: 2026-09-23
by: tech-lead

## Decision
The editor makes something in two steps and no other way: **Add entity** makes a new entity with only a
transform at the world origin, and **Add component** on the selected entity gives it everything else. There is
no "Add cube", "Add shape" or any other shortcut that makes an entity with components already on it. **Add
component is a dropdown that nests**: each component type says where it sits in the menu (a path such as
"Rendering / Shape"), and the menu is built from those paths, so a project with many component types opens
submenus instead of scrolling through one flat panel of buttons. The transform cannot be removed.

## Reasoning
The sponsor's call (2026-09-23): this is how they always want to work, and it follows 0189, where an entity is
only a number and everything it is comes from its components. A place in the menu that each component declares
keeps the menu from being a hand-kept list that someone forgets to update. Rejected: shortcuts per shape, which
the sponsor does not want; a flat list, which does not grow; a search box first, which can come later on top of
the same paths.

## Replaces
nothing
