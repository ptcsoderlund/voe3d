# 02 — The editor swallows the desktop's keyboard

## Seen
"Keypresses are being blocked. While voe editor is open, I can't use the start menu or stuff in the
same virtual desktop on my Linux Fedora KDE. If I switch to another virtual desktop it works. If I
don't, I can't type until I close voe editor."

Seen on the sponsor's Fedora KDE Plasma desktop, the same machine 009 is being tested on. It is not
known yet whether this began with 009's Wayland change (card 03, the fractional scale) or was there
before; it is filed here because 009 is the feature open, and it stops 009 being tested in comfort.

## Expected
- While the editor is open, the rest of the desktop takes the keyboard exactly as it does with any
  other program open: the start menu opens with its key, other windows on the same virtual desktop
  take typing when they have focus, and desktop shortcuts work.
- The editor takes keys only while its own window has focus, and lets go of them the moment another
  window or the start menu takes focus.
- Typing in the editor itself keeps working as 008 left it.

## How to reproduce
1. On Fedora KDE Plasma, start the editor.
2. Without switching virtual desktop, press the start menu key, or click another window on the same
   virtual desktop and type in it.
3. The start menu does not open and the other window gets no typing.
4. Switch to another virtual desktop: typing works there.
5. Close the editor: typing works everywhere again.
