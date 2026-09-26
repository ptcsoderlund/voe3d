# 11 — ALSA's open is declared without a double pointer
folder: platform/src
decisions: 0168, 0265

## Change
Only `platform/src/sound_wayland.c`. Its `pcm_open_fn` typedef (line 35) declares ALSA's
`snd_pcm_open` as taking `snd_pcm **pcm`; the checks allow one level of dereference only, and
its header (lines 6–7) says the `**` is kept.

- The typedef's first parameter becomes `void *pcm`: the address of a `snd_pcm *`, which the
  ABI passes exactly as ALSA's `snd_pcm **`. The call site `pcm_open(&sound->pcm, ...)` needs
  no change (an object pointer converts to `void *`).
- The header's sentence about `**` is replaced by one saying snd_pcm_open's out parameter is
  declared as `void *`, the address of the device pointer, the same at the ABI, because the
  engine never spells a pointer to a pointer.

## Done when
1. `grep -nE '\*[[:space:]]*\*' platform/src/sound_wayland.c | grep -vE '^[0-9]+:[[:space:]]*(/?\*|//)'`
   prints nothing.
2. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/src` prints `FINDINGS: 0`
   (it builds `platform` and runs every `platform/` test, `sound` among them).
