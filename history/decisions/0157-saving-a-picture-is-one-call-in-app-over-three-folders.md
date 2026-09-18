# 0157 Saving a picture is one call in `app`, over three folders, and a program may start with no window

Status: accepted
Date: 2026-09-15

Spec 003 wants a drawn frame written to a PNG file, and the editor started with no window so that
it draws one frame, writes the file and exits. Writing a file, encoding a PNG and reading a target
back are three different folders' work, and the spec also requires that a program which draws
nothing is not made to carry any of it.

## Decision

**Four folders, each doing its own half, and they are usable apart (criterion 3).**

1. **`platform` owns files, and gains one call: write a whole file.**
   `voe_platform_file_write(path, bytes, count, error)`. It is the first file API in this engine
   and it stays the smallest thing a caller needs: one buffer, one path, replace what is there.
   A path that cannot be opened is `VOE_BASE_ERROR_UNAVAILABLE` and a write that fails part way is
   `VOE_BASE_ERROR_REFUSED`; no new error code is added, because both are categories that already
   exist. Nothing is written anywhere the caller did not name — there is no default directory and
   no extension added.
2. **`assets` encodes, and still opens no file.** `voe_assets_png_encode` takes a
   `voe_assets_image` and an arena and hands back bytes, the mirror of the decoder beside it. The
   folder's rule that nothing in it opens a file is unchanged, and it is what lets the encoder be
   tested by decoding its own output rather than by writing to disk.
3. **`render` reads a target back** — ADR-0156.
4. **`app` ties the three together in one call**: `voe_app_capture_png(app, target, scratch, path,
   error)`. `app` is the folder a program already calls parts out of (ADR-0135), it is allowed to
   name `render`, `assets` and `platform`, and a capture is exactly a part a program calls in its
   own loop. It adds `assets` to its `DEPENDS`, which is an edge the map already allows; no new
   edge is created anywhere for this feature.

**A program may open with no window.** `voe_app_new_headless(arena, scratch, settings, error)`
opens the headless device alone. `voe_app_window` is then NULL — a caller that wants input asks
the window it did not get, and NULL is the honest answer — `voe_app_frame_open` skips the poll,
reports the settings' size and never reports closing, and the rest of the loop is unchanged. That
is what lets one program draw the same frame with or without a display, rather than growing a
second drawing path that has to be kept in step.

**A program that draws nothing links none of this.** Nothing is added to `base`, `math`, `ecs` or
`scene`; `platform`'s file call and `assets`' encoder pull in no graphics at all.

## Rejected

- Put the save in `assets`, reading the target itself — `assets` may not name `render`, and the
  edge would point the map backwards.
- Put it in `3d` — saving a picture has nothing to do with scenes, entities or meshes, and the
  editor would be made to name `3d` for a screenshot.
- No glue anywhere: let each program call the three folders itself — three calls and an
  intermediate buffer, written out in `editor`, in `dev` and in every game, each free to get the
  arena or the order wrong.
- A `platform` file API with open, seek, read and write — rule 10: a picture needs one write, and
  the rest is written when a loader needs it.
- Add a `VOE_BASE_ERROR_IO` code — the existing codes are categories and both failures already
  have one; what exactly happened goes in the reported line.

## Consequences

- `editor` gains a command line and a one-frame capture path (spec 003 criterion 4) through `app`
  alone; it names neither `assets` nor `platform` for it.
- The first `platform` file call fixes the shape of the ones after it: bytes and a path, failure
  returned, no handle type. A reader will be the second, and it is not written yet.
- `app`'s test folder gains a test that needs a graphics card, where it had only the clock's.
