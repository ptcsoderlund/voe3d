# 062 — Every reported problem goes through the one call

claimed-by: -
blocked-by: 061 (the call must exist), 052 (`dev/src/main.c` is open on that card)
status: todo
decision: *A recoverable problem is reported through one call in `base`, and where the lines go is a later step* (ADR-0140) point 7 — every existing site moves, in one card, because a half-migrated engine is worse than an unmigrated one: at the next step a console would show some of the engine's problems and be believed about all of them.

## Goal

No folder writes to stderr by hand any more. Every line an engine folder prints about a
recoverable problem goes through `VOE_BASE_WARNING` or `VOE_BASE_ERROR`, and the only
change a person sees is a level word at the start of each line.

## Scope

**Every `fprintf(stderr, ...)` in `render`, `assets`, `ui`, `3d`, `app` and `dev` moves.**
Around a hundred sites in a couple of dozen files; `render/src/device.c` and
`render/src/texture.c` are the two largest. The edit is the same one every time:

```c
fprintf(stderr, "render: vkCreateBuffer failed for %llu bytes (VkResult %d)\n",
	(unsigned long long)size, (int)result);
```
becomes
```c
VOE_BASE_ERROR("render", "vkCreateBuffer failed for %llu bytes (VkResult %d)",
	       (unsigned long long)size, (int)result);
```

- **The `"<module>: "` prefix leaves the message and becomes the first argument.** Where a
  file writes a second prefix — `"assets: sectioned: …"`, `"assets: glTF: …"` — **the module
  argument stays the folder** and the rest stays in the message: `("assets", "sectioned: …")`.
  A module name is a folder's name, so a later reader can filter by it.
- **The trailing `\n` leaves the message.** The call writes it.
- **Every one of these is `VOE_BASE_ERROR`.** They all report something that failed. Do not
  promote any of them to a warning on judgement — the only warnings in the engine are the
  drains ADR-0138 describes, and those arrive on cards 053 and 056.
- **The wording of every message is unchanged**, character for character after the prefix
  and before the newline. This card renames nothing and improves nothing.
- Add `#include <base/report.h>`; drop `#include <stdio.h>` where nothing else in the file
  needed it.
- Several files funnel many messages through one helper — `gltf_error`,
  `sectioned` parser errors. **Move the helper, not each of its callers.**

**What does not move.**

- `base/src/assert.c` — the fatal path, and not a report.
- `testing/include/testing/test.h` and everything under a `tests/` folder — the test
  framework's own output, read by `ctest`.
- Anything printing to `stdout`, if there is any. This card is about the error stream.

## What must not change

- **No dependency edge.** All six folders already depend on `base`; check the `DEPENDS` line
  before adding one, and if a folder somehow needs a new edge, **stop and report** —
  that is an architecture change and not this card's (ADR-0140).
- No behaviour. Nothing returns differently, nothing stops earlier, no site is deleted as
  redundant and no new site is added.
- `base/src/report.c` and `base/include/base/report.h` — card 061 wrote them and this card
  only calls them.
- No message text, no message order, no condition guarding a message.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest` passes.
- **`grep -rn 'fprintf(stderr' --include=*.c --include=*.h .` returns only
  `base/src/assert.c`, `testing/include/testing/test.h` and files under `tests/`.** Paste
  the result into Notes.
- **Run `dev` and compare its output to the same run before the change.** Every line must be
  identical but for a leading `error: `. Two ways in: run `dev` on `main` first and keep the
  output, or provoke the same failure both ways. Say in Notes which lines you actually saw
  and how.
- A format-string mistake introduced deliberately in one migrated site is a compiler warning.
  Show it in Notes, then remove it.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

One idiom for reporting a problem in the whole engine, an output that reads as it did but
says its level first, and one function where the destination will change when the editor
wants these lines in a panel.

## Notes
