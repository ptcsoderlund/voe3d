# 02 — A blocker's kind is named by what it blocks

## Seen
The Inspector shows a light blocker's Kind as Room, Indoors or Wall. Those name one use of the box, not what it
does, so I have to remember which one stops what.

## Expected
The field is called **Block** and its values are **All**, **Fill** and **Direct** (0352): All is the old Room,
Fill the old Indoors, Direct the old Wall. The lighting is exactly as before. A new blocker is Block All. A scene
saved with blockers before the rename opens with each blocker blocking what it did.

## How to reproduce
1. Open a scene and add a light blocker. Look at it in the Inspector: the field reads Kind, with Room selected.
2. Open the dropdown: Room, Indoors, Wall.
3. Expected instead: the field reads Block, with All selected, and the dropdown lists All, Fill, Direct.
4. Open a scene saved before the change with a Room, an Indoors and a Wall blocker in it: they show as All, Fill
   and Direct and the scene looks exactly as it did.
