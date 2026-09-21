# 017 — The editor leaves the rest of the machine alone

## What
The editor never takes more of the machine than the picture needs, and never disturbs the desktop it
runs on.

It draws no faster than the screen shows. When it is the window you are working in it draws at the
display's rate and no faster. When it is not the focused window it drops to a slow heartbeat — a few
frames a second, enough that the picture is right when you look back at it, not enough to cost
anything. When it is not on screen at all — minimised, on another virtual desktop, or fully covered —
it stops drawing entirely and costs nothing.

It draws on the fastest graphics card the machine has, as decision 0201 lays down. This laptop has
two: the Intel chip that the desktop itself is drawn on, and an NVIDIA RTX 4070. The editor takes the
NVIDIA one, so our rendering and the desktop's are not fighting over the same small chip. If there is
only one card it takes that, and if the fastest one cannot present to the window it falls back to the
next one down and says so rather than failing. There is no power-saving choice and no override.

It says which card it chose. On startup the editor prints one line naming the card it is drawing on
and the rate it is drawing at, so this is never guesswork again.

`voe_dev` does the same, by the same rules.

## Why
Running the editor makes the whole desktop sick: clicks land late or not at all, the start menu takes
seconds to appear, the panel fills with smeared text and colour, and the artifacts stay after the
editor is closed. Switching to another virtual desktop and back clears it. `voe_dev` does not do it.

The kernel log for 2026-09-20 says what is happening. Between 19:04 and 19:23 the Intel GPU logs
repeated `Fence expiration time out` against KWin's and Plasma's own render threads and against
Xwayland, and at 19:09:34 `GPU HANG: ecode 12:1:85dffffd, in Xwayland` followed by a context reset.
That is the sludge and the artifacts in one line: everyone on that chip is waiting on fences that
never signal, and after the reset their surfaces hold garbage until something forces a repaint —
which is what changing virtual desktop does. The screenshot of the broken panel is timestamped
19:29, minutes after.

This also closes out decision 0181, which closed the earlier "the editor swallows the desktop's
keyboard" bug as a transient compositor fault and said that if it ever came back it would be filed
as its own piece of work. It came back. It was never a keyboard grab — the keystrokes were queued
behind a compositor that could not get a turn.

One honest caveat for whoever builds this: we have not proved that the editor caused the Intel GPU
to hang. The hang is logged against Xwayland, not against us. What we know is that the editor is
almost certainly drawing on that same Intel chip and drawing flat out, which is enough to explain
the starvation and is wrong on its own merits on a machine with a 4070 sitting idle. If step 6 below
still fails once this is built, the fault is the Intel driver's and it becomes a different piece of
work.

## How to test
1. Start the editor. It prints one line saying which card it is drawing on and at what rate. The card
   named is the NVIDIA RTX 4070, not the Intel one.
2. With the editor focused and a scene open, in a terminal run `ps -o pcpu=,rss= -C voe_editor`. The
   CPU figure is well under one core and stays there. Note the memory figure.
3. Click a terminal so the editor loses focus, wait a few seconds, run the same command. The CPU
   figure has dropped to near nothing. Click back to the editor: the picture is correct and up to
   date, with no flash or catching up.
4. Switch to another virtual desktop, work there for a minute, switch back. Same as step 3, and the
   editor's window is correct the moment it reappears.
5. Minimise the editor and leave it minimised for a minute. `ps` shows near zero CPU.
6. Leave the editor running for an hour while you use the machine normally. Through the whole hour:
   the start menu opens at once, typing in a terminal lands immediately, windows take clicks, and the
   panel stays clean with no smeared text or colour. Then run
   `journalctl -k -b | grep -iE 'GPU HANG|Fence expiration'` — there is nothing newer than the moment
   you started the editor.
7. Check the memory figure from step 2 again after that hour. It has not climbed.
8. Close the editor. The desktop is exactly as it was before you started it.
9. Run `voe_dev` and repeat steps 2 and 3 against it. It behaves the same way.
