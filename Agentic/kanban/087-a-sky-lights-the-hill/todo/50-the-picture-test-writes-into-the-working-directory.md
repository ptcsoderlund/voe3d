# 50 — The picture test writes into the working directory
folder: app/tests
after: 42, 43
decisions: 0168, 0413

## Change
Bug 05: `app/tests/picture.c`'s `temp_path` (about line 45) calls `getenv("TMPDIR")` and `getenv("TEMP")`,
deprecated by the Windows headers. The test writes its two files into the working directory instead (0413
point 4), as `authoring/tests/scene_cook.c` does: `temp_path` goes, and `png_path`, `text_path` and the
no-such-file path are the bare names it was given (`voe_app_picture_test.png`, `.txt`,
`voe_app_picture_no_such_file.png`). Drop `<stdlib.h>` if nothing else needs it. Header comment: the files
go in the working directory, not the system temp folder; the TMPDIR/TEMP line goes. The removal at the end,
pass or fail, stays. `app/tests/tests.md`: fix the entry if it names the temp folder.

## Done when
- `grep -nE '\bgetenv[[:space:]]*\(' app/tests/picture.c` prints nothing.
- Test `app/picture` passes.
