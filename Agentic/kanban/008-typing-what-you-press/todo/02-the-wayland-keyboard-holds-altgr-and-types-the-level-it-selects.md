# 02 — The Wayland keyboard holds AltGr and types the level it selects
folder: platform
decisions: 0169, 0168

## Change

Card 01 taught `src/keymap.c` four levels per key and a per-code flag marking the key that selects levels three
and four (`0xfe03`, `ISO_Level3_Shift`). This card makes the Wayland backend use them; nothing outside
`platform` changes, and the Windows backend is not touched.

In `src/window_wayland.c`, keep a private `altgr_held` beside the keymap in `voe_platform_window`, set and
cleared in `keyboard_key` from the keymap's level-three flag for that evdev code, set in `keyboard_enter` from
the codes the compositor says are already held, and cleared in `keyboard_leave` beside
`voe_platform_input_focus_lost`. `keyboard_key` then picks the level as AltGr twice plus Shift — 0 plain, 1 Shift,
2 AltGr, 3 both — and appends the code point it finds there, with Control held still typing nothing so
Ctrl+N/O/S stay shortcuts only. `keyboard_modifiers` stays empty: AltGr arrives as a key like Shift, and the
paragraph in the file header that explains why gains AltGr by name (ADR-0169: AltGr is a place read out of the
keymap, not a modifier bit from the compositor). Update the header's keymap paragraphs and `platform.md`'s
`src/window_wayland.c` line. No public header changes, so no folder downstream is touched.

Settled, so do not re-derive: `ui`'s field stores the bytes `platform` hands it (`ui/tests/widgets.c` covers
that) and `text` draws U+0020 to U+00FF, so `å ä ö` and every AltGr character the feature names draw once they
are stored. `€` (AltGr+5, a legacy-block keysym) types nothing on purpose, and Caps Lock changes nothing typed;
neither is a defect of this card.

## Done when

`cmake -P check.cmake` exits zero on Linux, and `./build/debug/editor/voe_editor` — Save on the untitled scene,
then the name box beside *Make folder* — types the sponsor's own Swedish layout: letters and digits arrive as
pressed, Shift gives capitals and the shifted symbols, `å ä ö` type themselves, AltGr gives `@ $ { } [ ] \`,
Escape, Tab, the arrows and the function keys add nothing, Backspace deletes and Enter confirms, Ctrl+N/O/S
still do New/Open/Save and type nothing, and *Make folder* makes a folder with exactly the name typed. Report
which of those were seen and on which platform. With a keymap the reader refuses, the editor still opens, warns
once and types nothing.
