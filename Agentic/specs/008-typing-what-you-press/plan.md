# 008 Typing what you press — plan

The fault is in `platform/src/keymap.c` and nowhere else: this compositor writes every keysym as a
bare hexadecimal number, the lexer's number rule stops at the `x`, and `0x71` is read as the keysym
named `0` followed by an identifier `x71` — so every key types `0` and nothing shifted. The reader
is taught that a keysym is a number as often as it is a name, that a key has four levels, and that
the keymap itself names the key which selects levels three and four; `window_wayland.c` then holds
AltGr the way it already holds Shift — as a place, not as a compositor modifier — and picks the
level from the two. Nothing outside `platform` changes.

## The cause, measured

Taken from this machine's own compositor on 2026-09-17 (a client bound `wl_seat`, mapped the
`wl_keyboard.keymap` descriptor and wrote it out; the layout is `pc_se_inet(evdev)`, Swedish).
Running today's `voe_platform_keymap_read` over that text gives `U+0030` at level 0 and `U+0000` at
level 1 for **every** evdev code tested — the reported symptom, reproduced outside the editor.

The text is written like this, and these lines are the shapes the fix has to survive. They are
copied verbatim; the test in task 1 uses this spelling, not an invented one:

```
	<AE01> = 10;
	alias <ALGR> = <RALT>;
	key <AE02> {	[ 0x32, 0x22, 0x40, 0xb2 ] };
	key <AE04> {	[ 0x34, 0xa4, 0x24, 0xbc ] };
	key <AE05> {	[ 0x35, 0x25, 0x20ac, 0xad5 ] };
	key <AE07> {	[ 0x37, 0x2f, 0x7b, 0xf7 ] };
	key <AE08> {	[ 0x38, 0x28, 0x5b, 0xab ] };
	key <AE09> {	[ 0x39, 0x29, 0x5d, 0xbb ] };
	key <AE10> {	[ 0x30, 0x3d, 0x7d, 0xb0 ] };
	key <AE11> {	[ 0x2b, 0x3f, 0x5c, 0xbf ] };
	key <AE12> {	[ 0xfe51, 0xfe50, 0xb1, 0xac ] };
	key <AC01> {	[ 0x61, 0x41, 0xaa, 0xba ] };
	key <AC10> {	[ 0xf6, 0xd6, 0xf8, 0xd8 ] };
	key <AC11> {	[ 0xe4, 0xc4, 0xe6, 0xc6 ] };
	key <AD11> {	[ 0xe5, 0xc5, 0xfe57, 0xfe58 ] };
	key <BKSP> {	[ 0xff08, 0xff08 ] };
	key <LVL3> {	[ 0xfe03 ] };
	key <RALT> {
		type= "ONE_LEVEL",
		symbols[1]= [ 0xfe03 ]
	};
	modifier_map Mod5 { <LVL3> };
```

Four things in that are new to the reader: keysyms as `0x` numbers; a group index written `1`
rather than `Group1`; a `type=` statement with no index before it; and AltGr present only as the
keysym `0xfe03` on `<RALT>` (evdev code 100) and `<LVL3>`.

Three further points settled by reading, so that no task has to guess:

- The `ui` and `editor` sides are clear. `ui`'s field stores the bytes `platform` hands it and
  `ui/tests/widgets.c` covers that; the characters arrive already wrong.
- `text` draws U+0020 to U+00FF (`text/src/font.c`, `FIRST_CHARACTER`/`LAST_CHARACTER`), so `å ä ö`
  and every AltGr character the spec names draw once they are stored. A code point outside that
  range would be stored correctly and drawn as the missing-glyph box — see Risks.
- Nothing but `window_wayland.c` and `platform/tests/keymap.c` includes `keymap.h`; `window_win32.c`
  does not, so widening the table cannot touch Windows.

## Decisions

- A keysym is read as a number as readily as a name, a key carries four levels, and AltGr is the
  key the keymap points at rather than a modifier bit from the compositor — reason: the number form
  is what a real compositor emits, four levels is the smallest shape that holds the spec's AltGr
  characters, and reading AltGr as a place keeps `window_wayland.c`'s existing argument for taking
  Shift from the key and not from `wl_keyboard.modifiers`. Project-wide:
  `Agentic/decisions/0168-a-keysym-is-a-number-a-key-has-four-levels-and-altgr-is-a-place.md`.
- A keysym value becomes a code point by range, not by table: `0x20`–`0x7e` and `0xa0`–`0xff` are
  themselves, `0x01000000`–`0x0110ffff` are that minus `0x01000000`, everything else types nothing.
  Feature-local, because it is the body of one function: the ranges are the whole of what the two
  Latin-1 blocks and the Unicode form mean, and the legacy blocks between them (Greek, Cyrillic,
  the currency block that holds `€`) need a table this feature has no caller for — and `text` would
  draw them as the missing-glyph box anyway. **Asked and answered by the sponsor on 2026-09-17: `€`
  is left out on purpose. It types nothing for now, and a later feature adds it together with a font
  that can draw it. Everything else on AltGr — `@ $ { } [ ] \` — must type.** Do not add a legacy
  keysym table to this feature, and do not report `€` typing nothing as a defect.
- The reader refuses a keymap out of which not one key types anything. Feature-local. It is what
  makes criterion 7 reachable rather than theoretical, and it is the shape of failure this very bug
  would have had if the symbol list had lexed to nothing instead of to `0`.
- Caps Lock, dead keys, key repeat and groups past the first stay unread, as ADR-0161 left them.
  **Asked and answered by the sponsor on 2026-09-17: Caps Lock is not part of this feature. Shift
  gives capitals, as the spec says, and Caps Lock keeps changing nothing typed, exactly as today.**
  A task that finds itself reading a key's XKB type to honour Caps Lock has gone past this feature.

## Folders

- `platform/` — changed — internal only. `src/keymap.h` gains levels (`typed[code][4]`) and the
  level-three key table; no public header changes, so no folder downstream of `platform` is touched.

## Verification

- `cmake -P check.cmake` — exits zero on Linux, including `platform/keymap` and `platform/input`.
- `./build/debug/editor/voe_editor` — open Save on the untitled
  scene, and in the name box beside *Make folder* type the sponsor's own keyboard: letters and
  digits arrive as pressed, Shift gives capitals and the shifted symbols, `å ä ö` type themselves,
  AltGr gives `@ $ { } [ ] \`, Escape/Tab/the arrows/the function keys add nothing, Backspace
  deletes and Enter confirms, Ctrl+N/O/S still do New/Open/Save and type nothing, and *Make folder*
  makes a folder with exactly the name typed — criteria 1 to 6. AltGr+5 typing no `€`, and Caps Lock
  changing nothing, are both expected here and are not failures (see Decisions).
- With a keymap the reader refuses, the editor still opens and warns once and types nothing —
  criterion 7. Task 1's test covers the refusal itself; the warning path in `window_wayland.c` is
  unchanged by this feature.
