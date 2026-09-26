# 03 — A WAV file decodes to float samples
folder: assets
decisions: 0168, 0265, 0266

## Change
Shape the call as `voe_assets_png_decode` in `assets/include/assets/image.h` is shaped: bytes
in, a struct out, false with a category and a report.

- New `assets/include/assets/sound.h`:
  - `voe_assets_sound { uint32_t rate; uint32_t channels; uint64_t frames; float *samples; }`
    — `samples` interleaved, `frames × channels` of them, in the arena.
  - `[[nodiscard]] bool voe_assets_wav_decode(const uint8_t *bytes, size_t size, voe_base_arena *arena, voe_assets_sound *sound, voe_base_error *error);`
  - Header points: what is read (RIFF/WAVE; `fmt ` tag 1 PCM 16-bit, tag 3 float 32-bit, tag
    0xFFFE extensible with either subformat; one or two channels; any rate above 0), that
    16-bit becomes `s / 32768.0f`, that unknown chunks are skipped and an odd-sized chunk has
    its pad byte, what is UNSUPPORTED (another tag or bit depth, more than two channels) and
    what is MALFORMED (bad magic, no `fmt `/`data`, a chunk running past the end, a `data`
    size not a whole number of frames, zero channels or rate); `sound` untouched on failure;
    why only WAV (0266 point 2).
- New `assets/src/wav.c` — the chunk walk and the two sample conversions.
- New `assets/tests/wav.c` — WAV bytes built in the test: mono 16-bit at 44100 with samples
  −32768, 0, 16384 decodes to −1, 0, 0.5; stereo float at 48000 round-trips exactly; an
  extensible float header decodes; a `LIST` chunk before `data` is skipped; 8-bit is
  UNSUPPORTED; three channels is UNSUPPORTED; a truncated `data` and a `RIFF` with `AVI ` are
  MALFORMED, each leaving `sound` as it was.
- `assets/assets.md` — the `sound.h` entry; the opening line's list of inputs gains WAV.
- `assets/src/src.md`, `assets/tests/tests.md` — entries for `wav.c`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder assets` prints `FINDINGS: 0`
   (runs `assets/wav`).
