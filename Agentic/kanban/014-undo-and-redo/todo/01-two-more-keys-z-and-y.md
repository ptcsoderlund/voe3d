# 01 — Two more keys, Z and Y
folder: platform
decisions: 0168

## Change
Undo's shortcuts need two keys this folder does not read yet. The header already says what adding one costs:
a line in the enum and a line in each backend's table.

`platform/include/platform/input.h`:

- two entries in `voe_platform_key`, after `VOE_PLATFORM_KEY_DELETE` and before `VOE_PLATFORM_KEY_COUNT`:
  `VOE_PLATFORM_KEY_Z` and `VOE_PLATFORM_KEY_Y`. Nothing is reordered: the enum's values are an index into
  each backend's key array.
- the paragraph ending "Delete is the most recent to arrive that way and is not a movement key" now names Z
  and Y as the most recent instead, in the same shape of sentence, saying what reads them — the editor's undo
  and redo (spec 014). Delete keeps its own clause. The standing point above it is untouched and still holds:
  these are places on the keyboard, so on AZERTY the undo key is where W sits, and remapping is the call
  site's.

`platform/src/window_wayland.c`, `key_of`: `case KEY_Z:` and `case KEY_Y:` returning the two new keys, in
the enum's order beside `KEY_DELETE`.

`platform/src/window_win32.c`: the same two cases in its own `key_of`, on `'Z'` and `'Y'` as the letter keys
around them are; and two entries in `focus_gained`'s `VIRTUAL_KEYS` table, `[VOE_PLATFORM_KEY_Z] = 'Z'` and
`[VOE_PLATFORM_KEY_Y] = 'Y'`, so a window regaining focus with either held reads it as held.

No new file, so `platform/src/src.md` and `platform/tests/tests.md` do not change. Nothing else in the folder
reads the enum by length except the two loops over `VOE_PLATFORM_KEY_COUNT`, which are right as they are.

## Done when
`checks.sh --folder platform` exits 0 — which builds the folder and runs its tests, `input.c`'s two loops
over `VOE_PLATFORM_KEY_COUNT` among them.
