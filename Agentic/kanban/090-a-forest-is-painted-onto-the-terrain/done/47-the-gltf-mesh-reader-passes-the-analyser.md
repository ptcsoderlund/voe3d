# 47 — The glTF mesh reader passes the analyser
folder: assets/src
after: none
decisions: 0168

## Change
`assets/src/model_gltf_meshes.c`: clang's static analyser reports two
findings, both from one cause. `mesh_primitives` (about line 89) returns at
its first check, `mesh == VOE_ASSETS_JSON_NONE`, through
`voe_assets_gltf_malformed` without writing `*array` or `*count`. That helper
is always false (its comment in `assets/src/model_gltf_read.h` says so), but
it lives in another file, so the analyser follows a path on which the call
"succeeds" and the callers then read the unwritten outputs: `count` in
`count_primitives` (about line 139) and `array` in `read_primitives` (about
line 466).

Fix it once, in `mesh_primitives`, not in its two callers: on entry, before
any return, it writes `*array = VOE_ASSETS_JSON_NONE` and `*count = 0`, so
both outputs are defined on every path. Its one-line comment adds that the
outputs are none and nought when it refuses. No `__clang_analyzer__`
suppression: the outputs being unset on a refusal is real, so the code says
so. Refusal wording and behaviour stay as they are; the existing `assets/`
glTF tests keep passing.

## Done when
From the repo root, this prints nothing:
`clang --analyze -Xanalyzer -analyzer-output=text -DVOE_BASE_DESCRIPTIONS=1 -Iassets/include -Imath/include -Ibase/include -std=c23 -o /dev/null assets/src/model_gltf_meshes.c 2>&1 | grep -F 'warning:'`
