# Needs decision — 032: how the engine makes sound

The engine has no audio today (ADR-0009: audio is "later, not never"). Playing the coin sound
needs three things the tree lacks: a way to hand samples to the operating system, a decoder for
the sound file, and a mixer so two pickups overlap. The first two are not local to this feature:
they fix an OS API every later sound will go through, a folder and dependency edges in
`cmake/voe.cmake` (system.md: a new edge is a decision), and possibly a third-party library
(ADR-0023: the principal's call). So the planner stops here.

## Question
Where does sound output live, which OS API does it speak on Linux (and Windows), and is it
written by us or adopted?

## Options

1. **Written by us; output in `platform`, mixing in a new `audio` folder.** `platform` gains
   `sound.h`: open the default output device at 48 kHz stereo float and pull frames from a
   callback on the OS's audio thread. Linux talks to PipeWire via `libpipewire-0.3` (linked,
   programmer-installed, like `wayland-client`); Windows via WASAPI (a system library).
   `assets` gains a WAV (PCM 16-bit / float) decoder, buffer in, samples out. A new folder
   `audio` (depends on base, platform, assets; used by game and editor) owns a fixed set of
   voices: `play` starts a new voice each call, so two pickups overlap; it resamples/converts
   to the device format. Cost: two OS backends to write and one new folder with its edges.
2. **As option 1, but Linux speaks ALSA (`libasound`) instead of PipeWire.** ALSA exists on
   every Linux desktop (PipeWire and PulseAudio both expose it), its blocking-write API is
   simpler to write against; it has higher latency and fewer routing features on PipeWire
   desktops.
3. **Adopt miniaudio (one public-domain C file, vendored or fetched).** It does the device on
   every OS, WAV/FLAC/MP3 decoding and mixing; `audio` becomes a thin wrapper. Cheapest by far,
   but it is the third-party dependency ADR-0023 says no to by default.

## Recommendation
Option 2 for the first sound, shaped as option 1: `platform/sound.h` with ALSA on Linux and
WASAPI on Windows, a WAV decoder in `assets`, voices in a new `audio` folder between `platform`
and `game`. It keeps ADR-0023, is the smallest OS surface to write, and the `platform` header
lets a PipeWire backend replace ALSA later without touching a caller. The coin's sound then
rides on a component (say `SoundOnPickup { path }`) edited in the Inspector, and Ship copies the
`.wav` beside the program — those parts the planner can decide once this is settled.
