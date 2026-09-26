# platform

The public headers, one entry each; the fuller account of every one of these
stays on `platform/platform.md`.

- `window.h` — opening a window, polling it, and taking back a close the user
  asked for.
- `input.h` — the keyboard and the mouse as polled state: keys, motion, the
  wheel, typed text, the pointer and its lock.
- `clock.h` — how long something took, in seconds, from a clock that only goes
  forward.
- `file.h` — reading a whole file into an arena, testing a path, and writing a
  whole file atomically.
- `folder.h` — listing a folder into an arena, making one, and finding the home
  and settings folders.
- `path.h` — joining, a path's parent, its last name, and resolving one to an
  absolute path.
- `library.h` — a shared library opened by name at run time, and a symbol out of
  it; a failed open is reported with the loader's reason.
- `sound.h` — the default sound device: its room, and writing stereo float
  frames into it.
- `process.h` — starting a program with its output shared or appended to a file,
  polling it without waiting, and ending it with everything it started.
- `arguments.h` — the program's own arguments as UTF-8 on both platforms.
