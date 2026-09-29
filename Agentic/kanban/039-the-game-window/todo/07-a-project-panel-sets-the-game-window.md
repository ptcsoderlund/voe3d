# 07 — A Project panel sets the game window
folder: editor
after: 05, 06
decisions: 0168, 0291

## Change
0291 point 4. Card 05 added `voe_editor_project_window_set` (`editor/src/project.h`). Files:
new `editor/src/project_panel.h` and `project_panel.c`; `editor/src/topbar.h`, `topbar.c`,
`editor/src/interface.h`, `interface.c`, `editor/src/src.md`. Read `editor/src/preferences.h`
for the shape to copy and `ui/include/ui/widgets.h` for the number box and the choice.

- `project_panel.h/.c`: the Project panel, Preferences' shape: a zeroed struct is never shown;
  `voe_editor_project_panel_show`; a draw that lays out, in an anchored panel where Preferences
  goes, a Width and a Height number box (step 1) showing the window it is handed, a two-row
  choice Windowed / Fullscreen, and Close; a clicks read after `voe_ui_frame_end` answering
  closed, and a changed window when a number was committed (rounded, clamped to 0291's range) or
  the other choice picked. It carries out nothing.
- `topbar.h/.c`: a Project button left of Preferences, recorded and read as Preferences' is.
- `interface.h/.c`: Project shows the panel in Preferences' and the browser's place (one of the
  three at a time, as those two are now); a changed window goes to
  `voe_editor_project_window_set` on the session's project, a failure into the notice. The
  node and element budget paragraphs: the bar's new button and label, and the panel costing less
  than the browser it replaces (count it and say so, as the Preferences paragraph does); raise
  a capacity only if the count needs it.
- Header points for `project_panel.h`: what the panel shows, that a change is written at once and
  is not undone (0291 point 4), that the size is ignored while Fullscreen is chosen, and that it
  carries out no command.
- `src.md`: entries for the two new files; the `topbar`, `interface` entries name Project.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0 and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" &&
test -s "$d/t.png"` exits 0. For the human: Project opens the panel; set 1600×900, close and
reopen the editor, and the panel still reads 1600×900; `project.voe3d` has a `[window]` section.
