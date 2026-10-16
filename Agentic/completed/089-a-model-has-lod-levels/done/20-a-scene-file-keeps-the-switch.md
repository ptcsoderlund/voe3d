# 20 — A scene file keeps the switch
folder: authoring
after: 19
decisions: 0168, 0428

## Change
Card 19 gave `voe_scene_identity` a fourth described field, `bool off`,
after `folded`. Authoring writes, reads and cooks identities from their
description, so no source in `authoring/src/` changes: the file now carries
`off = false` after every `folded = …` line, and a cooked identity carries
`.off = false`. Only the tests that spell an identity out change.

Expected texts to update, adding the `off` line right after each `folded`
line (or `.off = false` after `.folded = …` where the expected value is a
struct): `authoring/tests/prefab.c`, `authoring/tests/scene_cook.c`,
`authoring/tests/scene_lights.c`, `authoring/tests/scene_read.c`,
`authoring/tests/scene_read_former.c`, `authoring/tests/scene_read_unsaid.c`,
`authoring/tests/scene_write.c`. `grep -n folded` in each finds every place;
a text given as input to a read (not an expected output) needs no `off`
line, since a missing key reads on.

New cases in `authoring/tests/scene_read.c`, beside
`test_folded_round_trip` and `test_no_folded_key` and shaped like them:
- `test_off_round_trip`: an identity with `off = true` is written as
  `off = true` and read back off.
- `test_no_off_key`: a file with no `off` key reads every identity on.
Both called from `main`. `authoring/tests/tests.md`'s `scene_read.c` entry
names the switch with the fold, one sentence, under 300 characters.

The example scenes and prefabs under `examples/` are not changed: they read
on without the key.

## Done when
The tests `authoring/prefab`, `authoring/scene_cook`,
`authoring/scene_lights`, `authoring/scene_read`, `authoring/scene_read_former`,
`authoring/scene_read_unsaid` and `authoring/scene_write` pass after the
folder's build, with the two new cases in `scene_read`.
