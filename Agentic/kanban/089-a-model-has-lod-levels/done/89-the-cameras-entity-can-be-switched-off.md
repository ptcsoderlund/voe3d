# 89 — The camera's entity can be switched off
folder: editor
after: 85, 86, 87, 88
decisions: 0168, 0427, 0428, 0441

## Change
0441 point 2 replaces 0428 point 8: the camera's entity has the Enabled
switch like any other; Duplicate and Delete stay refused (ADR-0218).
Every system already answers an off camera (cards 86-88); this card only
lets it be switched.

`editor/src/scene.c` (open only `voe_editor_scene_switch`, about lines
347-360): the camera's entity is no longer refused; nothing selected and a
prefab's part still are.

`editor/src/scene.h` (lines 332-336 only): the switch's comment drops the
camera's entity from what does nothing and cites 0441. The header's cap is
untouched.

`editor/src/inspector.c` (open only lines 425-460): the Enabled row is
drawn for any selection that is not a part, the camera's entity included;
the Duplicate and Delete row stays under today's condition (not a part,
not the camera's entity). The comment above says the camera's entity is
switched but neither copied nor deleted (ADR-0218, 0441).

`editor/src/interface.h`: no budget change, since the Enabled row is
already counted once; if its paragraph (about line 392) says the camera
has no row, that phrase goes.

`editor/src/src.md`: the `scene.c` and `inspector.c` entries, if they say
the camera is not switched, drop it; each one sentence under 300
characters.

## Done when
`voe_editor` builds in the folder's check with no warning. Then the human
runs bug 11's How to test, steps 1-5
(`Agentic/kanban/089-a-model-has-lod-levels/bugs/11-an-off-emitter-keeps-going-and-an-off-camera-still-looks.planned.md`),
in the editor on a project with an emitter, in Play and in a shipped
build for step 3; every step holds. In particular:
- step 1: switched on, the emitter starts from nothing, as a newly added
  one does, not with particles already in flight;
- step 3: the camera's Inspector shows Enabled; off, Play and Ship look
  from the origin along -Z; the camera's marker is gone from the views
  and a click where it stood selects what is behind; the scene list
  greys it and selects it to switch it back.
