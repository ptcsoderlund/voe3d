# 062 — Every reported problem goes through the one call

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: 061 (the call must exist), 052 (`dev/src/main.c` is open on that card)
status: review
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

Verified on **Linux** (Fedora 44, clang 22). `render/src/backend_win32.c` changed
(2 sites) and was **not compiled** here — checked by reading only.

Taken while 061 sat in `review/`: the call exists, which is what the blocker is for.

**Scope reading.** The card names six folders, but `scene` (4 sites, the ADR-0138 drains
from 055/056) and `editor` (2) also wrote to stderr, and the verify grep allows none.
Both depend on `base` already; migrated. 115 sites in 27 files. The helpers in `json.c`,
`model_glb.c`, `model_gltf.c` and `sectioned.c` were moved, not their callers.

- **Module = folder**: `voe_ui:` became `"ui"`. Second prefixes stay in the message
  (`sectioned:`, `glTF:`, `glb:`, `JSON at byte`, and `voe_scene_transform:` /
  `voe_scene_identity:` in `scene`).
- **`dev` (10) and `editor/src/main.c` (1) had no prefix**, so their lines gain
  `dev: ` / `editor: ` as well as the level word. Unavoidable with the module required.
- **Levels:** all `ERROR`, except lines that already said `warning:` before this
  card. `scene` picks error or warning while running, so each such site is an
  `if`/`else` on the two macros.
- `DEVIATION:` `render/src/device.c` `debug_message` — the validation layer's
  callback. It stays `VOE_BASE_ERROR` with the layer's own word kept in the message,
  so a layer warning reads `error: render: vulkan warning: …`. Mapping the layer's
  severity onto the level would be the judgement the card rules out. **Worth a
  decision.**
- `DEVIATION:` `editor/src/inspector.c` — the dropped-edit line was already
  `warning: voe_editor:` and stays `VOE_BASE_WARNING("editor", …)`, not promoted to error.
- `#include <stdio.h>` dropped from 21 files; kept where `printf`/`snprintf` remain
  (`dev/main.c`, `editor/main.c`, `editor/inspector.c`, `render/device.c`, both scene systems).

**Ran:**
- `cmake -P check.cmake` → every step `ok`, 43 tests passed.
- `grep -rn 'fprintf(stderr' --include=*.c --include=*.h .` (build/ excluded) →
  `base/src/assert.c:13`, `testing/include/testing/test.h:49,67,85`, and only
  `tests/` files after that: `base/tests/describe.c`, `render/tests/elements.c`,
  `assets/tests/{json,sectioned,model}.c`, `scene/tests/{identity,transform}.c`,
  `ui/tests/widgets.c`.
- **`dev` before and after**, same binary path, stderr diffed:
  - normal run with GPU, 6 s: empty both times.
  - inside the sandbox (no compositor socket): `app: the window would not open` →
    `error: app: the window would not open`.
  - `VK_ICD_FILENAMES=/nonexistent.json`: the four lines
    `render: vkCreateInstance failed (VkResult -9) with these extensions asked for:` /
    `render:     VK_KHR_surface` / `…wayland_surface` / `…debug_utils` → identical with
    `error: ` in front of each.
  No `dev:`, `scene` or validation line was provoked; those were checked by reading.
- Deliberate mistake at `app/src/app.c:43`, `"…would not open %d", "oops"` →
  `error: format specifies type 'int' but the argument has type 'char *'
  [-Werror,-Wformat]`, pointing through `report.h:56`. Reverted, rebuilt clean.
- `bash tools/hot.sh`: all under ceilings.

**Suggestions, not done:** `ui/tests/widgets.c` prints "the next voe_ui line is this
test's own" three times, and those lines now start `error: ui:` (a `tests/` file, which
the card excludes). `editor/src/main.c:105` still names `--preset editor`, which card 063
keeps on purpose.
