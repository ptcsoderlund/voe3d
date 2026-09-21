// The XKB keymap reader. Internal to this folder, and built on both
// platforms even though only the Wayland backend has a caller for it — it
// includes no OS header, so there is nothing platform-specific to guard
// (ADR-0161).
//
// WHAT IT READS. `wl_keyboard.keymap` hands the client a whole keymap as
// resolved XKB v1 text: no `#include`, no macro, everything already expanded
// by the compositor. This reader takes that text and answers two questions a
// scancode cannot answer on its own — "what does this key type?", at each of
// four levels, and "is this key AltGr?" — by building tables indexed by
// evdev code (the XKB keycode minus 8, which is what `wl_keyboard.key` hands
// back).
//
// It reads exactly two blocks and ignores the rest of the file:
//
//   - `xkb_keycodes { ... }` for `<NAME> = N;`, which names a keycode, and
//     `alias <A> = <B>;`, which names another name for one already declared.
//     Aliases are resolved once the block has been read in full, so an alias
//     may point at a name declared earlier or later in the block. N is read
//     as either a plain decimal run or a `0x` hexadecimal one, the same two
//     forms a keysym takes below.
//   - `xkb_symbols { ... }` for `key <NAME> { ... }`, read by its statements,
//     not by the first `[` — a `[` with no identifier before it is the bare
//     symbol list, `symbols[Group1] = [ ... ]` or `symbols[1] = [ ... ]`
//     (a plain `1` is Group 1 too) reads the same list explicitly, any other
//     index on `symbols` skips its list, and every other statement —
//     `type`, `virtualMods`, `repeat`, `actions`, ... — has its own optional
//     index, its `=` and its value swallowed unread. Only Group 1 is read,
//     and only its first four levels: plain, Shift, AltGr, Shift+AltGr. A
//     key with fewer entries in its list types nothing at the levels past
//     what it has, except that a list of exactly one entry types it shifted
//     too, on the reasoning that a key with nothing to shift to should not
//     go silent under Shift.
//
//
// WHAT IT DOES NOT READ, ON PURPOSE: Caps Lock, a key's XKB type, compose,
// dead keys, and every group past the first. Each is a fixed shape to add to
// when a caller needs it, not a gap this file is guessing at.
//
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

// Indexed by evdev code (the XKB keycode minus 8). 0 in a slot means the key
// types nothing at that level.
#define VOE_PLATFORM_KEYMAP_CODES 256

// Group 1's levels this reader keeps: 0 plain, 1 Shift, 2 AltGr, 3
// Shift+AltGr.
#define VOE_PLATFORM_KEYMAP_LEVELS 4

typedef struct {
	// A SYMBOL IS A NAME OR A VALUE. A token spelled `0x...` in a symbol list is
	// a keysym VALUE and becomes a code point by range, not by table: 0x20-0x7e
	// and 0xa0-0xff are themselves, 0x01000000-0x0110ffff are that minus
	// 0x01000000 (the form XKB uses to name a Unicode code point numerically),
	// and everything else — every dead key, function key, and the legacy
	// keysym blocks between the two Latin-1 ranges and the Unicode form (Greek,
	// Cyrillic, the currency block that holds the euro sign) — is 0, which
	// types nothing. Anything not spelled `0x...` is a keysym NAME and goes
	// through the X11 Latin-1 table in keymap.c plus the literal `U<hex>` form, the
	// same as before this reader knew a keysym could be a bare number at all —
	// a bare `1` in a keymap names the keysym called `1` (0x31), never the
	// value 1.
	uint32_t typed[VOE_PLATFORM_KEYMAP_CODES][VOE_PLATFORM_KEYMAP_LEVELS];
	// AltGr IS A PLACE THE KEYMAP NAMES, not a modifier bit from the compositor:
	// an evdev code is flagged as a level-three shift when any level of its own
	// symbol list is the keysym value 0xfe03 or the name `ISO_Level3_Shift`,
	// whether or not that level types a character (it never does — 0xfe03 is
	// not in any range above). A caller holds it exactly the way it already
	// holds Shift.
	//
	// Set for the evdev code(s) the keymap itself names as the
	// level-three shift (AltGr).
	bool level3_shift[VOE_PLATFORM_KEYMAP_CODES];
} voe_platform_keymap;

// A KEYMAP OUT OF WHICH NO KEY TYPES ANYTHING IS REFUSED, the same as a text
// with no `xkb_symbols` block at all — this is what makes "types nothing
// rather than wrong characters" a reported condition a caller can act on,
// rather than a silent one indistinguishable from a keyboard with nothing
// but dead keys and function keys on it.
//
// Reads size bytes of resolved XKB v1 text into *out, which is zeroed first.
// Returns false and leaves *out all zero when the text holds no
// `xkb_symbols` block, or when every key in it resolves to nothing —
// neither is a keymap this reader can turn into typed characters, and the
// difference is not this file's to report; the caller decides what to say.
// text need not be NUL-terminated; every read is bounded by size.
[[nodiscard]] bool voe_platform_keymap_read(const char *text, size_t size,
					    voe_platform_keymap *out);
