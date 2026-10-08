# 37 — The new-messages test reads what the close will name
folder: render/tests
after: 36
decisions: 0168, 0358

## Change
Card 36 made the classifier keep each new message in a fixed table on the
device and `voe_render_best_practices_list_new` print it at close. Prove the
record on made-up messages, with no graphics card.

- New `render/tests/new_messages.c`. Includes `../src/device_internal.h` by
  relative path, as `card.c` includes `startup.h`; read the table's fields
  and the comment above `_classify` and `_list_new` in
  `render/src/device_internal.h` and `render/src/device_calls.h`. A zeroed
  static `voe_render_device` with `vendor_id` 0x10DE; no Vulkan opened.
  Cases, each a `VOE_TEST_CHECK` on the verdict, the rows and the counts:
  - an allowlisted warning id (`BestPractices-PushConstants`) is ALLOWED and
    keeps no row;
  - an AMD-tagged id is DROPPED and keeps no row;
  - one new warning id three times with three texts is one row, count 3,
    holding the first text;
  - an error with an allowlisted id is NEW and kept as an error;
  - a NULL id and a NULL text are kept, under "(no id)";
  - a text longer than the row holds is cut and terminated, not overrun;
  - more distinct ids than rows: the rest go to the overflow count, and
    `new_messages` equals the rows' counts plus the overflow;
  - `_list_new` called once on that device returns (its stderr is not read).
  Header comment: what is proved, and that it needs no graphics card.
- `render/tests/tests.md`: a `new_messages.c` entry, one phrase, "needs no
  graphics card".

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/tests`
prints FINDINGS: 0; it runs the test `render/new_messages`, which passes.

The human's, once this card is in: on Windows, debug preset, start the
editor on a project with a landscape, close it; then on one without, close
it. Above the assert each new message is now named by id, count and text;
file what each run names as a bug against 082, to be fixed or allowed by a
decision.
