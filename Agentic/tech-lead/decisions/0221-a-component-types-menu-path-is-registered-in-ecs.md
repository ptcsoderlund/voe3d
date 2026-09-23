# 0221 — A component type's menu path is registered in ecs, and a type without one is not offered
date: 2026-09-23
by: planner

## Decision
For feature 018, the mechanism 0217 calls "each component type says where it sits in the menu".

- **The path is registered in `ecs` beside the needed type**: `voe_ecs_component_menu_set(world, type, path)`,
  once per type, and `voe_ecs_component_menu(world, type)` hands it back or NULL. The string is the declaring
  folder's static literal and must outlive the world, as a description does. `ecs` stores it and never reads it.
- **A path is parts separated by `/`**, each part trimmed of the spaces round it: `"Rendering / Shape"` is the
  group "Rendering" holding the entry "Shape". A part with no `/` after it is the entry; the ones before it are
  groups. Paths are the program's own, never read from a file, so a malformed one (empty part, deeper than the
  editor's limit) is an assert.
- **The built-in paths**: transform `"Transform"`, light `"Rendering / Light"`, shape `"Rendering / Shape"`.
  Identity and camera set none.
- **Add component offers a type when** it is described, has a default row and a menu path, and the entity
  lacks it. A type with no path is never offered, which is how identity and the camera (0218) stay out of the
  menu without the editor naming either. Groups appear in the order their first entry was registered.
- **The transform cannot be removed because the Inspector is handed it as kept**, beside the identity:
  `dock.c` hands the Inspector a short list of types whose section has no Remove, instead of the one identity
  type it hands today. The Inspector still names no component; 019 adds the camera to the same list.
- **A submenu opens beside its group row**, to the right of the list it is in, on the left when the panel's
  area has no room on the right, and fits vertically by 0200's rule. Opening a group closes any other open
  group at its level or deeper; choosing an entry, a press outside every open list, Escape, or a change of
  selection closes the whole menu.

## Reasoning
`ecs` already carries what a tool learns about a type without naming it — key, description, replace intent,
default row, needed type (0190, 0193) — so the menu path is one more of those, set by the folder that declares
the type, and the menu can never be a hand-kept list in the editor. Putting the path on the struct description
(`base/describe.h`) was rejected: a description is what a file saves, and where a type sits in an editor's menu
is not that. "No path, not offered" answers 0218's "never offer a camera" and keeps identity out with no
exception list. A kept-types list handed in by `dock.c` extends the rule the identity already follows instead
of adding a "not removable" flag to `ecs` that only the editor would read.

## Replaces
nothing
