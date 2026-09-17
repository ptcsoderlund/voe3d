# 008 Typing what you press

Status: building
Approved: 2026-09-17
Accepted: -

Typing in the editor on Linux puts a `0` in the box whatever key is pressed, so the folder name
box beside *Make folder* — and every other place text is typed — cannot be used. The editor reads
the keyboard layout the system hands it when it starts, and misreads it. This makes typing produce
the characters actually pressed, on whatever layout the machine is set to.

## Acceptance criteria

1. **Letters and digits.** In the editor's file browser, with Save on an untitled scene, type in the
   name box beside *Make folder*: each key puts its own character in the box, in the order pressed.
   No key puts a `0` in unless it is the `0` key.
2. **Shift.** Holding Shift types the capital letter or the shifted symbol printed on the key.
3. **The whole layout.** The Swedish keys `å`, `ä`, `ö` type themselves, and so do the characters
   that need AltGr, such as `@`, `$`, `{`, `}` and `\`.
4. **Keys that are not characters do nothing.** Function keys, the arrows, Escape, Tab, Home, End
   and the modifier keys by themselves put nothing in the box. Backspace deletes the last character
   and Enter confirms, as before.
5. **Shortcuts still work.** Ctrl+N, Ctrl+O and Ctrl+S still do New, Open and Save, and typing them
   puts nothing in the box.
6. **A folder gets the name typed.** Type a name, press *Make folder*, and the new folder in the
   list has exactly that name, accented letters included.
7. **A keyboard the editor cannot read does not break it.** If the layout cannot be understood, the
   editor still starts and still runs, says so once in plain words, and types nothing rather than
   wrong characters.
8. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- A held key repeating its character. Still a later feature, as agreed for 004.
- Dead keys and accent composition (typing `´` then `e` to get `é`), and input methods for
  languages that need them.
- Changing the keyboard layout while the editor is running: the layout the editor is given when it
  starts is the one it uses.
- Anything the sponsor has to configure. The layout comes from the machine.
- Windows. Its typing is written the same way it is today and is not verified in this feature.

## Constraints

- Linux first: acceptance is on Linux, on the sponsor's own Swedish layout.
- No new third-party dependency: the layout is read by our own code, as it is today.

## Defaults

- Windows keeps its own path to characters and is left alone; nothing in this feature changes it.
- The fix stays inside the folder that owns the keyboard. Nothing in the editor or the interface
  changes, because the wrong characters arrive already wrong.
- Tests are added for the shapes of keyboard layout the fix has to survive, so this cannot come
  back unnoticed.

## Open questions

- None.
