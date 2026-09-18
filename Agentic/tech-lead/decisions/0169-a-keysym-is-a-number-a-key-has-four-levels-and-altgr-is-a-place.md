# 0169 — A keysym is a number as often as a name, a key has four levels, and AltGr is a place
date: 2026-09-17
by: planner

ADR-0161 had `platform` read the Wayland keymap itself, and described a keysym as a name — the X11
Latin-1 names, plus `U<hex>` and `0x0100<hex>` for the two literal forms. A real compositor does not
write names. Dumping this machine's `wl_keyboard.keymap` (mutter, `pc_se_inet(evdev)`) shows every
symbol written as a bare hexadecimal number, `key <AC01> { [ 0x61, 0x41, 0xaa, 0xba ] };`, and the
reader's number rule stops at the `x`: `0x61` lexes as the keysym named `0` and an identifier
`x71`, so every key in the editor typed `0` and nothing else (spec 008). Spec 008 also asks for the
AltGr characters ADR-0161 left out — `@ $ { } \` are level three of a Swedish layout — which is a
question about levels and about which key selects them.

## Decision

**A symbol is read as a value whenever it is spelled as one.** A token beginning `0x` in a symbol
list is a keysym value; anything else is a keysym name and goes through the Latin-1 table and the
`U<hex>` form exactly as before. That distinction is the format's, not ours: a bare `1` in a keymap
is the keysym *named* `1`, never the value 1, and a reader that confuses the two mistypes every
digit on a keymap written in names.

**A keysym value becomes a code point by range, and the ranges are the whole rule.** `0x20`–`0x7e`
and `0xa0`–`0xff` are the two Latin-1 blocks, whose values are their code points;
`0x01000000`–`0x0110ffff` is the Unicode form, which is that minus `0x01000000`. Everything else —
the legacy blocks between them, every `0xfeXX` dead key and `0xffXX` function key — types nothing,
which is what a key that types nothing already meant here. This replaces ADR-0161's `0x0100<hex>`
clause, which named a form a compositor does write and reached it through a prefix match that no
input could satisfy.

**A key has four levels: plain, Shift, AltGr, Shift+AltGr.** Not a level count read from the key's
type — the types in a real keymap (`FOUR_LEVEL`, `ALPHABETIC`, `PC_ALT_LEVEL2`, `KEYPAD`, ...) are
a second language inside the first, and what they buy over four fixed levels is Caps Lock and the
per-type modifier maps, neither of which anything asks for. Group 1 only, as before.

**AltGr is a place, read out of the keymap, and not a modifier bit from the compositor.** The
reader marks every evdev code whose symbol list holds the keysym `0xfe03` (`ISO_Level3_Shift`), and
the Wayland backend holds those codes the way it already holds Shift — from `wl_keyboard.key`. This
keeps `window_wayland.c`'s existing argument intact: `wl_keyboard.modifiers` answers "what would a
keystroke type", the engine asks "is this key held", and reading both is two answers to one
question. It also means nothing has to interpret `modifier_map Mod5 { <LVL3> }`, a mapping that
differs between layouts and says which *modifier* AltGr sets, not which key it is.

**A keymap out of which not one key types anything is refused**, exactly as a text with no
`xkb_symbols` block is: the caller warns once and types nothing. A reader that silently produces a
table of zeros is the same failure as a reader that silently produces a table of `0` characters,
and spec 008's criterion 7 wants the first said out loud.

**ADR-0161 is amended, not superseded.** Typed text polled per frame, the buffer, Control typing
nothing, Windows reading `WM_CHAR`, no key repeat and the refusal to take `xkbcommon` all stand
untouched. What changes is the inside of the reader it introduced: its keysym clause and its "levels
one and two" clause.

## Reasoning

- **Take `xkbcommon` after all** — the dependency rule 5 turned down twice, and the fault was a
  lexer rule three lines long, not the size of the problem.
- **Read the key's XKB type and honour it** — correct, and it is a second parser, a second table and
  Caps Lock's semantics, for a feature whose acceptance is a folder name typed in a box. The day
  Caps Lock or a five-level layout is asked for, the type is where to start.
- **Take AltGr from `wl_keyboard.modifiers`** — it means reading `modifier_map` to learn which bit
  is level three, and it contradicts the file's own reason for taking Shift from the key.
- **A table for the legacy keysym blocks** (Greek, Cyrillic, the currency block that holds `€`) —
  eight hundred entries nothing calls for (rule 10), and `text`'s atlas stops at `0xff`, so the
  first thing such a table would buy is a missing-glyph box in place of a character.
- **Keep two levels and reach AltGr some other way** — there is no other way; level three *is* where
  those characters are written.

## Consequences

- `voe_platform_keymap` grows from two levels to four and gains a flag per code, about four
  kilobytes in a window. It is internal to `platform`; no public header moves and no folder
  downstream of `platform` is touched.
- A character on level five or in a group past the first still cannot be typed, and neither can one
  whose keysym is in a legacy block — `€`, on a Swedish layout's AltGr+5, is the one a Swedish
  sponsor is most likely to reach for.
- Caps Lock still does nothing to what is typed. Holding Shift is the only way to a capital.
- Some characters that now type are ones the editor's face cannot draw: Pixel Operator carries no
  `ª º ¤ §` (ADR-0167), so AltGr+a and the plain `§` key store a correct character and draw the
  missing-glyph box. That is the face's gap, not the keyboard's.
- The keymap text is now read in two spellings that no longer share a code path at the token level;
  `platform/tests/keymap.c` keeps one keymap in each spelling, and a keymap from a compositor that
  writes names stays covered.

## Replaces

Nothing. ADR-0161 is amended as the decision says, not superseded. Written on `feature/008-typing-what-you-press` under the earlier workflow as 0168; renumbered by ADR-0168 when the branch took the Agentic workflow.
