// The XKB keymap reader. Internal to this folder, and built on both
// platforms even though only the Wayland backend has a caller for it — it
// includes no OS header, so there is nothing platform-specific to guard
// (ADR-0161).
//
// WHAT IT READS. `wl_keyboard.keymap` hands the client a whole keymap as
// resolved XKB v1 text: no `#include`, no macro, everything already expanded
// by the compositor. This reader takes that text and answers one question a
// scancode cannot answer on its own — "what does this key type?" — by
// building a table indexed by evdev code (the XKB keycode minus 8, which is
// what `wl_keyboard.key` hands back) and level (0 plain, 1 shifted).
//
// It reads exactly two blocks and ignores the rest of the file:
//
//   - `xkb_keycodes { ... }` for `<NAME> = N;`, which names a keycode, and
//     `alias <A> = <B>;`, which names another name for one already declared.
//     Aliases are resolved once the block has been read in full, so an alias
//     may point at a name declared earlier or later in the block.
//   - `xkb_symbols { ... }` for `key <NAME> { ... }`, read in either spelling
//     the format allows: a bare `[ sym, sym, ... ]`, or
//     `symbols[Group1] = [ sym, sym, ... ]` with a `type = "..."` statement
//     free to sit beside it. Only Group 1 is read, and only its first two
//     levels — the plain key and the one held with Shift. A key with only one
//     level in the file types that level shifted too, on the reasoning that a
//     key with nothing to shift to should not go silent under Shift.
//
// WHAT IT DOES NOT READ, ON PURPOSE: Caps Lock, level three (AltGr), compose,
// dead keys and every group past the first. A name this reader cannot turn
// into a code point — a dead key, a function key, anything level three needs
// — types nothing, which is level 0. Each is a fixed shape to add to when a
// caller needs it, not a gap this file is guessing at.
//
// A KEYSYM NAME BECOMES A CODE POINT THROUGH A FIXED TABLE, not a computed
// rule: the X11 Latin-1 names for 0x20-0x7e and 0xa0-0xff (`space`,
// `exclam`, `a`, `A`, `odiaeresis`, `aring`, ...), plus two literal forms —
// `U` followed by hex digits, and `0x0100` followed by hex digits — both of
// which name a Unicode code point directly. Anything else is 0.
//
// NO RECURSION, PER RULE 14: the text arrives from the compositor, which
// makes it text from outside the program the same way a glTF file is, and a
// recursive-descent reader is a stack overflow on deeply nested input that
// neither -Werror nor the analyser will catch. The nesting this format can
// have is fixed and shallow — a block, a key inside it, a bracket inside
// that — so every level is its own small function reading from one shared
// cursor, called in a fixed sequence rather than calling itself; a block or a
// key this reader does not specifically parse is skipped by counting braces,
// which is bounded by the length of the text and never by how deep it
// nests. Every name has a fixed capacity too — a keycode name, a keysym name,
// how many names and aliases a `xkb_keycodes` block may declare — past which
// the reader keeps going and simply stops keeping more, rather than growing
// anything.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Indexed by evdev code (the XKB keycode minus 8) and level: 0 plain, 1
// shifted. 0 in a slot means the key types nothing at that level.
#define VOE_PLATFORM_KEYMAP_CODES 256

typedef struct {
	uint32_t typed[VOE_PLATFORM_KEYMAP_CODES][2];
} voe_platform_keymap;

// Reads size bytes of resolved XKB v1 text into *out, which is zeroed first.
// Returns false and leaves *out all zero when the text holds no
// `xkb_symbols` block — text is not a keymap at all, which the caller
// reports, not this file. text need not be NUL-terminated; every read is
// bounded by size.
[[nodiscard]] bool voe_platform_keymap_read(const char *text, size_t size,
					    voe_platform_keymap *out);
