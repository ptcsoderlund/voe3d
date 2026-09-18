# 01 — The keymap reader reads numeric keysyms and four levels
folder: platform
decisions: 0169, 0168

## Change

Done on 2026-09-17 under the earlier workflow as task 1 of spec 008, commit `ac95db0`; converted to a card by
ADR-0168. Fix `src/keymap.c` and widen `src/keymap.h`.

- **The lexer.** One number token covers both spellings: a decimal run as today, and `0x` followed by hex digits
  consumed whole. One `parse_number` reads either (hex when the span starts `0x`), and `xkb_keycodes`'
  `<NAME> = N;` uses it too.
- **A symbol is a name or a value.** In the symbol list, a token beginning `0x` is a keysym *value*; anything
  else is a keysym *name* and still goes through the Latin-1 table and the `U<hex>` form — a bare `1` in a keymap
  means the keysym named `1` (0x31), never the value 1, so the existing tests must keep passing unchanged.
- **A keysym value becomes a code point by range** (ADR-0169): `0x20`–`0x7e` and `0xa0`–`0xff` are themselves;
  `0x01000000`–`0x0110ffff` are that minus `0x01000000`; everything else is 0, which types nothing. The old
  `0x0100`-prefixed special case in `keysym_lookup` goes: the Unicode range subsumes it and it was unreachable
  anyway. The sponsor has confirmed that `€` (AltGr+5, a legacy-block keysym) is left typing nothing in this
  feature: add no table for those blocks, and add no special case for euro.
- **Four levels.** `voe_platform_keymap.typed` becomes `[VOE_PLATFORM_KEYMAP_CODES][4]` behind a named
  `VOE_PLATFORM_KEYMAP_LEVELS`, holding Group 1's levels 1 to 4 — plain, Shift, AltGr, Shift+AltGr. The existing
  fallback rule is kept and not extended: a list with one entry types that entry shifted too; levels three and
  four are whatever the list holds there, and nothing when it holds nothing. Four fixed levels is the whole of it
  — the sponsor has confirmed Caps Lock is not part of this feature, so do not read a key's XKB type to honour it.
- **AltGr is a place the keymap names.** `voe_platform_keymap` gains a per-code flag saying this key is a
  level-three shift — set when any level of that key's list is the keysym `0xfe03` or the name
  `ISO_Level3_Shift`. On this machine that marks evdev codes 100 (`<RALT>`) and 84 (`<LVL3>`).
- **A key body is read by its statements, not by the first `[`.** Today any `[` at body depth 1 starts the symbol
  list, which reads `Group1` out of `type[Group1]= "..."` as a keysym. At depth 1: a `[` that follows no
  identifier is the bare symbol list; the identifier `symbols` takes an optional `[ index ]` — `Group1` or `1` are
  Group 1, any other index is skipped with `skip_bracket` — then `=` then its list; any other identifier (`type`,
  `virtualMods`, `repeat`, `actions`, ...) swallows its optional `[ index ]`, its `=`, and its value, which is a
  bracketed list or one token. Stay forward-only: no token is put back.
- **A keymap out of which no key types anything is refused**, i.e. `voe_platform_keymap_read` returns false, the
  same as a text with no `xkb_symbols` block. Say why in the header: it is what makes "types nothing rather than
  wrong characters" a reported condition rather than a silent one.
- Update `src/keymap.h`'s and `src/keymap.c`'s headers — what it reads, the two numeric forms, the four levels,
  the level-three flag, what is still deliberately unread (Caps Lock, dead keys, groups past the first, the
  legacy keysym blocks between Latin-1 and the Unicode range) — and the `src/keymap.h`, `src/keymap.c` and
  `tests/keymap.c` lines in `platform.md`.
- **Tests.** Extend `tests/keymap.c` with a second keymap text written in the compositor's own spelling, copied
  from the lines below: `0x` keysyms, `symbols[1]=`, a `type=` with no index beside it, four-entry lists,
  `alias <ALGR> = <RALT>`, and a `modifier_map` line. Check at least: `<AC01>` gives `a A ª º`; `<AD11>`,
  `<AC10>`, `<AC11>` give `å Å`, `ö Ö`, `ä Ä`; `<AE02>` `<AE04>` `<AE07>` `<AE08>` `<AE09>` `<AE10>` `<AE11>`
  give `@ $ { [ ] } \` at level 2; `<AE05>`, whose level 2 is the legacy keysym `0x20ac`, types nothing there;
  `<AE12>` and `<BKSP>` type nothing at any level; `<RALT>`'s code is flagged level-three and types nothing; a
  keysym in the `0x0100xxxx` form still becomes its code point; and a keymap whose every key resolves to nothing
  is refused. The existing name-spelled keymap and its cases stay.

The compositor's own spelling, taken from this machine (mutter, `pc_se_inet(evdev)`, Swedish) on 2026-09-17,
verbatim — the shapes the reader has to survive:

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

## Done when

`cmake -P check.cmake` exits zero on Linux, and `ctest --test-dir build/debug -R '^platform/keymap$' -V` shows
the new cases running and passing.
