# src

`audio`'s implementation.

- `mixer.c` — the clip list, the voices and their handles, start, stop, tune,
  move, the listener, sweep, the mix and the pump. Its header says which arena holds what and why
  a failed path is kept as a clip.
- `place.c` — the listener's reference from its view axis and the gains of a
  point: falloff, screen-x pan and the balance law.
- `voice.c` — one voice read at its pitch and summed into a mix, looping
  without a seam and ramping its gain.
- `voice.h` — private: the clip and voice structs `mixer.c` keeps, and the
  read-and-sum `voice.c` gives it.
