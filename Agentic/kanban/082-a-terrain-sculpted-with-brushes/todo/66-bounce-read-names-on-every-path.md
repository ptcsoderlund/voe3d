# 66 — bounce_read's draw_frame names on every path
folder: render/tests
after: none
decisions: 0168

## Change
`render/tests/bounce_read.c`, `draw_frame` (line ~212): the clang analyser
(`cmake -P check.cmake`, step analyser) finds that its early `return false`
(line ~228) leaves `named` unwritten, so the check at line ~306,
`named[0] == VOE_RENDER_NO_BOUNCE`, reads a garbage value.

Fix it in `draw_frame`, not in its callers: before any return, fill every
entry of `named` with `VOE_RENDER_NO_BOUNCE`, so a frame that names nothing
says so on every path. The existing writes from the pass's block
(line ~256) still overwrite it. Its comment above says `named` holds
`VOE_RENDER_NO_BOUNCE` for a volume the pass does not name, including when
the frame returns early. No other file changes; no test assertion changes.

## Done when
`checks.sh --folder render/tests` reports no finding, its analyser step
included, in `render/tests/bounce_read.c`.
