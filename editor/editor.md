# editor

The program a person opens to author a scene. Today it opens a window on a top
bar — New, Open, Save, the project's name and a notice — above three columns:
`Scene` on the left, two scene views stacked in the middle, and `Inspector` on
the right. The left one lists the authored entities of the project it opens on
and a click on one selects it; each view draws the world from its own camera,
moved by a middle-button drag in it, lit by whichever light the world holds;
the right one lists what the selected entity is made of and lets a number in
it be dragged, which is what marks the project unsaved. Each column that is
not a scene view clips what is on it and scrolls it with the wheel or its
scrollbar, and the Inspector's field rows fold onto further lines when the
column is too narrow for them. `voe_editor [<folder>]` opens folder, or the
last project remembered when there is none, or an untitled cube and light when
there is neither — see `src/project.h` and `src/last_project.h`. New, Open and
Save — the buttons and Ctrl+N, Ctrl+O and Ctrl+S alike — go through
`src/session.h`, which refuses New, Open and closing the window once while
there are unsaved changes and goes ahead the second time. It can also be
started to draw one frame with no window at all, write it to a PNG file and
exit — `--capture <path>`, with `--size <W>x<H>` saying how big.

It draws in a theme: the built-in one, or the `*.theme` file in
`<settings>/voe3d/themes/` named by `<settings>/voe3d/theme` — see
`src/themes.h`. A remembered theme that is gone or refused draws the built-in
and says why in the bar. The selected row in `Scene` is drawn in the theme's
accent.

Open shows the editor's own file browser — an anchored panel over the dock,
below the bar — to choose a project's folder from; a folder marked "— project"
already holds one. Confirming it replaces the project on success, or leaves
the browser open with a notice on failure. Save on an untitled project shows
the same browser in SAVE mode instead, with a name box and a Make folder
button beside the confirm button, now "Save here": typing a name and either
pressing Enter in the box or clicking Make folder makes that folder and opens
it, and Save here writes `project.voe3d` and the scene into whichever folder
the browser is in, refusing one that is not empty. While the browser shows,
the top bar's clicks and the three shortcuts do nothing, and a scene view's
camera does not move; Escape or Cancel dismisses it without changing
anything.

It is a leaf and it stays one, exactly as `dev` is: it names whatever it needs
and nothing names it (ADR-0121). No engine folder gains anything for the
editor's sake — a gap in one of them is a card in that folder, never a
reach-around from here.

- `src` — the implementation: the loop, the project and its session, the
  themes, the top bar, the browser, the dock and the three panels; each file is listed on
  `src/src.md`.
