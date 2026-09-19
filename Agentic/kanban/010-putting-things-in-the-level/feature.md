# 010 — Putting things in the level

## What
A developer builds a scene in the editor from entities and components, and edits every value
by typing or by dragging.

**Entities.** An **Add** button at the top of the Scene list offers **Entity**, **Cube**,
**Capsule** and **Cylinder**. Entity adds an empty entity: a number with a name and no
components. Cube, Capsule and Cylinder are shortcuts: a new entity that already has a
transform at the centre of the scene (the world origin, scale 1, no rotation) and that shape.
A new entity is named after what was added ("Entity", "Cube", or "Cube 2" when "Cube" exists),
is selected, and shows in the Scene list. A shape shows in both views. The selected entity can be
**deleted** (the Delete key, or a Delete button in the Inspector) and **duplicated** (Ctrl+D, or
a Duplicate button). A copy gets the same components with the same values, a new name as
above, and is selected. The name is edited in the Inspector like any other value.

**Components.** The Inspector lists the selected entity's components, one section each.
An **Add component** button at the bottom lists every component a scene can hold that the
entity does not have yet (an entity holds at most one of each), and choosing one adds it with
its default values. Each section has a **Remove** button. The name cannot be removed: it is what
puts the entity in the Scene list. A shape with no transform is not drawn, and its section says
it needs a Transform.

**Editing values.** Every value in the Inspector can still be dragged. **Clicking a value
without dragging** turns it into a text box with the value selected. Enter or clicking elsewhere
applies it, Escape cancels, and Tab moves to the next value. Text that is not a valid value is
not applied, and the box says so.

**Colour.** A shape has **one colour**, shown in its section as a swatch. Clicking the swatch
opens a **colour picker**: a square for saturation and brightness, a strip for hue, and a hex box
(`#RRGGBB`). The shape changes colour live in both views. Escape or clicking outside closes it
and keeps the colour. The picker is a part of the editor that later panels can reuse, not
something only the Inspector has. A new shape starts in the grey shapes have today.

**Shapes.** Capsule and cylinder are built-in shapes, as the cube is, and lit by the sun the same
way. The capsule is upright, 1 unit across and 2 tall. The cylinder is 1 across and 1 tall with
flat caps.

Every change here (add, delete, duplicate, component added or removed, value edited) marks the
project unsaved, and all of it is saved and opened with the scene.

## Why
Milestone 1 of the road (0186): the developer builds a level by hand out of generic parts. The
engine has no coin; a developer makes one from a cylinder, a colour and a squashed scale. An
entity is only a number, and a transform starts at the origin (0189). Colour stands in for
materials until the shader and material editors exist (0189).

## How to test
1. **Shortcuts.** Open a project. Add → Capsule: a capsule appears at the centre of both views,
   "Capsule" is selected in the Scene list, the bar shows unsaved, and the Inspector shows its
   name, Transform and Shape. Add → Cylinder and Add → Cube do the same. Adding two more cubes
   gives "Cube 2" and "Cube 3".
2. **Shapes look right.** Orbit the views: the capsule is upright with rounded ends, the
   cylinder has flat caps, and both are lit like the cube.
3. **Empty entity.** Add → Entity: "Entity" is in the list and the Inspector shows only its
   name. Nothing is drawn.
4. **Add components.** On that entity, Add component → Shape: the section appears and says it
   needs a Transform, and nothing is drawn. Add component → Transform: the shape appears at the
   centre. Add component no longer offers Shape or Transform.
5. **Remove components.** Remove its Transform: the shape disappears from the views and the
   Shape section says it needs a Transform again. The name has no Remove button.
6. **Type a value.** Select a cylinder and click its Y scale without dragging: a text box opens.
   Type `0.1` and press Enter: the cylinder is a flat disc. Click X position, type `3`, press Tab:
   the cursor moves to Y position. Press Escape: Y is unchanged. Type `abc` in a value and press
   Enter: it is refused, says why, and the old value stays.
7. **Drag still works.** Drag a value as before: it changes and no text box opens.
8. **Rename.** Click the name, type `Coin`, press Enter: the Scene list shows "Coin".
9. **Colour.** Click the disc's swatch: the picker opens. Drag in the square and the strip: the
   disc changes colour live. Type `#FFC800` in the hex box: it turns gold. Press Escape: the
   picker closes and the disc stays gold. A mistyped hex such as `#12` is not applied and says
   so.
10. **Duplicate.** With Coin selected, press Ctrl+D: a copy is selected, in the same place with
    the same scale and colour. Move it on X to see both. The Duplicate button does the same.
11. **Delete.** Select one and press Delete: it is gone from the list and the views, and nothing
    is selected. The Delete button does the same. Delete with nothing selected does nothing.
12. **Saved.** Save, close the editor and open the project again: every entity is there with its
    name, its components, its values and its colour. Deleted ones and removed components stay
    gone.
13. **Unsaved guard.** Add a component, then press Ctrl+O: the editor refuses once, as it does
    for any unsaved change.
14. **A picture shows it.** `voe_editor <project> --capture out.png` shows the gold disc, the
    capsule and the cube.
15. `cmake -P check.cmake` exits zero on Linux.

## Out of scope
- A move, rotate and scale handle in the view, and undo/redo: work order 011.
- Placing a new thing anywhere but the origin, and drag and drop (0189).
- Materials, shaders, textures, and colour on anything but a shape (0189).
- Models from files in the Add menu.
- Several components of the same kind on one entity; multiple selection; parent/child.
- Transparency in the colour.
- Any idea of a coin, pickup or other game thing in the engine or the editor.

## Constraints
- Linux first: acceptance is on Linux (ADR-0130).
- Naming per 0189: an entity is a number, and its place is its transform component.
- The colour picker and the click-to-type value box can be used by any later editor panel.

## Defaults
- A new shape's colour is today's grey.
- Duplicate copies the place exactly and does not offset the copy.
- After a delete, nothing is selected.
- A component added from the Inspector starts at its default values; a Transform starts at the
  origin.

## Open questions
- None.
