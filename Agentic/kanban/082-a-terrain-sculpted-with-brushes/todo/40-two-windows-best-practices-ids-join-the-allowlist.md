# 40 — Two Windows Best Practices ids join the allowlist
folder: render/src
after: 39
decisions: 0168, 0358, 0385

## Change
Bug 04, the last of its cards. 0385 adds two rows to `render`'s allowlist
for ids only the sponsor's Windows layer (1.4.304) raises; the scratch
layer (1.4.341) cannot. Cards 38 and 39 fixed the other two ids.

- `render/src/best_practices.c`, the `allowlist` table: two rows, each in
  its sorted place among the existing ids, each reason in the form the
  existing rows give one:
  - `BestPractices-vkAllocateMemory-small-allocation`: every buffer and
    image has its own allocation, no sub-allocator yet; the same cause as
    the two `small-dedicated-allocation` rows.
  - `BestPractices-pipeline-stage-flags2-compute`: barriers name
    `ALL_COMMANDS`; the barrier review waits for the frame breakdown's
    measurement (0358).
- Same file, header comment, the "A NEW MESSAGE NOW COUNTS" paragraph
  gains a point: a row may name an id only some layer releases raise, and
  a row the checks' own layer cannot raise is kept, not pruned (0385); no
  layer release is required of anyone.

## Done when
- `grep -c "BestPractices-vkAllocateMemory-small-allocation\|BestPractices-pipeline-stage-flags2-compute" render/src/best_practices.c`
  prints 2.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
  prints FINDINGS: 0.
- The human's, not the coder's: on Windows, debug preset, start the editor
  on a project, do nothing, close it; it ends with no assert at
  `render/src/device.c`. Then the same on a project with a landscape in
  view.
