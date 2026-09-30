# audio

The public headers, one entry each.

- `mixer.h` — voices started by path with a handle to stop, tune or move them, looping, pitched, held and placed from a listener, mixed into 48 kHz stereo and pumped to the device; its header says why a stale handle is harmless and the steal rule.
- `place.h` — the listener made from a camera, and the falloff, screen-x pan and balance law that give a world point its left and right gains.
- `sound_component.h` — a sound a thing carries: a described row, its replace intent, a play, stop and tune control, and the runtime-only row holding its voice.
- `sound_system.h` — the run: drains, voice rows added and dropped, the listener from the first camera, each sound started, stopped, tuned and moved, and the sweep.
