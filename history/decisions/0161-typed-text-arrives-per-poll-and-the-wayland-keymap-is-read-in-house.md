# 0161 Typed text arrives as UTF-8 per poll, and the Wayland keymap is read in-house

Status: accepted
Date: 2026-09-15

Spec 004's file browser makes a new folder from a typed name. Nothing in the engine reads typed
text (D-245, opened by ADR-0136): `platform` reports keys as places, `window_win32.c` ignores
`WM_CHAR`, and `window_wayland.c` closes the keymap it is sent unread, because turning an evdev
scancode into a character on Wayland needs the XKB keymap and the usual reader is `xkbcommon`.
`platform/input.h` says the card that brings text replaces its "no text" decision rather than
adding a queue beside it. Closes D-245.

## Decision

**Typed text is polled like the wheel.** `platform/input.h` gains one read: the UTF-8 bytes
typed since the previous `voe_platform_window_poll`, in the order typed, from a fixed buffer
in the input state that the poll empties; losing focus empties it too. What is typed past the
buffer's end in one frame is dropped. Keys stay places; editing keys (Backspace, Enter) are
places in the key list like every other, read as level state by the caller.

**No character is produced while Control is held**, so a shortcut does not also type.

**Windows reads `WM_CHAR`**, joining surrogate pairs and dropping control characters.

**Linux reads the keymap itself.** The compositor's `wl_keyboard.keymap` text (XKB v1, already
resolved — no includes) is mapped and read by an in-house reader in `platform/src/`: the
`xkb_keycodes` block's names and aliases, and the `xkb_symbols` block's first group, levels one
and two. A keysym name becomes a code point through a table of the Latin-1 names, `U<hex>` and
`0x0100<hex>`; any other name — a dead key, a function key — types nothing. Shift chooses level
two. Caps Lock, AltGr (level three), compose and dead keys are not read; the header names each.
The reader is a plain loop with fixed limits and no recursion, and is tested from a keymap text
in the test itself, needing no display.

**No key repeat on Wayland.** `wl_keyboard` is bound at version 1, which has no repeat
information; a held key types once. Windows repeats, because `WM_CHAR` does.

## Rejected

- `xkbcommon` — a new dependency (rule 5) and a new required install on Linux, for a first
  caller that types folder names.
- An ordered event queue for all input — the rest of input is state and has callers that want
  state; text alone needs order, and within one frame a byte buffer is that order.
- Letters by key position on a US layout — types the wrong character on most of the world's
  layouts, silently.

## Consequences

- Layouts that need AltGr or dead keys for a character cannot type it on Linux yet. Each is an
  addition to the reader, not a change of shape.
- A text field is the caller's composition: `ui` is unchanged, and the editor builds its name
  field from a label and the typed bytes.
- `window_wayland.c`'s and `input.h`'s "no text" paragraphs are replaced, not appended to.
