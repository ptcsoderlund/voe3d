# 03 — App times a start's steps and writes them out
folder: app
after: 01
decisions: 0168, 0345

## Change
A new module in `app`, beside `clock.h`.

- `app/include/app/start_log.h` (new): a `voe_app_start_log` value holding up to
  `VOE_APP_START_STEPS` (16) steps, each a name (a string the caller keeps alive)
  and seconds, plus the last mark. Calls:
  - `void voe_app_start_log_begin(voe_app_start_log *log)` — zeroes it and, when
    `voe_platform_clock_launched` answers, records a first step "before main"
    from the launch to now; marks now.
  - `void voe_app_start_log_step(voe_app_start_log *log, const char *name)` — the
    time since the last mark becomes step `name`; marks now. A 17th step asserts.
  - `double voe_app_start_log_total(const voe_app_start_log *log)` — the steps'
    sum.
  - `[[nodiscard]] bool voe_app_start_log_write(const voe_app_start_log *log, const char *program, const char *path, voe_base_arena *scratch)`
    — one stderr line per step and one for the total, each starting with
    `program`; when `path` is not NULL, the same lines appended to that file
    (read whole, emptied first when over 64 KiB, written whole through
    `platform/file.h`'s atomic write). False only when the file write failed,
    with platform's line on stderr.
  The header says: why (where a slow start went, 0345), that the first step is
  the time before the program's own code ran when the OS can say, that the steps
  add up to the time from launch to the last mark, and the format of a line
  (name, seconds to milliseconds).
- `app/src/start_log.c` (new): those four calls.
- `app/app.md` and `app/src/src.md`: one entry each for the header and the file.
- `app/tests/start_log.c` (new): `steps_in_order_add_up` — begin, two named steps,
  the names come back in order after "before main", every seconds at or above
  nought, the total their sum; `file_holds_both_starts` — write twice to a file in
  a scratch folder (as `app/tests/capture.c` makes its output path), the file holds
  both blocks' total lines; `file_over_limit_is_emptied` — a file of 65 KiB
  written to holds only the new block. List it in `app/tests/tests.md`.

## Done when
The three cases in `app/tests/start_log.c` pass.
