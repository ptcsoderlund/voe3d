# 15 — A project type names its need with a call of its own
folder: game
after: none
decisions: 0168, 0303

## Change
Bug 01: card 04's `needs` member made every pre-043 positional initialiser
a `-Wmissing-field-initializers` error under `-Werror`. 0303 moves the need
to a call. Read `game/include/game/project.h`, `game/src/project.c`,
`ecs/include/ecs/component.h` (lines 120-140 and 220-255: type count, type
at, key, needs, needs_set, type lookup), `game/tests/project.c` and
`game/tests/tests.md`.

- `game/include/game/project.h`: `voe_game_project_type` loses `needs`,
  back to its six members; its comment loses the `needs` sentences. New
  `[[nodiscard]] bool voe_game_project_component_needs(voe_ecs_world *world,
  const struct voe_ecs_key *key, const struct voe_ecs_key *needed);`: says
  rows of the project type `key` do nothing without a row of `needed`
  (0303), so Add component brings it and the Inspector keeps it. Its
  comment: `needed` is an engine type or a project type registered before;
  no call, no need; refusals as below. The header's example at the top
  registers with six members and then calls it with
  `&voe_scene_transform_key`. The "A REFUSAL IS REPORTED" paragraph names
  the new refusals.
- `game/src/project.c`: `voe_game_project_component` drops its `needs`
  lines. `voe_game_project_component_needs` finds both types by walking the
  world's types by key, without `voe_ecs_component_type`'s assert, and
  refuses with a `VOE_BASE_ERROR` line and false when `key` is not a project
  type (its index below `VOE_GAME_WORLD_TYPES` or not registered), `needed`
  is not registered, or `key` already has a need (`voe_ecs_component_needs`);
  else `voe_ecs_component_needs_set` and true. NULL world or keys assert.
- `game/tests/project.c`: the `needs` check becomes: a type registered with
  six positional members (the pre-043 shape, which the `-Wextra -Werror`
  build compiling this file proves) answers no need; after
  `voe_game_project_component_needs(world, &follow_key,
  &voe_scene_transform_key)` it answers the transform; a second call, an
  unregistered key and an engine type as `key` are each refused. The file's
  header comment says so. `game/tests/tests.md`: the `project.c` entry if
  its wording changes.
- `game/game.md`: only if it names the `needs` member.

## Done when
`game/tests/project.c` passes in the folder's checks, and the pre-043
projects that were not touched compile again:
`for f in examples/{capsule,coin_game}/Code/*.c; do clang -std=c23 -fsyntax-only -Wall -Wextra -Wpedantic -Werror $(printf -- '-I%s ' */include) -I"${f%/*}" "$f" || exit 1; done`
exits 0 from the repo root.
