# 012 — Choose a shape's kind from a dropdown

## What
In the Inspector, a Shape's kind is a dropdown showing Cube, Capsule and Cylinder by name, not a number that
cannot be changed. Picking another one changes what the entity draws straight away, in both scene views, and
marks the project unsaved. Its colour, position, rotation and scale stay as they were. Saving and opening the
project again keeps the kind that was picked. Adding a Shape component to a plain entity still starts as a grey
cube; the dropdown is how you make it something else (decision 0195).

## Why
Today a shape added by hand can only be a cube, and swapping one means deleting it and adding it again.

## How to test
1. Open the editor on a project, click Add → Entity, then Add component → Transform, then Add component →
   Shape. A grey cube appears at the origin, and the Shape section shows a dropdown reading Cube.
2. Open the dropdown. It lists Cube, Capsule and Cylinder. Pick Cylinder. The cube becomes a cylinder in both
   views at once, and the project is marked unsaved.
3. Pick Capsule, then Cube again. Each time the drawn shape changes straight away and nothing else about it
   does: same place, same size, same colour.
4. Give it a colour with the swatch, move it with the Transform, then pick Cylinder. It stays that colour and
   where you put it.
5. Select an entity made with Add → Capsule. Its dropdown reads Capsule, and it can be changed the same way.
6. Duplicate an entity whose kind you changed. The copy is the same kind.
7. Save, close the editor, open the project again. Every shape is the kind you left it.
8. With the dropdown open, press Escape or click somewhere else. It closes and the kind is unchanged.
