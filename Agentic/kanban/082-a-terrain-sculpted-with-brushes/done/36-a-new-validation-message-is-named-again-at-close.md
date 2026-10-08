# 36 — A new validation message is named again at close
folder: render/src
after: 35
decisions: 0168, 0358

## Change
Bug 03: on Windows the editor asserts at close on "31 new validation
messages", and none of the 31 is among the lines the sponsor can see; every
printed line is "(allowed)". Linux lands clean (3d's `models_landscape`
draws a landscape and closes through the same gate), so the 31 are likely
vendor-tagged or Windows-driver messages this machine never raises. The fix
here is that the close names them: the device keeps what each new message
was and prints it again right above the count line and the assert, whatever
happened to the line printed when it arrived. The allowlist does not change
(adding a row is a decision, 0358).

- `render/src/device_internal.h`, the Best Practices fields beside
  `new_messages`: a fixed table of the new messages kept, one row per
  distinct id name, up to eight rows. A row: the id name copied into a
  bounded buffer ("(no id)" for a NULL id), whether it was an error, how
  many times it came, and the first message text cut to a few hundred
  bytes. Plus a count of new messages whose id found no free row. Comment
  points: fixed and in the struct because a messenger callback must not
  allocate or fail; written where `new_messages` is counted, so it shares
  that counter's threading; read once, at close.
- `render/src/device_calls.h`: `voe_render_best_practices_classify` gains
  `const char *message` (may be NULL) after `id_name`; new
  `void voe_render_best_practices_list_new(const voe_render_device *device)`.
  The comment block above them says what `_list_new` prints and when.
- `render/src/best_practices.c`: `_classify`, for a NEW verdict only, finds
  or takes the id's row and counts it, keeping the text of the first; a
  full table counts into the overflow. `_list_new` prints one
  `VOE_BASE_ERROR("render", ...)` per kept row — error or warning, the id,
  how many times, the first text — and one more for the overflow when it is
  not zero. Header comment gains a point: a new message is named again at
  close, and why (the line printed on arrival may be far above or lost; the
  line above the assert must say what it counts — bug 03).
- `render/src/instance.c`: `debug_message` passes `data->pMessage` to
  `_classify`. Header's "A NEW MESSAGE NOW COUNTS" paragraph: the text goes
  to the classifier too.
- `render/src/device.c`, `voe_render_device_destroy`: after `close_down`
  and before `release`, when the count is not zero, call `_list_new`; the
  count line and the assert stay as they are, after it. The comment above
  the function says the kept messages are listed before the count.
- `render/src/src.md`: the `best_practices.c` entry gains "keeps each new
  message and names it again at close".

## Done when
- `grep -n "voe_render_best_practices_list_new" render/src/device.c` prints
  one line.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
  prints FINDINGS: 0.
