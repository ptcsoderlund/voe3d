# 061 — `base` reports a recoverable problem through one call

claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -
status: review
decision: *A recoverable problem is reported through one call in `base`, and where the lines go is a later step* (ADR-0140) points 1 to 6, 8 and 9. Card 062 moves every existing site onto it; this card adds the call and moves nothing.

## Goal

`base` has one reporting call. It writes to stderr. No other folder changes, and no
existing `fprintf(stderr, ...)` is touched — that is card 062's whole job.

## Scope

**1. `base/include/base/report.h` — new.**

```c
typedef enum {
	VOE_BASE_LEVEL_WARNING,
	VOE_BASE_LEVEL_ERROR,
} voe_base_report_level;

// Public because the macros expand to it; not meant to be called directly.
[[gnu::format(printf, 5, 6)]]
void voe_base_report_at(voe_base_report_level level, const char *module,
			const char *file, int line, const char *format, ...);

#define VOE_BASE_WARNING(module, ...) \
	voe_base_report_at(VOE_BASE_LEVEL_WARNING, (module), __FILE__, __LINE__, __VA_ARGS__)
#define VOE_BASE_ERROR(module, ...) \
	voe_base_report_at(VOE_BASE_LEVEL_ERROR, (module), __FILE__, __LINE__, __VA_ARGS__)
```

- **Exactly two levels.** Nothing below warning has a caller (ADR-0140 point 9).
- **The module is its own argument**, never text inside the format (point 2).
- **The message carries no trailing newline.** The call writes it.
- `[[gnu::format]]` is the C23 attribute spelling `assert.h` already uses for
  `[[noreturn]]`. **If Clang rejects it here, stop and report rather than dropping it** —
  the checking is the point (point 6).

**2. The header's own paragraphs.** Written the way `assert.h` and `error.h` are: what this
is *for* — a problem the world caused, which the caller could not prevent and a person can
act on — and where the line runs between it and an assert, pointing at `assert.h` rather
than restating it. Say the four things a reader will otherwise get wrong: the module is an
argument; the message has no newline; **the file and line are captured and not printed**,
and why (point 3); and a long message is truncated rather than allocated for.

**3. `base/src/report.c` — new.** One function.

- Compose `"<level>: <module>: <detail>\n"` into a **fixed buffer on the stack**, `warning`
  or `error` for the level, then write it to stderr in **one** call and flush.
- **A message too long is truncated**, not allocated for and not dropped. Pick the buffer
  size, say in the header what it is, and end a truncated line so a reader can see it was
  cut.
- The file and line are parameters and are **not** written.
- The one write is what keeps a line whole between threads; the composed line is what makes
  a later destination a change inside this function (ADR-0140 point 4). Say both in a
  comment, briefly.

**4. `base/include/base/error.h`** — its worked example currently shows a hand-written
`fprintf`. Rewrite that example to `VOE_BASE_ERROR`. Nothing else in the header changes.

**5. Tests — `base/tests/report.c`.** Test what can be tested without capturing stderr:
that a message at the buffer's length is not truncated and one past it is, that the composed
line is what point 5 says, and that both levels produce their own word. **If that means the
composition is its own small function the test can call, do that** and keep the writing in
the caller — say so in Notes.

**6. `base/base.md`** — entries for `include/base/report.h` and `src/report.c`, in the voice
the rest of the file uses.

## What must not change

- **No `fprintf(stderr, ...)` anywhere outside `base/src/report.c` is edited.** Card 062
  does that, and doing any of it here makes both cards unreviewable.
- `base/src/assert.c` and `testing/include/testing/test.h`. Neither is a report.
- No folder but `base`. No CMake edit — `src/` and `tests/` are globbed.
- No destination, no callback, no sink, no filtering, no third level, no timestamp, no
  thread id. Every one of those is a later step with a named condition (ADR-0140).

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R base` passes.
- A scratch program calling both macros prints exactly
  `warning: demo: something (7)` and `error: demo: something (7)` and nothing else. Paste
  both lines into Notes.
- Passing an argument that does not match the format is a **compiler warning**. Show it in
  Notes, then remove the line.
- `grep -rn 'fprintf(stderr' base/src` returns `assert.c` only.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

One call every folder may use, with the compiler still checking its arguments, and an engine
that behaves exactly as it did this morning because nothing calls it yet.

## Notes

Verified on **Linux** (Fedora 44, clang 22). Windows not checked — nothing here is
platform-specific beyond `fwrite`/`fflush` on stderr.

- **The composition is its own function**, as the card allowed:
  `voe_base_report_compose` in the internal header `base/src/report_line.h`, which
  `tests/report.c` includes as `"../src/report_line.h"` (the idiom `render/tests` and
  `assets/tests` already use). `voe_base_report_at` composes and writes.
- Buffer: a line is at most **1024 bytes including its newline**; a longer one is cut
  there and its last three bytes before the newline become `...`. The constant lives in
  `report_line.h`; `report.h` states the number in prose. A prefix that alone fills the
  line still gets the mark — tested.
- `[[gnu::format(printf, 5, 6)]]` accepted by clang in `-std=c23`; no fallback needed.
- The write is `fwrite` + `fflush`, so `grep -rn 'fprintf(stderr' base/src` →
  `base/src/assert.c:13` only.
- `cmake -P check.cmake` → every step `ok`, tests 43 passed (base 4).
  `ctest -R base` → 4/4 passed.
- Scratch program calling both macros with `"something (%d)", 7`, stderr exactly
  (stdout empty):

      warning: demo: something (7)
      error: demo: something (7)

- Mismatched argument in a scratch file (never in the tree):

      bad.c:4:43: warning: format specifies type 'int' but the argument has type 'char *' [-Wformat]
          VOE_BASE_ERROR("demo", "something (%d)", "seven");

- `bash tools/hot.sh` from the root: all hot files under their ceilings.
- No `DEVIATION:` or `BLOCKED:` markers.

**Suggestion, not done:** `base/include/base/assert.h` still says recoverable
failures' reporting "is not decided yet". That sentence is now stale and should point
at `report.h`; the card did not name the file, so it is left for a card.
