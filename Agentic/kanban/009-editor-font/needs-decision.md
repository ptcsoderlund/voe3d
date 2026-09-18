# 009 — needs decision: who holds the desktop's keyboard (bug 02)

## Question
Card 04 (`blocked/04-the-editor-lets-go-of-the-desktops-keyboard.md`, its `## Found` holds the table) measured
everything a coder can measure: the compositor answers new clients as fast with the editor open as with nothing
open, on either GPU, with or without card 03's Wayland change, and the editor binds no protocol that can hold a
keyboard and sends `set_destination` once. No folder is named. The only remaining evidence is a key press, and
only a person can press the start menu key. Before any card can fix bug 02, the sponsor has to say what
happens on the machine, or decide the bug leaves 009.

## Options
1. **The sponsor runs four key tests, and the planner cuts the fix from the answer.** Each on the same
   virtual desktop, pressing the start menu key and then typing into another window, with only this running:
   a. `vkcube` (a stock Vulkan FIFO client, from the Vulkan SDK) — if keys are held here too, the fault is
      KWin's or the driver's, not ours.
   b. `./build/debug/dev/voe_dev` — held here too means `platform` or `render`, not `editor`.
   c. `./build/debug/editor/voe_editor` at HEAD.
   d. The editor built from `74e3009` (before card 03) — free here but held in (c) means card 03.
   Optionally, while doing (c), run KWin with `QT_LOGGING_RULES='kwin_*.debug=true'` and keep
   `journalctl --user -b --since -2min` from right after.
   Report which of a–d hold the keys.
2. **Bug 02 leaves 009.** It is filed on 009 only because 009 was open; nothing shows 009 caused it. 009 is
   accepted on its eight `## How to test` steps, and the bug moves to a feature of its own that starts from
   option 1's tests.
3. **Close it as the compositor's.** Accept the coder's table as enough, and report the behaviour to KDE
   with it. No engine change.

## Recommendation
Option 1, with option 2 if (a) also holds the keys: then it is not the engine's to fix inside 009. The four
tests take a few minutes and are the only way to name an owner; without them any card is a guess.
