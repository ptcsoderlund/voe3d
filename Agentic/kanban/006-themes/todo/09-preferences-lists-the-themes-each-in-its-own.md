# 09 — Preferences lists the themes, each row in its own
folder: editor
decisions: 0168, 0170, 0172

## Change

New `src/preferences.h` / `src/preferences.c`, the same shape as `src/browser.h` / `.c`: an anchored panel over
the dock, below the bar, shown or not. One row per entry of `src/themes.h`'s list, the built-in first, each a
label with the display name and a Choose button, the one in force marked; a Close button. Each row is drawn
between `voe_ui_theme_push` and `voe_ui_theme_pop` of that row's own `voe_ui_theme`, so it shows its theme while
the rest stays in the chosen one. What fired is recorded and read after the frame, as `src/topbar.c`'s buttons
are.

- `src/topbar.c` / `.h`: a Preferences button beside Save; it shows the panel.
- `src/interface.c`: draws the panel when shown and reads its clicks where it reads the browser's. Choose calls
  `src/themes.h`'s choose (sets the theme on the context, writes the remembered file name); Close hides it.
- `src/main.c`: Escape hides Preferences when the browser is not showing; the browser keeps Escape when it is.
  Preferences suppresses nothing else.
- `src/src.md` and `editor.md` updated.

## Done when

`cmake --preset debug && cmake --build --preset debug --target voe_editor` succeeds and
`./build/debug/editor/voe_editor --capture "$(mktemp -d)/p.png"` exits 0.
