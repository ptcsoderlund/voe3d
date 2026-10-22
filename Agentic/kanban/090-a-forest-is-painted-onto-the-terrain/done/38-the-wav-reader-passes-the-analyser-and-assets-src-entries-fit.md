# 38 — The WAV reader passes the analyser, and the assets/src entries fit the cap
folder: assets/src
after: none
decisions: 0168

## Change
`assets/src/wav.c`: clang's static analyser reports a division by zero at
`chunks.data_size % (sample_bytes * out.channels)` in the decode function
(about line 258): it follows a path on which `read_format` returned true yet
`out.channels` is 0. `read_format` refuses 0 channels (about line 198) and then
asserts `channels <= 2 && rate > 0`, which does not state `channels >= 1`.
Find the path the analyser takes (run the command under Done when) and make it
provably nonzero: either the path really skips the check, and the check then
covers it, or it does not, and the post-check `VOE_BASE_ASSERT` states
`channels >= 1` as well. No `__clang_analyzer__` suppression: the check is
real, so the code says so. Refusal wording and behaviour stay as they are;
the existing `assets/` WAV tests keep passing.

`assets/src/src.md`: shorten the entries `model_gltf_read.h` (306) and
`model_gltf_meshes.c` (307) to one sentence of at most 300 characters each
(the cap counts the whole `- `name` — ...` line). Any point an entry drops that
the file's header (`assets/src/model_gltf_read.h`,
`assets/src/model_gltf_meshes.c`) does not already make goes into it, kept
under its 60-line cap (measure with
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).

## Done when
From the repo root, this prints nothing:
`clang --analyze -Xanalyzer -analyzer-output=text -DVOE_BASE_DESCRIPTIONS=1 -Iassets/include -Imath/include -Ibase/include -std=c23 -o /dev/null assets/src/wav.c 2>&1 | grep -F 'warning:'`
and `bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING assets/src/'` prints nothing.
