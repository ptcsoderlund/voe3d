# 0287 — With no light, the editor lights with a preview light and the game draws black
date: 2026-09-29
by: tech-lead

## Decision
A scene or prefab still does not need a light, and drawing one that has none never asserts or fails.
In the editor's views, a scene or prefab with no light is lit by the editor's preview light: a
directional light with exactly the settings a newly added directional light gets (its direction,
colour, strength, fill, and shadows as the real one casts them). If the new light's defaults change,
the preview changes with them, because they are one set of values. The preview light is the editor's
alone. It is never saved, is not an entity, never shows in the Scene list or Inspector, and goes away
as soon as the scene or prefab has a light of its own. It lights the editor's normal lit view. Later
view modes (unlit, wireframe and so on) will not use it. Play in the editor and the shipped game use
no preview light: with no light, every lit surface draws black. The background colour and the
game's GUI draw as before.

## Reasoning
The sponsor wants what Godot does: a lightless prefab or level looks right while being edited, and a
game gets only the light its developer put in. Black at run time makes a missing light obvious the
first time Play is pressed, instead of shipping unnoticed. Rejected alternatives:
- unshaded when there is no light (0238): a prefab with no light looks flat and wrong in the editor.
- Play keeps the preview light: the editor would hide the very mistake that going black shows.
- the preview borrows the open level's light: it goes flat again when the level has none, and it
  ties editing a prefab to whichever level is open.
- a prefab carries its own light: every placed copy would bring a light with it.
A 2D or "everything lit as it is" game is no longer served by adding no light; that case waits for a
developer's own shading or an unlit material (`ideas.md`).

## Replaces
Decision 0238.
