# 018 — Add entity, then Add component from a nested menu

## What
There is one way to make something in the level. **Add entity** makes a new entity that has only a transform,
at the centre of the scene. With it selected, **Add component** opens a dropdown of every component type the
entity can take, arranged in submenus, so a long list of types opens as groups rather than one long panel of
buttons. The old buttons that made a cube or another shape in one go are gone: a cube is now an empty entity
with a shape component added. The transform cannot be removed; other components can be, as before. Undo and
redo cover adding an entity and adding a component, one step each.

## Why
Decision 0217: this is how the sponsor always wants to work, and the flat panel of buttons will not hold the
component types a project adds.

## How to test
1. Open the editor on a project. There is no button that makes a cube, capsule or cylinder directly.
2. Press Add entity. A new entity appears in the Scene list and is selected. The Inspector shows only its
   transform, at 0, 0, 0. Nothing is drawn in the view.
3. Press Add component. A dropdown opens with groups; opening a group shows the types in it. Pick the shape
   component. A shape appears at the centre of the view and the Inspector shows it, with its kind and colour.
4. Change its kind to capsule and its colour. The view follows.
5. Open Add component again. Types the entity already has are not offered a second time.
6. Try to remove the transform. The editor does not let you.
7. Remove the shape component. The shape disappears from the view; the entity stays in the Scene list.
8. Undo three times: the shape comes back, then goes, then the entity is gone. Redo brings them back in order.
9. Save, close, reopen. The level is as you left it.
