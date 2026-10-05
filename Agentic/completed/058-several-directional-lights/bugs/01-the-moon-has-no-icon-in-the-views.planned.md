# 01 — The moon has no icon in the views

## Seen
"Moon have no icon/gizmo. I can select it in scene list but not in editor view, rotation handles show."

The second directional light shows no icon in the scene views, so it cannot be clicked there. Selected from
the Scene list, its rotation handles do show.

## Expected
Every directional light shows the same icon the sun does, where it is, and clicking that icon selects it, just
as clicking the sun's icon selects the sun. With several lights, clicking one icon selects that light and no
other.

## How to reproduce
1. Open the sun and moon example (or a scene with a sun, then add a second directional light named Moon).
2. Look in a scene view: the sun has its icon, the moon has none.
3. Try to click where the moon is in the view: it is not selected.
4. Select the Moon in the Scene list: its rotation handles show in the view.
