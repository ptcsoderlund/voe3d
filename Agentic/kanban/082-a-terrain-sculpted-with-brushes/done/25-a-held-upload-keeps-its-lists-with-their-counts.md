# 25 — A held upload keeps its lists with their counts
folder: 3d/src
after: none
decisions: 0168

## Change
`3d/src/models.c` fails clang's static analyser (step 7 of `check.cmake`): two
`core.NullDereference` findings in `release()`, on `held->shadings[i]` and
`held->textures[i]`. The path the analyser walks: `voe_3d_models_load` ->
`voe_3d_models_entry_new` (stores NULL lists) -> `load_glb` -> `hold_upload`,
where it assumes the byte size `upload->texture_count * sizeof(...)` is 0
(wrap-around) while the count is not, so no list is pushed but the count is
stored -> `voe_3d_models_keep` on a full store -> `release()` walks the count.

Fix it at the owner, `hold_upload()` in `3d/src/models.c`: decide whether to
push each list on its count (`upload->texture_count > 0`,
`upload->shading_count > 0`), not on the computed byte size, so a list is
pushed exactly when its count is non-zero. `release()` and the callers stay
as they are. No suppression (`__clang_analyzer__`): the code is reshaped
because the guard tested the wrong quantity, not to quiet the tool. Touch no
other file; the header comment of `models.c` needs no change.

## Done when
From the repository root, after `checks.sh --folder 3d/src` has built
`build/debug`, this exits 0 (it prints nothing when the analyser is clean):

```sh
c=$(jq -r '.[]|select(.file|endswith("/3d/src/models.c")).command' build/debug/compile_commands.json | sed -E 's/ -(o|MT|MF) [^ ]+//g; s/ -(c|MD) / /g'); ! eval "$c --analyze -Xclang -analyzer-output=text -Wno-unused-command-line-argument -o /dev/null" 2>&1 | grep 'warning:'
```

and the `3d/` tests still pass under `checks.sh --folder 3d/src`.
