# 04 — The editor lets go of the desktop's keyboard
folder: platform
decisions: 0168, 0180

## Change
Bug 02: while the editor is open on the programmer's KWin 6.7 (Fedora, Intel RPL-P + NVIDIA RTX 4070 laptop),
the start menu key and typing in other windows on the same virtual desktop do nothing; on another virtual
desktop, or with the editor closed, they work. `platform` owns the window and the seat, so it owns this until
the evidence below says otherwise.

What is already known, from a `WAYLAND_DEBUG=1` run of the current build: the editor binds no protocol that
can hold a keyboard (no `zwp_keyboard_shortcuts_inhibit`, no text-input, no pointer lock in the editor; only
`dev` calls `voe_platform_input_lock_pointer`). It gets `wl_keyboard.enter` and an activated configure, then
commits about once per refresh through Mesa's `wp_fifo_v1` and `wp_linux_drm_syncobj` (with a 150 preferred
scale that run). So no request of ours takes keys; the suspect is the compositor being held up while our
surface is on screen, which is also why another virtual desktop (no repaint, the fifo barrier holds us) frees
it.

1. Find it. Measure, each with the program started by `timeout 8 …` and measured from 2 s in, the wall time of
   `for i in 1 2 3 4 5; do wayland-info >/dev/null; done` (use `/usr/bin/time -f %e`), and read
   `journalctl --user -b --since -1min | grep -i -E 'kwin|voe'` afterwards. Record a baseline with nothing
   running, then:
   a. `./build/debug/editor/voe_editor` at HEAD.
   b. `./build/debug/dev/voe_dev` at HEAD.
   c. The editor built from `74e3009` (before card 03) in a `git worktree` under your scratch folder.
   d. The editor at HEAD with `VK_LOADER_DRIVERS_SELECT='*intel*'` and again with `'*nvidia*'`.
   A run whose `wayland-info` time is several times the baseline, or that leaves a KWin warning, is the holder.
   Write the table and your reading of it under a new `## Found` on this card.
2. Fix it, if the owner is `platform`: the fault is in `src/window_wayland.c` (for instance something card 03
   added — `set_destination` or the fractional-scale object — sent more often than the logical size or scale
   changes, or held across a configure). Fix it there, once, and say in that file's header, in the paragraph the
   fix belongs to, what the compositor needs from us and why.
3. If the owner is not `platform` (only the editor holds it: `editor`; only one GPU holds it, or `dev` too:
   `render`'s device choice or present), change nothing, put the evidence in `## Found`, and move the card to
   `blocked/` with `## Blocked` naming the folder and function. If no run differs from the baseline, block the
   same way with the table: the planner needs it to ask the sponsor.

## Done when
- `## Found` holds the table from step 1 for all six runs.
- After the fix, run (a) again: its `wayland-info` time is within twice the baseline and the journal has no new
  KWin line naming the editor. Add both numbers to `## Found`.
- `timeout 4 env WAYLAND_DEBUG=1 ./build/debug/editor/voe_editor 2>&1 | grep -c set_destination` is between 1 and
  the number of configures in the same log (it still follows the logical size, and no more often).
- `checks.sh --all` exits 0.

For the human, on Fedora KDE Plasma, with `./build/debug/editor/voe_editor` open and not switching virtual
desktop:
1. The start menu key opens the start menu.
2. Click another window on the same desktop and type: it takes the typing; a desktop shortcut works.
3. Click back into the editor: typing in a text field works as 008 left it, and the text is still as sharp as
   card 03 left it.

## Found
Each program run as `timeout 8 …`, measured from 2 s in: wall time of five `wayland-info` in a row
(`/usr/bin/time -f %e`), then `journalctl --user -b` since the run started, grepped for `kwin|voe`. Two passes.

| run                                             | pass 1 (s) | pass 2 (s) | KWin/voe journal lines |
|-------------------------------------------------|-----------:|-----------:|------------------------|
| baseline, nothing running                       | 0.01 | 0.02 | none |
| (a) `voe_editor` at HEAD                        | 0.01 | 0.02 | none |
| (b) `voe_dev` at HEAD                           | 0.02 | 0.02 | none |
| (c) `voe_editor` at `74e3009` (before card 03)  | 0.01 | 0.01 | none |
| (d) editor, `VK_LOADER_DRIVERS_SELECT='*intel*'`  | 0.01 | 0.02 | none |
| (d) editor, `VK_LOADER_DRIVERS_SELECT='*nvidia*'` | 0.02 | 0.02 | pass 2 only: `kwin_wayland_wrapper: error in client communication (pid 872877)` |

Three more interleaved nvidia/intel pairs: 0.01–0.02 s each, no journal line, so the one nvidia line did not
repeat; its pid was gone by then and is most likely the editor's own connection torn down by `timeout`'s SIGTERM.

Reading: no run is slower than the baseline (all within 0.01 s, the timer's resolution), and none leaves a
repeatable KWin warning. The compositor answers new clients just as fast while our surface is on screen,
whichever GPU draws it and whether or not card 03's code is in. `wayland-info` measures the compositor's socket
dispatch, not its input routing, so this does not rule out a hold on KWin's keyboard path only.

`timeout 4 env WAYLAND_DEBUG=1 ./build/debug/editor/voe_editor` at HEAD: 1 `set_destination`, 2
`xdg_surface.configure`, 1 `preferred_scale(150)`, 1 `wl_keyboard.enter`, 0 `leave`, 488 `wl_surface.commit`
(about 120 per second, one per refresh). `set_destination` already follows the logical size and no more often,
so there is nothing in `src/window_wayland.c` to fix on this evidence. Nothing was changed.

## Blocked
No run differs from the baseline, so no folder or function is named: not `platform` (`window_wayland.c` sends
`set_destination` once), not `editor` (`dev` behaves the same), not `render`'s device choice (Intel and NVIDIA
behave the same). The planner needs the table above to ask the sponsor, and a measurement of the input path
itself, for instance KWin's `QT_LOGGING_RULES='kwin_*.debug=true'` log while a person presses the start menu
key with the editor open, or the same key test with a stock Vulkan FIFO client such as `vkcube` in its place.
