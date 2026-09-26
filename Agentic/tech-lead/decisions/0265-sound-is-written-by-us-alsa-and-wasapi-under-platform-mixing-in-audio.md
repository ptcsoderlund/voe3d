# 0265 — Sound is written by us: ALSA and WASAPI under `platform`, mixing in a new `audio` folder
date: 2026-09-26
by: tech-lead

## Decision
The engine plays sound with code we write (ADR-0023 holds; no audio library is adopted).
`platform` gains `sound.h`: open the default output device, 48 kHz stereo float, and fill it with
frames. On Linux it speaks ALSA (`libasound`), **loaded at run time** with the platform's
library loader rather than linked, so building needs no audio package and a machine without
ALSA or a sound device runs silent with one line on stderr. On Windows it speaks WASAPI, a system
library. `assets` gains a WAV decoder (PCM 16-bit and 32-bit float), buffer in, samples out. A
new folder `audio`, depending on `base`, `platform` and `assets` and used by `game` and `editor`,
owns the mixer: a fixed set of voices, where each `play` starts a new voice so sounds overlap,
converting to the device's format. The mixer works in 32-bit float from voice to device, and
everything the ear hears passes through it, so later effects (reverb, equaliser, filters,
distortion, volume per sound) are work inside `audio` and never touch the OS backends. The new
folder and its edges go into `cmake/voe.cmake`.

## Reasoning
ALSA is the smallest OS surface to write and reaches every Linux desktop (PipeWire and
PulseAudio both accept it); its extra latency does not matter for game sounds, and a PipeWire
backend can replace it later behind the same `platform/sound.h`. Loading it at run time keeps the
programmer's install list at Clang, CMake, Ninja and slangc. Keeping the device dumb (it only
receives finished float samples) and doing all mixing ourselves is what makes effects a later
feature rather than a later rewrite. Rejected: PipeWire first (more code, callback-driven, harder
to get right for one coin sound); miniaudio (least work, but the first exception to ADR-0023 and
every later sound would rest on it); linking `libasound` (adds a build-time package).

## Replaces
nothing
