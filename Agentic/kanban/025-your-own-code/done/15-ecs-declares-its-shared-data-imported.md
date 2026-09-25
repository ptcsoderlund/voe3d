# 15 — ecs declares its public data imported
folder: ecs
decisions: 0168, 0245

## Change
Point 2 of 0245, the first user of card 14's `base/imported.h`.

- `ecs/include/ecs/component.h` — include `base/imported.h`; `voe_ecs_runtime_only` and
  `voe_ecs_description_compiled_out` gain `VOE_BASE_IMPORTED` after `extern`. Where the header says
  what these objects are, one point: a project's library imports them on Windows (0245), so their
  addresses are taken at run time there.
- `ecs/include/ecs/ecs.md` — only if its entry for `component.h` names these objects' declaration.

## Done when
1. `checks.sh --folder ecs` prints `FINDINGS: 0`.
