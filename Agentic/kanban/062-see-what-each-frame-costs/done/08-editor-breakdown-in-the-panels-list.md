# 08 — The frame breakdown opened from the Panels list
folder: editor
after: 07
decisions: 0168, 0358, 0363, 0367

## Change
- `editor/src/dock.h` / `editor/src/dock.c`: `VOE_EDITOR_CLOSABLE_FRAME` after ERRORS, named
  "Frame breakdown"; the comments listing the closables name it.
- `editor/src/panels.h` / `editor/src/panels.c`: `voe_editor_panels_toggle` and
  `voe_editor_panels_open` take the `voe_editor_frame_breakdown *` (const for open); FRAME shows or
  hides it and touches no other panel nor the settings file. Header comment names the case.
- `editor/src/panels_menu.h`: its row list names Frame breakdown.
- `editor/src/interface.h` / `editor/src/interface.c`: `voe_editor_interface_draw` takes the
  breakdown; while it shows and the browser, Preferences, Project and Errors do not, it is drawn at
  the left just below the bar, over the Scene list, so the views and the Inspector stay usable; its ×
  hides it. The draw comment says so; `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` grow by the cost
  `frame_breakdown.h` states, with a paragraph in the budget comment like the others.
- `editor/src/main.c`: holds one breakdown beside the other panels, calls
  `voe_editor_frame_breakdown_take` once a frame after the frame ends with the frame's seconds, and
  passes it to the interface. Not shown in a capture.
- `editor/src/src.md`: fix the entries of the files changed.

## Done when
- `cmake --build --preset debug --target voe_editor` exits 0.
- `build/debug/editor/voe_editor examples/tank_game --capture <scratch>/tank.png` exits 0.

## Human
Feature steps 1, 2, 3 and 5: open the tank game, Panels → Frame breakdown; every pass is listed
with a time and they add up to roughly the total; a sun's shadows and a light's bounce add their
passes when on and drop them when off; a RenderDoc capture shows the pass and resource names.
