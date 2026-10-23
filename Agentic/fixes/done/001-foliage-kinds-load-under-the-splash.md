# 001 — A landscape's foliage kinds load under the splash
folder: game

## Seen
After the splash closes, the editor stops responding while the trees load: a landscape's foliage kinds and
their materials load in the first frame instead of under the splash. The splash counts "2/2". (Bug 13 of
090: `Agentic/completed/090-a-forest-is-painted-onto-the-terrain/bugs/13-the-editor-stops-responding-after-the-splash-while-the-trees-load.md`.)

## Expected
A landscape's foliage kinds and their materials load under the splash, which counts them, and the editor
responds as soon as the splash closes.

## Change
In `voe_game_models_update` (`game/src/models.c` 261–304) the foliage kinds are gathered once, by
`voe_game_models_kinds_gather` at :278, and counted at :280, before the file loop at :281–298. The gather
(:156–182) looks only at landscapes already in the store (:164–168). In the editor the landscape
(`Hill.landscape`) is itself a model row loaded inside that loop, so under the splash the gather finds no
kinds, the total is only the files, and the kinds' `.glb` are left to the next frame's update. The game does
not hit this, because `game/src/run.c:170` loads the cooked landscapes before the update at :173.

The fault: kinds named by a landscape that loads during this update are neither gathered nor counted in the
same call. Make one update load them: after a landscape loads in the loop, gather its kinds, add them to the
progress total, and load them in the same call, so that the materials call that follows
(`game/src/models_materials.c:369`, called from `editor/src/models.c:227` under the splash) finds them.
The change stays in `game/`; `editor/` already passes the splash's progress (`editor/src/models.c:211`,
`editor/src/loading.c:48, 67–69`) and is not changed. Keep `game/include/game/models.h`'s description of
update and progress true.

Feature 093's card 06 also changes `update` in `game/src/models.c`; this fix lands first on the same branch.

## Done when
A test under `game/` builds a store whose only model row is a landscape naming a foliage kind `.glb`, runs
one `voe_game_models_update` with a progress, and finds the kind's model loaded after that one call and the
progress total counting it. The folder's check in CLAUDE.md passes for `game`.
