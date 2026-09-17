# 008 Typing what you press — tasks

Read `plan.md` first: it carries the measured cause and the verbatim keymap lines both tasks are
written against. Both tasks are in `platform`; do them in order.

- [x] 1. `platform/` — the keymap reader reads numeric keysyms and four levels
  - Change: fix `src/keymap.c` and widen `src/keymap.h`.
    - **The lexer.** One number token covers both spellings: a decimal run as today, and `0x`
      followed by hex digits consumed whole. One `parse_number` reads either (hex when the span
      starts `0x`), and `xkb_keycodes`' `<NAME> = N;` uses it too.
    - **A symbol is a name or a value.** In the symbol list, a token beginning `0x` is a keysym
      *value*; anything else is a keysym *name* and still goes through the Latin-1 table and the
      `U<hex>` form — a bare `1` in a keymap means the keysym named `1` (0x31), never the value 1,
      so the existing tests must keep passing unchanged.
    - **A keysym value becomes a code point by range** (plan.md, Decisions): `0x20`–`0x7e` and
      `0xa0`–`0xff` are themselves; `0x01000000`–`0x0110ffff` are that minus `0x01000000`;
      everything else is 0, which types nothing. The old `0x0100`-prefixed special case in
      `keysym_lookup` goes: the Unicode range subsumes it and it was unreachable anyway. The
      sponsor has confirmed that `€` (AltGr+5, a legacy-block keysym) is left typing nothing in this
      feature: add no table for those blocks, and add no special case for euro.
    - **Four levels.** `voe_platform_keymap.typed` becomes `[VOE_PLATFORM_KEYMAP_CODES][4]` behind a
      named `VOE_PLATFORM_KEYMAP_LEVELS`, holding Group 1's levels 1 to 4 — plain, Shift, AltGr,
      Shift+AltGr. The existing fallback rule is kept and not extended: a list with one entry types
      that entry shifted too; levels three and four are whatever the list holds there, and nothing
      when it holds nothing. Four fixed levels is the whole of it — the sponsor has confirmed Caps
      Lock is not part of this feature, so do not read a key's XKB type to honour it.
    - **AltGr is a place the keymap names.** `voe_platform_keymap` gains a per-code flag saying this
      key is a level-three shift — set when any level of that key's list is the keysym `0xfe03` or
      the name `ISO_Level3_Shift`. On this machine that marks evdev codes 100 (`<RALT>`) and 84
      (`<LVL3>`).
    - **A key body is read by its statements, not by the first `[`.** Today any `[` at body depth 1
      starts the symbol list, which reads `Group1` out of `type[Group1]= "..."` as a keysym. At
      depth 1: a `[` that follows no identifier is the bare symbol list; the identifier `symbols`
      takes an optional `[ index ]` — `Group1` or `1` are Group 1, any other index is skipped with
      `skip_bracket` — then `=` then its list; any other identifier (`type`, `virtualMods`,
      `repeat`, `actions`, ...) swallows its optional `[ index ]`, its `=`, and its value, which is
      a bracketed list or one token. Stay forward-only: no token is put back.
    - **A keymap out of which no key types anything is refused**, i.e.
      `voe_platform_keymap_read` returns false, the same as a text with no `xkb_symbols` block.
      Say why in the header: it is what makes "types nothing rather than wrong characters" a
      reported condition rather than a silent one.
    - Update `src/keymap.h`'s and `src/keymap.c`'s headers — what it reads, the two numeric forms,
      the four levels, the level-three flag, what is still deliberately unread (Caps Lock, dead
      keys, groups past the first, the legacy keysym blocks between Latin-1 and the Unicode range)
      — and the `src/keymap.h`, `src/keymap.c` and `tests/keymap.c` lines in `platform.md`.
    - **Tests.** Extend `tests/keymap.c` with a second keymap text written in the compositor's own
      spelling, copied from the lines in `plan.md`: `0x` keysyms, `symbols[1]=`, a `type=` with no
      index beside it, four-entry lists, `alias <ALGR> = <RALT>`, and a `modifier_map` line. Check
      at least: `<AC01>` gives `a A ª º`; `<AD11>`, `<AC10>`, `<AC11>` give `å Å`, `ö Ö`, `ä Ä`;
      `<AE02>` `<AE04>` `<AE07>` `<AE08>` `<AE09>` `<AE10>` `<AE11>` give `@ $ { [ ] } \` at level 2;
    `<AE05>`, whose level 2 is the legacy keysym `0x20ac`, types nothing there; `<AE12>` and
      `<BKSP>` type nothing at any level; `<RALT>`'s code is flagged level-three and types nothing;
      a keysym in the `0x0100xxxx` form still becomes its code point; and a keymap whose every key
      resolves to nothing is refused. The existing name-spelled keymap and its cases stay.
  - Covers: 1, 2, 3, 4, 7
  - Depends on: -
  - Done when: `cmake -P check.cmake` exits zero on Linux, and
    `ctest --test-dir build/debug -R '^platform/keymap$' -V` shows the new cases running and
    passing.

- [ ] 2. `platform/` — the Wayland keyboard holds AltGr and types the level it selects
  - Change: in `src/window_wayland.c`, keep a private `altgr_held` beside the keymap in
    `voe_platform_window`, set and cleared in `keyboard_key` from the keymap's level-three flag for
    that evdev code, set in `keyboard_enter` from the codes the compositor says are already held,
    and cleared in `keyboard_leave` beside `voe_platform_input_focus_lost`. `keyboard_key` then
    picks the level as AltGr twice plus Shift — 0 plain, 1 Shift, 2 AltGr, 3 both — and appends the
    code point it finds there, with Control held still typing nothing so Ctrl+N/O/S stay
    shortcuts only. `keyboard_modifiers` stays empty: AltGr arrives as a key like Shift, and the
    paragraph in the file header that explains why gains AltGr by name. Update the header's
    keymap paragraphs and `platform.md`'s `src/window_wayland.c` line. No public header changes, so
    no folder downstream is touched.
  - Covers: 1, 2, 3, 4, 5, 6, 8
  - Depends on: 1
  - Done when: `cmake -P check.cmake` exits zero on Linux, and `./build/debug/editor/voe_editor`
    types the sponsor's own layout into the name box beside *Make folder* as `plan.md`'s
    Verification describes — letters, digits, Shift, `å ä ö`, AltGr `@ $ { } \`, nothing from the
    function keys, arrows, Escape or Tab, and a folder made with exactly the name typed. Report
    which of those were seen and on which platform.
